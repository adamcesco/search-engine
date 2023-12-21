# search-engine

## Instructions for Demo Use

### boost program options

| Explanation                                                                | Command-line Flag   | Default Values
|----------------------------------------------------------------------------|---------------------|---------------------------------------------------|
| Help flag                                                                  | help                |                                                   |
| set the path of the files to be parsed                                     | path                |    default value = ../sample_kaggle_finance_data  |
| set the number of threads that will be used to parse the dataset           | parser-threads, pt  |    default value = 1                              |
| set the number of threads that will be used to fill the run-time database  | filler-threads, ft  |    default value = 1                              |
| print the run-time database to the console                                 | print-database, pd  |                                                   |
| open the search console option that allows the user to enter a query       | search, s           |                                                   |
| open the default user interface console option                             | ui                  |                                                   |

### query formatting

| Query Format                                             |  Example                                      |
|----------------------------------------------------------|-----------------------------------------------|
| `category1: term1 term2`                                 | `values: german`                              |
| `category1: "term1 term2"`                               | `people: "eaton vance"`                       |
| `category1: term1 term2 \| category2: term3 term4 term5` | `values: german income \| title: funds euro`  |

- Valid categories:
  - values
  - title
  - sites
  - langs
  - locations
  - people
  - orgs
  - authors
  - countries
- A term can be any string, and if the term has a space within it, it must be wrapped in quotation marks.
- You can have as many categories as you want, but they must be separated by a '|' character.
