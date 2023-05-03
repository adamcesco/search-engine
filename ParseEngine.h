#ifndef SEARCH_ENGINE_PROJECT_INDEXENGINE_H_
#define SEARCH_ENGINE_PROJECT_INDEXENGINE_H_

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace search_engine {

namespace parse_util {

struct RunTimeDataBase {
    std::unordered_map<std::string, std::string> uuid_map;
    std::unordered_map<std::string, std::unordered_set<std::string>> text_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> site_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> language_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> location_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> people_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> organization_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> author_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> title_index;
    std::unordered_map<std::string, std::unordered_set<std::string>> country_index;
};

class ParseEngine {
   public:
    virtual const void Parse(std::istream& data_file) = 0;

    // the use of the return value should be restricted to the lifetime of the ParseEngine object
    virtual inline const RunTimeDataBase* GetRunTimeDataBase() const = 0;
};

}  // namespace parse_util

// the `KaggleFinanceParseEngine` class will be used to parse the data found at https://www.kaggle.com/datasets/jeet2016/us-financial-news-articles
class KaggleFinanceParseEngine : public parse_util::ParseEngine {
   public:
    const void Parse(std::istream& data_file) override;
    inline const parse_util::RunTimeDataBase* GetRunTimeDataBase() const override { return &database_; };

   private:
    parse_util::RunTimeDataBase database_;
};

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_INDEXENGINE_H_