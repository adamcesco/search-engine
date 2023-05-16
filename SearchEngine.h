#ifndef SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_
#define SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_

#include <memory>
#include <optional>
#include <regex>

#include "SourceEngine.h"

namespace search_engine {

template <typename T, typename U, typename V = U>
class SearchEngine {
   public:
    SearchEngine(std::unique_ptr<source_util::SourceEngine<T, U, V>>&& dep_inj_ptr) : source_engine_ptr_{std::forward<std::unique_ptr<source_util::SourceEngine<T, U, V>>>(dep_inj_ptr)} {}

    void InitCommandLineInterface(std::optional<std::string> shortcut = std::nullopt);

    /*!
     * @brief Handles a query and returns a vector containing a set of the filepaths of the sources that match the query.
     * @param query The query to be handled.
     * @return A vector containing a set of the filepaths of the sources that match the query.
     */
    std::vector<std::string> HandleQuery(std::string query);

   private:
    std::unique_ptr<source_util::SourceEngine<T, U, V>> source_engine_ptr_;
};

template <typename T, typename U, typename V>
void SearchEngine<T, U, V>::InitCommandLineInterface(std::optional<std::string> shortcut) {
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
                    if (result_index == 10) {
                        break;
                    }
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
            this->source_engine_ptr_->ClearRunTimeDatabase();
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
    std::vector<std::string> results;
    std::regex category_pattern(R"(((?:(?:values)|(?:titles)|(?:sites)|(?:langs)|(?:locations)|(?:people)|(?:orgs)|(?:authors)|(?:countries))[\s]{0,1}:[\s]{0,1}\{[^\{\}]+\}))");
    for (std::regex_iterator<std::string::iterator> it(query.begin(), query.end(), category_pattern); it != std::regex_iterator<std::string::iterator>(); ++it) {
        std::string category_match = std::move(it->str());
        int64_t category_hash = category_match[0] + (category_match[1] * 2);
        std::regex arg_pattern("\"((?:\\\\\"|[^\"])+)\"|([^, \\{\\}]+)");  // states that the user must seperate the arguments with a comma and/or a space, and that the arguments can't contain a comma, space, or curly brace.
                                                                           // stats that the user can enter in a string with commas and/or spaces in it by surrounding the string with double quotes.

        for (std::regex_iterator<std::string::iterator> it2(category_match.begin(), category_match.end(), arg_pattern); it2 != std::regex_iterator<std::string::iterator>(); ++it2) {
            std::string arg_match = std::move(it2->str());
            
            if(arg_match.size() <= 2) {
                std::cout << "Invalid term size. The following term was skipped: " << arg_match << std::endl;
                continue;
            }
            
            bool has_front_quote = arg_match.front() == '\"';
            bool has_back_quote = arg_match.back() == '\"';
            bool back_quote_esc = has_back_quote == true && arg_match[arg_match.size() - 2] == '\\';
            if ((has_front_quote == true && (has_back_quote == false || back_quote_esc == true)) || (has_front_quote == false && (has_back_quote == true && back_quote_esc == false))) {
                std::cout << "Invalid quote matching. The following term was skipped: " << arg_match << std::endl;
                continue;
            }

            if (has_front_quote == true && has_back_quote == true && back_quote_esc == false) {
                arg_match = std::move(arg_match.substr(1, arg_match.size() - 2));
            }

            std::cout << arg_match << std::endl;
        }
    }
    return {};
}

}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_SEARCHENGINE_H_