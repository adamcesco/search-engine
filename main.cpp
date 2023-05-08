#include <iostream>

#include "SearchEngine.h"
/*
note that your kaggle parser does not support unicode characters
*/

int main(int argc, char** argv) {
    search_engine::KaggleFinanceParseEngine parseEngine(8, 8);
    parseEngine.Parse("../full_kaggle_finance_data");
}