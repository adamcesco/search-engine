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
    std::unordered_map<std::string, std::string> id_map;                                                 // uuid -> file path
    std::vector<std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>>> text_index;  // word -> list of {uuid -> count}
    std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> title_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> site_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> language_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> location_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> person_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> organization_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> author_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> country_index;
};

class ParseEngine {
   public:
    /*!
     * @warning The optional `stop_words_ptr` pointer parameter, if supplied to this function, must outlive this functions entire execution.
     * @param file_path The file path of the file or folder of files you desire to parse and fill a RunTimeDataBase object with.
     * @param stop_words_ptr An optional parameter that is a constant pointer to an unordered_set of stop words.
     */
    virtual void Parse(std::string file_path, const std::unordered_set<std::string>* stop_words_ptr = NULL) = 0;

    /*!
     * @brief Returns the RunTimeDataBase owned by the invoked ParseEngine object.
     * @warning The return value should not be deleted, and the use of the return value should be restricted to the lifetime of the invoked ParseEngine object.
     */
    virtual inline const RunTimeDataBase* GetRunTimeDatabase() const = 0;
};

}  // namespace parse_util

/// @brief The `KaggleFinanceParseEngine` class should be used to parse the data found at https://www.kaggle.com/datasets/jeet2016/us-financial-news-articles
class KaggleFinanceParseEngine : public parse_util::ParseEngine {
   public:
    explicit KaggleFinanceParseEngine(size_t parse_amount, size_t fill_amount);
    void Parse(std::string file_path, const std::unordered_set<std::string>* stop_words = NULL) override;
    inline const parse_util::RunTimeDataBase* GetRunTimeDatabase() const override { return &database_; };

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
    static void* ParsingThreadFunc(void* _arg);     // producer
    static void* ArbitratorThreadFunc(void* _arg);  // consumer and producer
    static void* FillingThreadFunc(void* _arg);     // consumer

    parse_util::RunTimeDataBase database_;
    std::vector<std::pair<std::string, std::unordered_map<std::string, uint32_t>>> unformatted_database_;
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

#endif  // SEARCH_ENGINE_PROJECT_INDEXENGINE_H_