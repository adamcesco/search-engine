#include <boost/program_options.hpp>
#include <iostream>

#include "KaggleFinanceSourceEngine.h"
#include "SearchEngine.h"

int main(int argc, char** argv) {
    std::string path;
    int64_t parser_thread_count;
    int64_t filler_thread_count;
    try {
        boost::program_options::options_description desc("Options");
        desc.add_options()
            /*help flag   */ ("help", "Help screen")
            /*path flag   */ ("path", boost::program_options::value<std::string>(&path)->default_value("../sample_kaggle_finance_data"), "Sets the path to the file or folder of files you wish to parse.")
            /*thread flag */ ("parser-threads,pt", boost::program_options::value<int64_t>(&parser_thread_count)->default_value(1), "Sets the number of threads to be used to parse the given file or folder of files.")
            /*thread flag */ ("filler-threads,ft", boost::program_options::value<int64_t>(&filler_thread_count)->default_value(1), "Sets the number of threads to be used to fill the database while parsing the given file or folder of files.")
            /*print flag  */ ("print-database,pd", "Prints the contents of the database after completely parsing the given file or folder of files.")
            /*search flag */ ("search,s", "Prompts the user to enter a query and then searches the database for the given query.")
            /*ui flag     */ ("ui", "Initializes the command line interface for the search engine.");

        boost::program_options::variables_map vm;
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);
        if (vm.count("help")) {
            std::cout << desc << '\n';
            return 1;
        }

        search_engine::KaggleFinanceEngine source_engine(parser_thread_count, filler_thread_count);
        source_engine.ParseSources(path);
        auto database_ptr = source_engine.GetRunTimeDatabase();
        search_engine::SearchEngine<size_t, size_t, std::string> search_engine(std::make_unique<search_engine::KaggleFinanceEngine>(source_engine));
        if (vm.count("print-database")) {
            std::cout << "value_index: " << std::endl;
            for (auto&& map : database_ptr->value_index) {
                for (auto&& pair : map) {
                    std::cout << pair.first << " -> " << std::endl;
                    for (auto&& pair2 : pair.second) {
                        std::cout << "\t" << pair2.first << " -> " << pair2.second << std::endl;
                    }
                }
            }
        }
        if (vm.count("search")) {
            std::string query;
            std::cout << "Enter a query: ";
            std::getline(std::cin, query);
            std::cout << "Results for query: " << query << std::endl;
            auto results = search_engine.HandleQuery(query);
            for (auto&& result : results) {
                std::cout << "\t" << result << std::endl;
            }
        }
        if (vm.count("ui")) {
            search_engine.InitCommandLineInterface();
            return 0;
        }
    } catch (const boost::program_options::error& ex) {
        std::cerr << ex.what() << '\n';
    }

    return 0;
}
