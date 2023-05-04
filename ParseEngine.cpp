#include "ParseEngine.h"

#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>

#include <pthread.h>


#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"

search_engine::parse_util::WordStatisticList::WordStatisticList(const search_engine::parse_util::WordStatisticList& list) {
    this->dummy_ = new ListNode();
    this->dummy_->prev = this->dummy_;
    this->dummy_->next = this->dummy_;
    ListNode* iter = list.dummy_->next;
    while (iter != list.dummy_) {
        this->PushBack(iter->uuid, iter->count);
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
            this->PushBack(iter->uuid, iter->count);
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

    std::vector<std::pair<std::string, std::unordered_map<std::string, int64_t>>> unformatted_database(files_.size());  // uuid -> {word -> appearance count}
    for (size_t i = 0; i < files_.size(); i++) {
        if(i % 10000 == 0){
            std::cout << i << std::endl;
        }
        this->ParseSingleArticle(i, unformatted_database);
    }

    // database_.text_index = std::move(this->MergeIntoDatabase(0, files_.size(), unformatted_database));

    int counter = 0;
    for (auto&& outer_element : unformatted_database) {
        if(counter % 10000 == 0){
            std::cout << counter << std::endl;
        }

        std::unordered_map<std::string, parse_util::WordStatisticList> word_map;
        for (auto&& inner_element : outer_element.second) {
            word_map[inner_element.first].PushBack(outer_element.first, inner_element.second);
        }

        for (auto&& word_element : word_map) {
            database_.text_index[word_element.first].CatGive(word_element.second);
        }
        counter++;
    }

    for (auto&& i : database_.text_index) {
        std::cout << i.first << std::endl;
        for (parse_util::WordStatisticList::ListNode* j = i.second.Front(); j != i.second.End(); j = j->next) {
            std::cout << '\t' << j->uuid << ' ' << j->count << std::endl;
        }
    }
}

void search_engine::KaggleFinanceParseEngine::ParseSingleArticle(const size_t i, std::vector<std::pair<std::string, std::unordered_map<std::string, int64_t>>>& unformatted_database) {
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
    unformatted_database[i].first = doc["uuid"].GetString();
    std::unordered_map<std::string, int64_t>& word_map = unformatted_database[i].second;

    char* token = std::strtok(text.data(), delimeters);
    while (token != NULL) {
        // cleaning token

        auto iter = word_map.emplace(token, 0);
        iter.first->second++;

        token = std::strtok(nullptr, delimeters);
    }
}

std::unordered_map<std::string, search_engine::parse_util::WordStatisticList> search_engine::KaggleFinanceParseEngine::MergeIntoDatabase(const size_t low, const size_t high, std::vector<std::pair<std::string, std::unordered_map<std::string, int64_t>>>& unformatted_database) {
    if (low > high) {
        return {};
    } else if (low == high){
        const size_t i = low;
        std::unordered_map<std::string, parse_util::WordStatisticList> word_map;
        for (auto&& inner_element : unformatted_database[i].second) {
            word_map[inner_element.first].PushBack(unformatted_database[i].first, inner_element.second);
        }
        return word_map;
    }
    
    size_t mid = (low + high) / 2;
    std::unordered_map<std::string, parse_util::WordStatisticList> word_map_left = std::move(this->MergeIntoDatabase(low, mid, unformatted_database));
    std::unordered_map<std::string, parse_util::WordStatisticList> word_map_right = std::move(this->MergeIntoDatabase(mid + 1, high, unformatted_database));

    for (auto&& right_map_element : word_map_right) {
        auto iter = word_map_left.emplace(right_map_element.first, right_map_element.second);
        if(iter.second == false){
            iter.first->second.CatGive(right_map_element.second);
        }
    }
    return word_map_left;
}