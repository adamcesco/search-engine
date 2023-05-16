#ifndef SEARCH_ENGINE_PROJECT_SOURCEENGINE_H_
#define SEARCH_ENGINE_PROJECT_SOURCEENGINE_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace search_engine {

namespace source_util {

/*!
 * @brief A struct that contains all of the indexes that are used to store the data parsed from a file by a SourceEngine object.
 * @tparam T The data type you wish to use to store the ID of each source.
 * @tparam U The data type you wish to use to store the values to be indexed and the titles of the sources to be indexed.
 * @tparam V The data type you wish to use to store the meta-data of each source. This template parameter is optional, and defaults to the same type as the U template parameter.
 * @warning Not all of the indexes in this struct are guaranteed to be filled by a SourceEngine object. For example, a SourceEngine object that only parses files only containing test will not fill the site_index, language_index, location_index, person_index, organization_index, author_index, or country_index indexes.
 */
template <typename T, typename U, typename V = U>
struct RunTimeDatabase {
    std::unordered_map<T, std::string> id_map;                                        // uuid -> file path
    std::vector<std::unordered_map<U, std::unordered_map<T, uint32_t>>> value_index;  // word -> list of {uuid -> count}
    std::unordered_map<U, std::unordered_map<T, uint32_t>> title_index;
    std::unordered_map<V, std::unordered_set<T>> site_index;
    std::unordered_map<V, std::unordered_set<T>> language_index;
    std::unordered_map<V, std::unordered_set<T>> location_index;
    std::unordered_map<V, std::unordered_set<T>> person_index;
    std::unordered_map<V, std::unordered_set<T>> organization_index;
    std::unordered_map<V, std::unordered_set<T>> author_index;
    std::unordered_map<V, std::unordered_set<T>> country_index;
};

/*!
 * @tparam T The data type you wish to use to store the ID of each source.
 * @tparam U The data type you wish to use to store the values to be indexed.
 * @tparam V The data type you wish to use to store the meta-data of each source.
 */
template <typename T, typename U, typename V = U>
class SourceEngine {
   public:
    /*!
     * @warning The optional `stop_words_ptr` pointer parameter, if supplied to this function, must outlive this functions entire execution.
     * @param path The file path of the file or folder of files you desire to parse and fill a RunTimeDatabase object with.
     * @param stop_words_ptr An optional parameter that is a constant pointer to an unordered_set of stop words.
     */
    virtual void ParseSources(std::string path, const std::unordered_set<U>* const stop_words_ptr = NULL) = 0;

    /*!
     * @brief Displays the source with the given file_path to the console.
     * @param file_path The file path of the source you wish to display.
     * @param just_header A boolean parameter that determines whether or not you wish to display just the header of the source, or the entire source.
     */
    virtual void DisplaySource(std::string file_path, bool just_header) = 0;

    virtual inline void ClearRunTimeDatabase() = 0;
    
    /*!
     * @brief Cleans the given char* id_token and returns the cleaned id_token in the T data type. This function should be used when parsing a file to clean the id of a source, and it should be used when querying the RunTimeDatabase object.
     * @param id_token The char* id_token to be cleaned.
     * @param size The size of the char* id_token to be cleaned. This parameter is optional, and defaults to std::nullopt.
     * @return The cleaned id_token in the T data type.
     */
    virtual T CleanID(const char* const id_token, std::optional<size_t> size = std::nullopt) = 0;

    /*!
     * @brief Cleans the given char* value_token and returns the cleaned value_token in the U data type. This function should be used when parsing a file to clean the value tokens in a source, and it should be used when querying the RunTimeDatabase object.
     * @param value_token The char* value_token to be cleaned.
     * @param size The size of the char* value_token to be cleaned. This parameter is optional, and defaults to std::nullopt.
     * @return The cleaned value_token in the U data type.
     */
    virtual U CleanValue(const char* const value_token, std::optional<size_t> size = std::nullopt) = 0;

    /*!
     * @brief Cleans the given char* metadata_token and returns the cleaned metadata_token in the V data type. This function should be used when parsing a file to clean then metadata tokens in a source, and it should be used when querying the RunTimeDatabase object.
     * @param metadata_token The char* metadata_token to be cleaned.
     * @param size The size of the char* metadata_token to be cleaned. This parameter is optional, and defaults to std::nullopt.
     * @return The cleaned metadata_token in the V data type.
     */
    virtual V CleanMetaData(const char* const metadata_token, std::optional<size_t> size = std::nullopt) = 0;

    /*!
     * @brief Returns the RunTimeDatabase owned by the invoked SourceEngine object.
     * @warning The return value should not be deleted, and the use of the return value should be restricted to the lifetime of the invoked SourceEngine object.
     */
    virtual inline const RunTimeDatabase<T, U, V>* const GetRunTimeDatabase() const = 0;


    virtual ~SourceEngine() = default;
};

}  // namespace source_util
}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_SOURCEENGINE_H_