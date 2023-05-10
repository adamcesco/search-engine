#include <iostream>

#include "KaggleFinanceParseEngine.h"
#include "SearchEngine.h"

int main(int argc, char** argv) {
    search_engine::KaggleFinanceParseEngine parseEngine(4, 3);
    parseEngine.ParseData("../sample_kaggle_finance_data");
    auto database_ptr = parseEngine.GetRunTimeDatabase();
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