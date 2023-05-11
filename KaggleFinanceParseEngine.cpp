#include "KaggleFinanceParseEngine.h"

#include <pthread.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

search_engine::KaggleFinanceParseEngine::KaggleFinanceParseEngine(size_t parse_amount, size_t fill_amount) : parsing_thread_count_(parse_amount), filling_thread_count_(fill_amount) {
    sem_init(&this->production_state_sem_, 1, 0);
    pthread_mutex_init(&arbitrator_buffer_mutex_, NULL);
    pthread_mutex_init(&metadata_mutex_, NULL);
    this->alpha_buffer_ = std::move(std::vector<std::queue<AlphaBufferArgs>>(this->filling_thread_count_));
    this->arbitrator_sem_vec_ = std::move(std::vector<sem_t>(this->filling_thread_count_));
    for (size_t i = 0; i < this->filling_thread_count_; i++) {
        sem_init(this->arbitrator_sem_vec_.data() + i, 1, 0);
    }
    this->alpha_buffer_mutex_ = std::move(std::vector<pthread_mutex_t>(this->filling_thread_count_));
    for (size_t i = 0; i < this->filling_thread_count_; i++) {
        pthread_mutex_init(this->alpha_buffer_mutex_.data() + i, NULL);
    }
}

void search_engine::KaggleFinanceParseEngine::ParseData(std::string file_path, const std::unordered_set<size_t>* const stop_words_ptr) {
    auto it = std::filesystem::recursive_directory_iterator(file_path);
    for (auto&& entry : it) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".json") {
            this->files_.push_back(std::move(entry.path()));
        }
    }

    this->unformatted_database_ = std::move(std::vector<std::pair<size_t, std::unordered_map<size_t, uint32_t>>>(files_.size()));
    this->database_.value_index = std::move(std::vector<std::unordered_map<size_t, std::unordered_map<size_t, uint32_t>>>(this->filling_thread_count_));
    this->currently_parsing_ = true;

    ParsingThreadArgs parsing_arg_array[this->parsing_thread_count_];
    FillingThreadArgs filling_arg_array[this->filling_thread_count_];
    pthread_t parsing_thread_array[this->parsing_thread_count_];
    pthread_t filling_arbitrator_thread;
    pthread_t filling_thread_array[this->filling_thread_count_];

    pthread_create(&filling_arbitrator_thread, NULL, this->ArbitratorThreadFunc, (void*)(this));
    for (size_t i = 0; i < this->filling_thread_count_; i++) {
        filling_arg_array[i] = {
            .obj_ptr = this,
            .buffer_subscript = i,
        };
        pthread_create(filling_thread_array + i, NULL, this->FillingThreadFunc, (void*)(filling_arg_array + i));
    }

    size_t proportion = files_.size() / this->parsing_thread_count_;
    size_t prev = 0;
    for (size_t i = 0; i < this->parsing_thread_count_ - 1; i++) {
        parsing_arg_array[i] = {
            .obj_ptr = this,
            .stop_words_ptr = stop_words_ptr,
            .start = prev,
            .end = prev + proportion,
        };
        pthread_create(parsing_thread_array + i, NULL, this->ParsingThreadFunc, (void*)(parsing_arg_array + i));
        prev = prev + proportion;
    }
    parsing_arg_array[this->parsing_thread_count_ - 1] = {
        .obj_ptr = this,
        .stop_words_ptr = stop_words_ptr,
        .start = prev,
        .end = files_.size(),
    };
    pthread_create(parsing_thread_array + this->parsing_thread_count_ - 1, NULL, this->ParsingThreadFunc, (void*)(parsing_arg_array + this->parsing_thread_count_ - 1));

    for (size_t i = 0; i < this->parsing_thread_count_; i++) {
        pthread_join(parsing_thread_array[i], NULL);
    }
    this->currently_parsing_ = false;
    for (size_t i = 0; i < this->filling_thread_count_; i++) {
        pthread_join(filling_thread_array[i], NULL);
    }
    pthread_join(filling_arbitrator_thread, NULL);
}

size_t search_engine::KaggleFinanceParseEngine::CleanID(const char* const id_token, std::optional<size_t> size) {
    return std::hash<std::string_view>{}(std::string_view(id_token));
}

size_t search_engine::KaggleFinanceParseEngine::CleanValue(const char* const value_token, std::optional<size_t> size) {
    std::string cleaned_token;
    if (size.has_value() == false) {
        size = strlen(value_token);
    }
    cleaned_token.resize(size.value());
    // check for unicode characters and lowercase token
    for (size_t i = 0, j = 0; i < size; i++, j++) {
        if (value_token[i] < 0 || value_token[i] > 127) {
            return std::string::npos;
        }
        if (value_token[i] == '\'') {
            j--;
            continue;
        }
        cleaned_token[j] = tolower(value_token[i]);
    }

    return std::hash<std::string_view>{}(std::string_view(cleaned_token));
}

std::string search_engine::KaggleFinanceParseEngine::CleanMetaData(const char* const metadata_token, std::optional<size_t> size) {
    std::string cleaned_token;
    if (size.has_value() == false) {
        size = strlen(metadata_token);
    }
    cleaned_token.resize(size.value());
    // check for unicode characters and lowercase token
    for (size_t i = 0, j = 0; i < size; i++, j++) {
        if (metadata_token[i] < 0 || metadata_token[i] > 127) {
            return {};
        }
        if (metadata_token[i] == '\'') {
            j--;
            continue;
        }
        cleaned_token[j] = tolower(metadata_token[i]);
    }

    return cleaned_token;
}

void search_engine::KaggleFinanceParseEngine::ParseSingleArticle(const size_t file_subscript, const std::unordered_set<size_t>* stop_words_ptr) {
    std::ifstream input(this->files_[file_subscript]);
    if (input.is_open() == false) {
        std::cerr << "cannot open file: " << this->files_[file_subscript].string() << std::endl;
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw(input);
    doc.ParseStream(isw);

    const char* const delimeters = " \t\v\n\r,.?!;:\"/()";
    size_t uuid = this->unformatted_database_[file_subscript].first = std::move(this->CleanID(doc["uuid"].GetString()));

    pthread_mutex_lock(&this->metadata_mutex_);
    this->database_.id_map[uuid] = this->files_[file_subscript].string();
    this->database_.site_index[this->CleanMetaData(doc["thread"]["site"].GetString())].emplace(uuid);
    this->database_.author_index[this->CleanMetaData(doc["author"].GetString())].emplace(uuid);
    this->database_.country_index[this->CleanMetaData(doc["thread"]["country"].GetString())].emplace(uuid);
    this->database_.language_index[this->CleanMetaData(doc["language"].GetString())].emplace(uuid);

    rapidjson::GenericArray<false, rapidjson::Value> people_array = std::move(doc["entities"]["persons"].GetArray());
    for (auto&& person : people_array) {
        this->database_.person_index[this->CleanMetaData(person["name"].GetString())].emplace(uuid);
    }

    rapidjson::GenericArray<false, rapidjson::Value> location_array = std::move(doc["entities"]["locations"].GetArray());
    for (auto&& location : location_array) {
        this->database_.location_index[this->CleanMetaData(location["name"].GetString())].emplace(uuid);
    }

    rapidjson::GenericArray<false, rapidjson::Value> organization_array = std::move(doc["entities"]["organizations"].GetArray());
    for (auto&& organization : organization_array) {
        this->database_.organization_index[this->CleanMetaData(organization["name"].GetString())].emplace(uuid);
    }

    char* title_token = strtok((char*)doc["thread"]["title"].GetString(), delimeters);
    while (title_token != NULL) {
        size_t title_token_length = strlen(title_token);

        size_t cleaned_title_token = this->CleanValue(title_token, title_token_length);
        if (cleaned_title_token == std::string::npos) {
            title_token = strtok(NULL, delimeters);
            continue;
        }
        auto iter = this->database_.title_index[cleaned_title_token].emplace(uuid, 0);
        iter.first->second++;

        title_token = strtok(NULL, delimeters);
    }
    pthread_mutex_unlock(&this->metadata_mutex_);

    std::unordered_map<size_t, uint32_t>& word_map = this->unformatted_database_[file_subscript].second;
    char* save_ptr;
    char* token = strtok_r((char*)doc["text"].GetString(), delimeters, &save_ptr);
    while (token != NULL) {
        size_t token_length = strlen(token);

        size_t cleaned_token = this->CleanValue(token, token_length);
        if (cleaned_token == std::string::npos || (stop_words_ptr != NULL && stop_words_ptr->find(cleaned_token) != stop_words_ptr->end())) {
            token = strtok_r(NULL, delimeters, &save_ptr);
            continue;
        }
        auto iter = word_map.emplace(cleaned_token, 0);
        iter.first->second++;

        token = strtok_r(NULL, delimeters, &save_ptr);
    }
    input.close();

    pthread_mutex_lock(&this->arbitrator_buffer_mutex_);
    this->arbitrator_buffer_.push(file_subscript);
    pthread_mutex_unlock(&this->arbitrator_buffer_mutex_);
    sem_post(&this->production_state_sem_);
}

void* search_engine::KaggleFinanceParseEngine::ParsingThreadFunc(void* _arg) {
    ParsingThreadArgs* const thread_args = (ParsingThreadArgs*)_arg;
    for (size_t i = thread_args->start; i < thread_args->end; i++) {
        thread_args->obj_ptr->ParseSingleArticle(i, thread_args->stop_words_ptr);
    }

    return NULL;
}

void* search_engine::KaggleFinanceParseEngine::ArbitratorThreadFunc(void* _arg) {
    search_engine::KaggleFinanceParseEngine* const parse_engine = (search_engine::KaggleFinanceParseEngine*)_arg;
    while (parse_engine->currently_parsing_ == true || parse_engine->arbitrator_buffer_.empty() == false) {
        if (sem_trywait(&parse_engine->production_state_sem_) != 0) {
            continue;
        }
        pthread_mutex_lock(&parse_engine->arbitrator_buffer_mutex_);
        const size_t i = parse_engine->arbitrator_buffer_.front();
        parse_engine->arbitrator_buffer_.pop();
        pthread_mutex_unlock(&parse_engine->arbitrator_buffer_mutex_);

        for (auto&& inner_element : parse_engine->unformatted_database_[i].second) {
            size_t word_buffer_index = inner_element.first % parse_engine->filling_thread_count_;

            pthread_mutex_lock(&parse_engine->alpha_buffer_mutex_[word_buffer_index]);
            parse_engine->alpha_buffer_[word_buffer_index].push(std::move(AlphaBufferArgs{
                .file_subscript = i,
                .word = std::move(inner_element.first),
                .count = inner_element.second,
            }));
            pthread_mutex_unlock(&parse_engine->alpha_buffer_mutex_[word_buffer_index]);
            sem_post(&parse_engine->arbitrator_sem_vec_[word_buffer_index]);
        }
    }
    return NULL;
}

void* search_engine::KaggleFinanceParseEngine::FillingThreadFunc(void* _arg) {
    FillingThreadArgs* const thread_args = (FillingThreadArgs*)_arg;
    while (thread_args->obj_ptr->currently_parsing_ == true || thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_subscript].empty() == false) {
        if (sem_trywait(&thread_args->obj_ptr->arbitrator_sem_vec_[thread_args->buffer_subscript]) != 0) {
            continue;
        }
        pthread_mutex_lock(&thread_args->obj_ptr->alpha_buffer_mutex_[thread_args->buffer_subscript]);
        const AlphaBufferArgs word_args = std::move(thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_subscript].front());
        thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_subscript].pop();
        pthread_mutex_unlock(&thread_args->obj_ptr->alpha_buffer_mutex_[thread_args->buffer_subscript]);

        thread_args->obj_ptr->database_.value_index[thread_args->buffer_subscript][word_args.word].emplace(thread_args->obj_ptr->unformatted_database_[word_args.file_subscript].first, word_args.count);
    }
    return NULL;
}