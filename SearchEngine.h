#ifndef SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_
#define SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_

#include <memory>
#include <optional>

#include "SourceEngine.h"

namespace search_engine {

template <typename T, typename U, typename V = U>
class SearchEngine {
   public:
    SearchEngine(std::unique_ptr<parse_util::SourceEngine<T, U, V>>&& dep_inj_ptr) : source_engine_ptr_{std::forward<std::unique_ptr<parse_util::SourceEngine<T, U, V>>>(dep_inj_ptr)} {}
    void DisplayConsoleUserInterface(std::optional<std::string> shortcut = std::nullopt);

   private:
    /*!
     * @brief Handles a query and returns a vector containing a set of the filepaths of the sources that match the query.
     * @param query The query to be handled.
     * @return A vector containing a set of the filepaths of the sources that match the query.
     */
    std::vector<std::string> HandleQuery(std::string query);

    std::unique_ptr<parse_util::SourceEngine<T, U, V>> source_engine_ptr_;
};

template <typename T, typename U, typename V>
void SearchEngine<T, U, V>::DisplayConsoleUserInterface(std::optional<std::string> shortcut) {
    std::string input;
    if (shortcut.has_value()) {
        input = shortcut.value();
    } else {
        std::cout << "Welcome to the search engine!" << std::endl;
        std::cout << "Please type 'query' to enter a query, type 'parse' to parse data sources, or type 'exit' to quit." << std::endl;
        std::cout << ">> ";
        std::getline(std::cin, input);
    }

    while (input != "exit") {
        while (input == "query") {
            std::cout << "Please enter your query: ";
            std::getline(std::cin, input);
            std::vector<std::string> results = std::move(this->HandleQuery(input));
            while (true) {
                size_t result_index = 0;
                std::cout << "Results: " << std::endl;
                for (auto&& result : results) {
                    std::cout << result_index++ << "\t";
                    this->source_engine_ptr_->DisplaySource(result, true);
                }
                size_t result_number = std::string::npos;
                std::cout << std::endl
                          << "Please type `see {result_number}` to see the result, type `query` to enter another query, or type 'main' to return to the main menu." << std::endl;
                std::cout << ">> ";
                std::getline(std::cin, input);
                if (input == "query" || input == "main") {
                    break;
                } else if (input.size() > 4 && input.substr(0, 3) == "see" && (std::stringstream(input.substr(4)) >> result_number) && result_number < results.size()) {
                    this->source_engine_ptr_->DisplaySource(results[result_number], false);
                } else {
                    std::cout << "Invalid input. Please try again." << std::endl;
                }
            }
            if (input == "main") {
                break;
            }
        }
        if (input == "parse") {
            std::cout << "Please enter the path to the data you would like to parse: ";
            std::getline(std::cin, input);
            this->source_engine_ptr_->ParseSources(input);
        } else {
            std::cout << "Invalid input. Please try again." << std::endl;
        }

        std::cout << "Please type 'query' to enter a query, type 'parse' to parse data sources, or type 'exit' to quit." << std::endl;
        std::cout << ">> ";
        std::getline(std::cin, input);
    }
}

template <typename T, typename U, typename V>
std::vector<std::string> SearchEngine<T, U, V>::HandleQuery(std::string query) {
    // todo: implement function
    std::vector<std::string> results;
    for (const auto& word_map : this->source_engine_ptr_->GetRunTimeDatabase()->value_index) {
        try {
            for (const auto& source_item : word_map.at(this->source_engine_ptr_->CleanValue(query.data()))) {
                results.push_back(this->source_engine_ptr_->GetRunTimeDatabase()->id_map.at(source_item.first));
            }
        } catch (const std::out_of_range& e) {
            continue;
        }
    }
    return results;
}

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_