#ifndef SEARCH_ENGINE_PROJECT_KAGGLEFINANCEPARSEENGINE_H_
#define SEARCH_ENGINE_PROJECT_KAGGLEFINANCEPARSEENGINE_H_

#include <semaphore.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <queue>

#include "ParseEngine.h"

namespace search_engine {

/// @brief The `KaggleFinanceParseEngine` class should be used to parse the data found at https://www.kaggle.com/datasets/jeet2016/us-financial-news-articles
class KaggleFinanceParseEngine : public parse_util::ParseEngine<size_t, std::string> {
   public:
    explicit KaggleFinanceParseEngine(size_t parse_amount, size_t fill_amount);
    void ParseData(std::string file_path, const std::unordered_set<std::string>* const stop_words = NULL) override;
    size_t CleanID(const char* const id, std::optional<size_t> size = std::nullopt) override;
    std::string CleanValue(const char* const token, std::optional<size_t> size = std::nullopt) override;
    std::string CleanMetaData(const char* const token, std::optional<size_t> size = std::nullopt) override;
    inline const parse_util::RunTimeDatabase<size_t, std::string>* const GetRunTimeDatabase() const override { return &database_; };

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

    void ParseSingleArticle(const size_t file_subscript, const std::unordered_set<std::string>* const stop_words_ptr);
    static void* ParsingThreadFunc(void* _arg);     // producer
    static void* ArbitratorThreadFunc(void* _arg);  // consumer and producer
    static void* FillingThreadFunc(void* _arg);     // consumer

    parse_util::RunTimeDatabase<size_t, std::string> database_;  // todo: change database_ to be a map of size_t (hashed words) to size_t (hashed uuids)
    std::vector<std::pair<size_t, std::unordered_map<std::string, uint32_t>>> unformatted_database_;
    std::vector<std::filesystem::__cxx11::path> files_;
    size_t parsing_thread_count_;
    size_t filling_thread_count_;
    std::queue<size_t> arbitrator_buffer_;
    std::vector<std::queue<AlphaBufferArgs>> alpha_buffer_;
    bool currently_parsing_ = false;
    sem_t production_state_sem_;
    std::vector<sem_t> arbitrator_sem_vec_;
    pthread_mutex_t arbitrator_buffer_mutex_;
    pthread_mutex_t metadata_mutex_;
    std::vector<pthread_mutex_t> alpha_buffer_mutex_;
};

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_KAGGLEFINANCEPARSEENGINE_H_