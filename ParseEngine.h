#ifndef SEARCH_ENGINE_PROJECT_INDEXENGINE_H_
#define SEARCH_ENGINE_PROJECT_INDEXENGINE_H_

#include <semaphore.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace search_engine {

namespace parse_util {

struct RunTimeDataBase {
    std::unordered_map<std::string, std::string> id_map;            // uuid -> file path
    std::vector<std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>>> text_index;  // word -> list of {uuid -> count}
    std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> title_index;
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
    KaggleFinanceParseEngine(size_t parse_amount, size_t fill_amount) : parsing_thread_count_(parse_amount), filling_thread_count_(fill_amount) {
        sem_init(&this->production_state_sem_, 1, 0);
        pthread_mutex_init(&buffer_mutex_, NULL);
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
    void Parse(std::string file_path, const std::unordered_set<std::string>* stop_words) override;
    inline const parse_util::RunTimeDataBase* GetRunTimeDataBase() const override { return &database_; };

   private:
    struct ParsingThreadArgs {
        KaggleFinanceParseEngine* obj_ptr;
        const std::unordered_set<std::string>* stop_words_ptr;
        size_t start;
        size_t end;
    };
    struct FillingThreadArgs {
        KaggleFinanceParseEngine* obj_ptr;
        size_t buffer_subscript;
    };
    struct AlphaBufferArgs {
        size_t file_subscript;
        std::string word;
        uint32_t count;
    };
    void ParseSingleArticle(const size_t file_subscript, const std::unordered_set<std::string>* stop_words_ptr);
    static void* ParsingThreadFunc(void* _arg); //producer
    static void* ArbitratorThreadFunc(void* _arg); //consumer and producer
    static void* FillingThreadFunc(void* _arg); //consumer

    parse_util::RunTimeDataBase database_;
    std::vector<std::pair<std::string, std::unordered_map<std::string, uint32_t>>> unformatted_database_;
    std::vector<std::filesystem::__cxx11::path> files_;
    size_t parsing_thread_count_;
    size_t filling_thread_count_;
    std::queue<size_t> buffer_;
    std::vector<std::queue<AlphaBufferArgs>> alpha_buffer_;
    bool currently_parsing_ = false;
    sem_t production_state_sem_;
    std::vector<sem_t> arbitrator_sem_vec_;
    pthread_mutex_t buffer_mutex_;
    std::vector<pthread_mutex_t> alpha_buffer_mutex_;
};

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_INDEXENGINE_H_