#include "ParseEngine.h"

#include <pthread.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

void search_engine::KaggleFinanceParseEngine::Parse(std::string file_path, const std::unordered_set<std::string>* stop_words) {
    auto it = std::filesystem::recursive_directory_iterator(file_path);
    for (const auto& entry : it) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".json") {
            this->files_.push_back(entry.path());
        }
    }

    this->unformatted_database_ = std::move(std::vector<std::pair<std::string, std::unordered_map<std::string, int64_t>>>(files_.size()));                        // uuid -> {word -> appearance count}
    this->database_.text_index = std::move(std::vector<std::unordered_map<std::string, std::unordered_map<std::string, int64_t>>>(this->filling_thread_count_));  // uuid -> {word -> appearance count}
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
            .buffer_index = i,
        };
        pthread_create(filling_thread_array + i, NULL, this->FillingThreadFunc, (void*)(filling_arg_array + i));
    }

    size_t proportion = files_.size() / this->parsing_thread_count_;
    size_t prev = 0;
    for (size_t i = 0; i < this->parsing_thread_count_ - 1; i++) {
        parsing_arg_array[i] = {
            .obj_ptr = this,
            .start = prev,
            .end = prev + proportion,
        };
        pthread_create(parsing_thread_array + i, NULL, this->ParsingThreadFunc, (void*)(parsing_arg_array + i));
        prev = prev + proportion;
    }
    parsing_arg_array[this->parsing_thread_count_ - 1] = {
        .obj_ptr = this,
        .start = prev,
        .end = files_.size(),
    };
    pthread_create(parsing_thread_array + this->parsing_thread_count_ - 1, NULL, this->ParsingThreadFunc, (void*)(parsing_arg_array + this->parsing_thread_count_ - 1));
    for (int64_t i = 0; i < this->parsing_thread_count_; i++) {
        pthread_join(parsing_thread_array[i], NULL);
    }
    this->currently_parsing_ = false;
    for (int64_t i = 0; i < this->filling_thread_count_; i++) {
        pthread_join(filling_thread_array[i], NULL);
    }

    // prints out the data within the text index
    for (auto&& i : database_.text_index) {
        for (auto&& k : i) {
            std::cout << k.first << std::endl;
            for (auto&& j : k.second) {
                std::cout << '\t' << j.first << ' ' << j.second << std::endl;
            }
        }
    }
}

void search_engine::KaggleFinanceParseEngine::ParseSingleArticle(const size_t i) {  // todo: fill all other data within `database_`
    std::ifstream input(files_[i]);
    if (input.is_open() == false) {
        std::cerr << "cannot open file: " << files_[i].string() << std::endl;
        return;
    }

    rapidjson::Document doc;
    rapidjson::IStreamWrapper isw(input);
    doc.ParseStream(isw);

    const char* delimeters = " \t\v\n\r,.?!;:\"()";
    std::string text = doc["text"].GetString();
    unformatted_database_[i].first = doc["uuid"].GetString();
    std::unordered_map<std::string, int64_t>& word_map = unformatted_database_[i].second;

    char* save_ptr;
    char* token = strtok_r(text.data(), delimeters, &save_ptr);
    while (token != NULL) {
        // todo: clean token

        auto iter = word_map.emplace(token, 0);
        iter.first->second++;

        token = strtok_r(NULL, delimeters, &save_ptr);
    }
    input.close();

    pthread_mutex_lock(&buffer_mutex_);
    this->buffer_.push(i);
    pthread_mutex_unlock(&buffer_mutex_);
    sem_post(&this->production_state_sem_);
}

void* search_engine::KaggleFinanceParseEngine::ParsingThreadFunc(void* _arg) {
    ParsingThreadArgs* thread_args = (ParsingThreadArgs*)_arg;
    for (size_t i = thread_args->start; i < thread_args->end; i++) {
        thread_args->obj_ptr->ParseSingleArticle(i);
    }

    return nullptr;
}

void* search_engine::KaggleFinanceParseEngine::ArbitratorThreadFunc(void* _arg) {
    search_engine::KaggleFinanceParseEngine* parse_engine = (search_engine::KaggleFinanceParseEngine*)_arg;
    while (parse_engine->currently_parsing_ == true || parse_engine->buffer_.empty() == false) {
        if (sem_trywait(&parse_engine->production_state_sem_) != 0) {
            continue;
        }
        const size_t i = parse_engine->buffer_.front();
        pthread_mutex_lock(&parse_engine->buffer_mutex_);
        parse_engine->buffer_.pop();
        pthread_mutex_unlock(&parse_engine->buffer_mutex_);

        for (auto&& inner_element : parse_engine->unformatted_database_[i].second) {
            if (inner_element.first.empty() == true) {
                continue;
            }
            
            size_t index = 0;
            index = int16_t(inner_element.first[0]) % parse_engine->filling_thread_count_;

            parse_engine->alpha_buffer_[index].push(AlphaBufferArgs{
                .file_index = i,
                .word = std::move(inner_element.first),
                .count = inner_element.second,
            });
            sem_post(&parse_engine->arbitrator_sem_vec_[index]);
        }
    }
    return nullptr;
}

void* search_engine::KaggleFinanceParseEngine::FillingThreadFunc(void* _arg) {
    FillingThreadArgs* thread_args = (FillingThreadArgs*)_arg;
    while (thread_args->obj_ptr->currently_parsing_ == true || thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_index].empty() == false) {
        if (sem_trywait(&thread_args->obj_ptr->arbitrator_sem_vec_[thread_args->buffer_index]) != 0) {
            continue;
        }
        const AlphaBufferArgs word_args = std::move(thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_index].front());
        thread_args->obj_ptr->alpha_buffer_[thread_args->buffer_index].pop();
        thread_args->obj_ptr->database_.text_index[thread_args->buffer_index][std::move(word_args.word)].emplace(thread_args->obj_ptr->unformatted_database_[word_args.file_index].first, word_args.count);
    }
    return nullptr;
}
