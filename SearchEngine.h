#ifndef SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_
#define SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_

#include <memory>
#include <optional>
#include <string_view>

#include "ParseEngine.h"

namespace search_engine {

template <typename T, typename U>
class SearchEngine {
   public:
    SearchEngine(std::unique_ptr<parse_util::RunTimeDatabase<T, U>>&& dep_inj_ptr) : database_ptr_{std::forward<std::unique_ptr<parse_util::RunTimeDatabase<T, U>>>(dep_inj_ptr)} {}
    void DisplayConsoleUserInterface(std::optional<std::string> shortcut = std::nullopt);

   private:
    std::unordered_set<std::string> HandleQuery(std::string query);

    std::unique_ptr<parse_util::RunTimeDatabase<T, U>> database_ptr_;
};

template <typename T, typename U>
void SearchEngine<T, U>::DisplayConsoleUserInterface(std::optional<std::string> shortcut) {
    // todo: implement function
}

template <typename T, typename U>
std::unordered_set<std::string> SearchEngine<T, U>::HandleQuery(std::string query) {
    // todo: implement function
    return std::unordered_set<std::string>();
}

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_