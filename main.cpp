#include <iostream>

#include "KaggleFinanceParseEngine.h"
#include "SearchEngine.h"

int main(int argc, char** argv) {
    search_engine::KaggleFinanceParseEngine parseEngine(4, 3);
    parseEngine.ParseData("../full_kaggle_finance_data");
    auto database_ptr = parseEngine.GetRunTimeDatabase();
    if(argc < 2 || std::string(argv[1]) == "time") {
        return 0;
    }
    if (std::string(argv[1]) == "print") {
        // print contents of database
        std::cout << "value_index: " << std::endl;
        for (auto&& map : database_ptr->value_index) {
            for (auto&& pair : map) {
                std::cout << pair.first << " -> " << std::endl;
                for (auto&& pair2 : pair.second) {
                    std::cout << "\t" << pair2.first << " -> " << pair2.second << std::endl;
                }
            }
        }
        return 0;
    }
    if (std::string(argv[1]) == "search") {
        // search database
        search_engine::SearchEngine<size_t, size_t, std::string> searchEngine(std::make_unique<search_engine::KaggleFinanceParseEngine>(parseEngine));
        searchEngine.DisplayConsoleUserInterface();
        return 0;
    }
    return 0;
}
