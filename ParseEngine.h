#ifndef SEARCH_ENGINE_PROJECT_INDEXENGINE_H_
#define SEARCH_ENGINE_PROJECT_INDEXENGINE_H_

#include <cstdint>
#include <filesystem>
#include <list>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace search_engine {

namespace parse_util {

class WordStatisticList {
   public:
    struct ListNode {
        std::string uuid;
        int64_t count = 0;
        ListNode* next = nullptr;
        ListNode* prev = nullptr;
        ListNode() = default;
        explicit ListNode(std::string id, int64_t x) : uuid(std::move(id)), count(std::move(x)) {}
        ListNode(std::string id, int64_t x, ListNode* next, ListNode* prev) : uuid(std::move(id)), count(std::move(x)), next(std::move(next)), prev(std::move(prev)) {}
    };

    // constructor
    WordStatisticList() : dummy_(new ListNode()) {
        this->dummy_->prev = this->dummy_;
        this->dummy_->next = this->dummy_;
    };

    // rule of 3
    WordStatisticList(const WordStatisticList& list);
    WordStatisticList& operator=(const WordStatisticList& list);
    ~WordStatisticList();

    inline void PushBack(const std::string& id, int64_t word_count) {
        ListNode* node = new ListNode(id, word_count);
        this->dummy_->prev->next = node;
        node->prev = this->dummy_->prev;
        this->dummy_->prev = node;
        node->next = this->dummy_;
    }

    inline void CatGive(WordStatisticList& list) {
        if (list.dummy_ == nullptr || list.dummy_->next == list.dummy_ || list.dummy_->prev == list.dummy_) {
            return;
        }
        this->dummy_->prev->next = list.dummy_->next;
        list.dummy_->next->prev = this->dummy_->prev;
        list.dummy_->prev->next = this->dummy_;
        this->dummy_->prev = list.dummy_->prev;
        list.dummy_->prev = list.dummy_;
        list.dummy_->next = list.dummy_;
    }

    inline ListNode* Front() { return dummy_->next; }
    inline ListNode* Back() { return dummy_->prev; }
    inline const ListNode* End() { return dummy_; }

   private:
    ListNode* dummy_ = nullptr;
};

struct RunTimeDataBase {
    std::unordered_map<std::string, std::string> id_map;            // uuid to file path
    std::unordered_map<std::string, WordStatisticList> text_index;  // word -> list of {uuid -> count}
    std::unordered_map<std::string, WordStatisticList> title_index;
    std::unordered_map<std::string, std::vector<std::string>> site_index;
    std::unordered_map<std::string, std::vector<std::string>> language_index;
    std::unordered_map<std::string, std::vector<std::string>> location_index;
    std::unordered_map<std::string, std::vector<std::string>> people_index;
    std::unordered_map<std::string, std::vector<std::string>> organization_index;
    std::unordered_map<std::string, std::vector<std::string>> author_index;
    std::unordered_map<std::string, std::vector<std::string>> country_index;
};

class ParseEngine {
   public:
    // the value pointed to by the `stop_words` pointer must outlive this functions entire execution, but providing a value for `stop_words` is optional
    virtual void Parse(std::string file_path, const std::unordered_set<std::string>* stop_words) = 0;

    // the use of the return value should be restricted to the lifetime of the ParseEngine object
    virtual inline const RunTimeDataBase* GetRunTimeDataBase() const = 0;
};

}  // namespace parse_util

// the `KaggleFinanceParseEngine` class will be used to parse the data found at https://www.kaggle.com/datasets/jeet2016/us-financial-news-articles
class KaggleFinanceParseEngine : public parse_util::ParseEngine {
   public:
    KaggleFinanceParseEngine(int64_t thread_amount) : thread_count_(thread_amount) {}
    void Parse(std::string file_path, const std::unordered_set<std::string>* stop_words) override;
    inline const parse_util::RunTimeDataBase* GetRunTimeDataBase() const override { return &database_; };

   private:
    struct args {
        KaggleFinanceParseEngine* obj_ptr;
        size_t start;
        size_t end;
    };
    void ParseSingleArticle(const size_t i);
    static void* ThreadParser(void* _arg);
    std::unordered_map<std::string, parse_util::WordStatisticList> MergeIntoDatabase(const size_t low, const size_t high);

    parse_util::RunTimeDataBase database_;
    std::vector<std::pair<std::string, std::unordered_map<std::string, int64_t>>> unformatted_database_;
    std::vector<std::filesystem::__cxx11::path> files_;
    int64_t thread_count_;
};

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_INDEXENGINE_H_