#include <iostream>

#include "KaggleFinanceSourceEngine.h"
#include "SearchEngine.h"

int main(int argc, char** argv) {
    search_engine::KaggleFinanceEngine parseEngine(6, 4);
    auto parse_engine_ptr = parseEngine.GetRunTimeDatabase();
    if(argc > 2 && std::string(argv[1]) == "time") {
        parseEngine.ParseSources(argv[2]);
        return 0;
    }
    if (argc > 2 && std::string(argv[1]) == "print") {
        // print contents of database
        parseEngine.ParseSources(argv[2]);
        std::cout << "value_index: " << std::endl;
        for (auto&& map : parse_engine_ptr->value_index) {
            for (auto&& pair : map) {
                std::cout << pair.first << " -> " << std::endl;
                for (auto&& pair2 : pair.second) {
                    std::cout << "\t" << pair2.first << " -> " << pair2.second << std::endl;
                }
            }
        }
        return 0;
    }
    if (argc > 1 && std::string(argv[1]) == "search") {
        // search database
        search_engine::SearchEngine<size_t, size_t, std::string> searchEngine(std::make_unique<search_engine::KaggleFinanceEngine>(parseEngine));
        if (argc < 3) {
            searchEngine.DisplayConsoleUserInterface();
        } else {
            searchEngine.DisplayConsoleUserInterface(std::string(argv[2]));
        }
        return 0;
    }
    return 0;
}
