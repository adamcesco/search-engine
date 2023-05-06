#include "ParseEngine.h"

#include <pthread.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

search_engine::parse_util::WordStatisticList::WordStatisticList(const search_engine::parse_util::WordStatisticList& list) {
    this->dummy_ = new ListNode();
    this->dummy_->prev = this->dummy_;
    this->dummy_->next = this->dummy_;
    ListNode* iter = list.dummy_->next;
    while (iter != list.dummy_) {
        ListNode* node = new ListNode(iter->uuid, iter->count);
        this->dummy_->prev->next = node;
        node->prev = this->dummy_->prev;
        this->dummy_->prev = node;
        node->next = this->dummy_;
        iter = iter->next;
    }
}

search_engine::parse_util::WordStatisticList& search_engine::parse_util::WordStatisticList::operator=(const search_engine::parse_util::WordStatisticList& list) {
    if (this == &list) {
        return *this;
    }

    {
        ListNode* iter = dummy_;
        dummy_->prev->next = nullptr;
        while (iter != nullptr) {
            ListNode* next = iter->next;
            delete iter;
            iter = next;
        }
    }
    {
        this->dummy_ = new ListNode();
        this->dummy_->prev = this->dummy_;
        this->dummy_->next = this->dummy_;
        ListNode* iter = list.dummy_->next;
        while (iter != list.dummy_) {
            ListNode* node = new ListNode(iter->uuid, iter->count);
            this->dummy_->prev->next = node;
            node->prev = this->dummy_->prev;
            this->dummy_->prev = node;
            node->next = this->dummy_;

            iter = iter->next;
        }
    }
    return *this;
}

search_engine::parse_util::WordStatisticList::~WordStatisticList() {
    ListNode* iter = dummy_;
    dummy_->prev->next = nullptr;
    while (iter != nullptr) {
        ListNode* next = iter->next;
        delete iter;
        iter = next;
    }
}

//-----------------------------------------------

void search_engine::KaggleFinanceParseEngine::Parse(std::string file_path, const std::unordered_set<std::string>* stop_words) {
    auto it = std::filesystem::recursive_directory_iterator(file_path);
    for (const auto& entry : it) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".json") {
            this->files_.push_back(entry.path());
        }
    }

    this->unformatted_database_ = std::move(std::vector<std::pair<std::string, std::unordered_map<std::string, int64_t>>>(files_.size()));  // uuid -> {word -> appearance count}
    pthread_t parsing_thread_array[this->thread_count_];
    args arg_array[this->thread_count_];
    size_t proportion = files_.size() / this->thread_count_;
    size_t prev = 0;
    for (int64_t i = 0; i < this->thread_count_ - 1; i++) {
        arg_array[i] = {
            .obj_ptr = this,
            .start = prev,
            .end = prev + proportion,
        };
        pthread_create(parsing_thread_array + i, NULL, this->ThreadParser, (void*)(arg_array + i));
        prev = prev + proportion;
    }
    arg_array[this->thread_count_ - 1] = {
        .obj_ptr = this,
        .start = prev,
        .end = files_.size(),
    };
    pthread_create(parsing_thread_array + thread_count_ - 1, NULL, this->ThreadParser, (void*)(arg_array + this->thread_count_ - 1));
    for (int64_t i = 0; i < this->thread_count_; i++) {
        pthread_join(parsing_thread_array[i], NULL);
    }

    for (auto&& outer_element : unformatted_database_) {
        for (auto&& inner_element : outer_element.second) {
            database_.text_index[std::move(inner_element.first)].PushBack(outer_element.first, inner_element.second);
        }
    }

    // prints out the data within the text index
    // for (auto&& i : database_.text_index) {
    //     std::cout << i.first << std::endl;
    //     for (parse_util::WordStatisticList::ListNode* j = i.second.Front(); j != i.second.End(); j = j->next) {
    //         std::cout << '\t' << j->uuid << ' ' << j->count << std::endl;
    //     }
    // }
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
}

void* search_engine::KaggleFinanceParseEngine::ThreadParser(void* _arg) {
    args* arguments = (args*)_arg;
    for (size_t i = arguments->start; i < arguments->end; i++) {
        arguments->obj_ptr->ParseSingleArticle(i);
    }

    return nullptr;
}