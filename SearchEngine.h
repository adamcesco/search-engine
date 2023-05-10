#ifndef SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_
#define SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_

#include <memory>
#include <optional>
#include <string_view>

#include "ParseEngine.h"

namespace search_engine {

template <typename T, typename U, typename V = U>
class SearchEngine {
   public:
    SearchEngine(std::unique_ptr<parse_util::ParseEngine<T, U, V>>&& dep_inj_ptr) : parse_engine_ptr_{std::forward<std::unique_ptr<parse_util::ParseEngine<T, U, V>>>(dep_inj_ptr)} {}
    void DisplayConsoleUserInterface(std::optional<std::string> shortcut = std::nullopt);

   private:
    std::unordered_set<std::string> HandleQuery(std::string query);

    std::unique_ptr<parse_util::ParseEngine<T, U, V>> parse_engine_ptr_;
};

template <typename T, typename U, typename V>
void SearchEngine<T, U, V>::DisplayConsoleUserInterface(std::optional<std::string> shortcut) {
    // todo: implement function
}

template <typename T, typename U, typename V>
std::unordered_set<std::string> SearchEngine<T, U, V>::HandleQuery(std::string query) {
    // todo: implement function
    return std::unordered_set<std::string>();
}

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_