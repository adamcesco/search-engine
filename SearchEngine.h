#ifndef SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_
#define SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_

#include <memory>
#include <optional>

#include "ParseEngine.h"

namespace search_engine {

class SearchEngine {
   public:
    SearchEngine(std::unique_ptr<parse_util::RunTimeDataBase>&& dep_inj_ptr) : database_ptr_{std::forward<std::unique_ptr<parse_util::RunTimeDataBase>>(dep_inj_ptr)} {}

    void DisplayConsoleUserInterface(std::optional<std::string> shortcut);

    std::unordered_set<std::string> HandleQuery(std::string query);

   private:

    std::unique_ptr<parse_util::RunTimeDataBase> database_ptr_;
};

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_