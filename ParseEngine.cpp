#include "ParseEngine.h"

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

void search_engine::KaggleFinanceParseEngine::Parse(std::string file_path, const std::unordered_set<std::string>* stop_words_ptr) {
    auto it = std::filesystem::recursive_directory_iterator(file_path);
    for (auto&& entry : it) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".json") {
            this->files_.push_back(std::move(entry.path()));
        }
    }

    this->unformatted_database_ = std::move(std::vector<std::pair<std::string, std::unordered_map<std::string, uint32_t>>>(files_.size()));
    this->database_.text_index = std::move(std::vector<std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>>>(this->filling_thread_count_));
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

std::string search_engine::KaggleFinanceParseEngine::CleanToken(const char* token, std::optional<size_t> size) {
    std::string cleaned_token;
    if (size.has_value() == false) {
        size = strlen(token);
    }
    cleaned_token.resize(size.value());
    // check for unicode characters and lowercase token
    for (size_t i = 0; i < size; i++) {
        if (token[i] < 0 || token[i] > 127) {
            return {};
        }
        cleaned_token[i] = tolower(token[i]);
    }

    return cleaned_token;
}

void search_engine::KaggleFinanceParseEngine::ParseSingleArticle(const size_t file_subscript, const std::unordered_set<std::string>* stop_words_ptr) {
    std::ifstream input(files_[file_subscript]);
    if (input.is_open() == false) {
        std::cerr << "cannot open file: " << this->files_[file_subscript].string() << std::endl;
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw(input);
    doc.ParseStream(isw);

    const char* delimeters = " \t\v\n\r,.?!;:\"/()";
    std::string_view uuid = this->unformatted_database_[file_subscript].first = std::move(doc["uuid"].GetString());

    pthread_mutex_lock(&this->metadata_mutex_);
    this->database_.id_map[uuid.data()] = this->files_[file_subscript].string();
    this->database_.site_index[doc["thread"]["site"].GetString()].emplace(uuid);
    this->database_.author_index[doc["author"].GetString()].emplace(uuid);
    this->database_.country_index[doc["thread"]["country"].GetString()].emplace(uuid);
    this->database_.language_index[doc["language"].GetString()].emplace(uuid);

    const auto& people_array = doc["entities"]["persons"].GetArray();
    for (const auto& person : people_array) {
        this->database_.person_index[person["name"].GetString()].emplace(uuid);
    }

    const auto& location_array = doc["entities"]["locations"].GetArray();
    for (const auto& location : location_array) {
        this->database_.location_index[location["name"].GetString()].emplace(uuid);
    }

    const auto& organization_array = doc["entities"]["organizations"].GetArray();
    for (const auto& organization : organization_array) {
        this->database_.organization_index[organization["name"].GetString()].emplace(uuid);
    }
    pthread_mutex_unlock(&this->metadata_mutex_);

    std::unordered_map<std::string, uint32_t>& word_map = this->unformatted_database_[file_subscript].second;
    std::string text(std::move(doc["text"].GetString()));
    char* save_ptr;
    char* token = strtok_r(text.data(), delimeters, &save_ptr);
    while (token != NULL) {
        size_t token_length = strlen(token);

        std::string clean = std::move(this->CleanToken(token, token_length));
        if (clean.empty() == true || (stop_words_ptr != NULL && stop_words_ptr->find(clean) != stop_words_ptr->end())) {
            token = strtok_r(NULL, delimeters, &save_ptr);
            continue;
        }
        auto iter = word_map.emplace(std::move(clean), 0);
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
    ParsingThreadArgs* thread_args = (ParsingThreadArgs*)_arg;
    for (size_t i = thread_args->start; i < thread_args->end; i++) {
        thread_args->obj_ptr->ParseSingleArticle(i, thread_args->stop_words_ptr);
    }

    return NULL;
}

void* search_engine::KaggleFinanceParseEngine::ArbitratorThreadFunc(void* _arg) {
    search_engine::KaggleFinanceParseEngine* parse_engine = (search_engine::KaggleFinanceParseEngine*)_arg;
    while (parse_engine->currently_parsing_ == true || parse_engine->arbitrator_buffer_.empty() == false) {
        if (sem_trywait(&parse_engine->production_state_sem_) != 0) {
            continue;
        }
        pthread_mutex_lock(&parse_engine->arbitrator_buffer_mutex_);
        const size_t i = parse_engine->arbitrator_buffer_.front();
        parse_engine->arbitrator_buffer_.pop();
        pthread_mutex_unlock(&parse_engine->arbitrator_buffer_mutex_);

        for (auto&& inner_element : parse_engine->unformatted_database_[i].second) {
            if (inner_element.first.empty() == true) {
                continue;
            }

            size_t word_buffer_index = 0;
            word_buffer_index = size_t(inner_element.first[0]) % parse_engine->filling_thread_count_;

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
    FillingThreadArgs* thread_args = (FillingThreadArgs*)_arg;
    while (thread_args->obj_ptr->currently_parsing_ == true || thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_subscript].empty() == false) {
        if (sem_trywait(&thread_args->obj_ptr->arbitrator_sem_vec_[thread_args->buffer_subscript]) != 0) {
            continue;
        }
        pthread_mutex_lock(&thread_args->obj_ptr->alpha_buffer_mutex_[thread_args->buffer_subscript]);
        const AlphaBufferArgs word_args = std::move(thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_subscript].front());
        thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_subscript].pop();
        pthread_mutex_unlock(&thread_args->obj_ptr->alpha_buffer_mutex_[thread_args->buffer_subscript]);

        thread_args->obj_ptr->database_.text_index[thread_args->buffer_subscript][std::move(word_args.word)].emplace(thread_args->obj_ptr->unformatted_database_[word_args.file_subscript].first, word_args.count);
    }
    return NULL;
}
