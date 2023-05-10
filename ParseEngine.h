#ifndef SEARCH_ENGINE_PROJECT_INDEXENGINE_H_
#define SEARCH_ENGINE_PROJECT_INDEXENGINE_H_

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace search_engine {

namespace parse_util {

/*!
 * @brief A struct that contains all of the indexes that are used to store the data parsed from a file by a ParseEngine object.
 * @tparam T The data type you wish to use to store the ID of each source.
 * @tparam U The data type you wish to use to store the values to be indexed.
 * @tparam V The data type you wish to use to store the meta-data of each source. This template parameter is optional, and defaults to the same type as the U template parameter.
 * @warning The use of this struct is not recommended outside of a child class of the ParseEngine class.
 */
template <typename T, typename U, typename V = U>
struct RunTimeDatabase {
    std::unordered_map<T, std::string> id_map;                                       // uuid -> file path
    std::vector<std::unordered_map<U, std::unordered_map<T, uint32_t>>> text_index;  // word -> list of {uuid -> count}
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
class ParseEngine {
   public:
    /*!
     * @warning The optional `stop_words_ptr` pointer parameter, if supplied to this function, must outlive this functions entire execution.
     * @param file_path The file path of the file or folder of files you desire to parse and fill a RunTimeDatabase object with.
     * @param stop_words_ptr An optional parameter that is a constant pointer to an unordered_set of stop words.
     */
    virtual void ParseData(std::string file_path, const std::unordered_set<U>* const stop_words_ptr = NULL) = 0;

    virtual T CleanID(const char* const id, std::optional<size_t> size = std::nullopt) = 0;
    virtual U CleanValue(const char* const token, std::optional<size_t> size = std::nullopt) = 0;
    virtual V CleanMetaData(const char* const token, std::optional<size_t> size = std::nullopt) = 0;

    /*!
     * @brief Returns the RunTimeDatabase owned by the invoked ParseEngine object.
     * @warning The return value should not be deleted, and the use of the return value should be restricted to the lifetime of the invoked ParseEngine object.
     */
    virtual inline const RunTimeDatabase<T, U, V>* const GetRunTimeDatabase() const = 0;

    virtual ~ParseEngine() = default;
};

}  // namespace parse_util
}  // namespace search_engine

#endif  // SEARCH_ENGINE_PROJECT_INDEXENGINE_H_