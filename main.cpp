#include <iostream>

#include "SearchEngine.h"

int main(int argc, char** argv) {
    search_engine::KaggleFinanceParseEngine parseEngine(8, 8);
    parseEngine.Parse("/users7/cse/acescobedo/Projects/search-engine/full_kaggle_finance_data");
    auto database_ptr = parseEngine.GetRunTimeDatabase();
    //print contents of database
    // std::cout << "text_index: " << std::endl;
    // for (auto&& map : database_ptr->text_index) {
    //     for (auto&& pair : map) {
    //         std::cout << pair.first << " -> " << std::endl;
    //         for (auto&& pair2 : pair.second) {
    //             std::cout << "\t" << pair2.first << " -> " << pair2.second << std::endl;
    //         }
    //     }
    // }
    
    return 0;
}