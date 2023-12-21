# search-engine

## Solution Details

- Uses the POSIX library to optimize file parsing speeds. This allowed for the implementation of load balancing, and it allows users to insert a custom amount of threads to be used for parsing files and filling the run-time database.
- Thoruhgout the implementation of this solution, we follow [Google's C++ Coding & Testing Standards](https://google.github.io/styleguide/cppguide.html).
- This project was implemented to explore how multithreading is implemented and how lower-level C++ tactics affect the complexity and optimization of a project. We implemented things like the "produce-consumer" paradigm and move operation logic. This has expanded our knowledge on lower-level C++ and how to use multithreading in practice.

## Instructions for Demo Use

### General

- This demo can only be run on a linux-based system or sub-system. This is becuase this demo uses the POSIX library for multithreading in C++.
- This demo is purely run in the terminal.

### boost program options

- Example command-line arguments: `./build/search-engine-project --pt 2 --ft 2 --ui`

| Explanation                                                                | Command-line Flag   | Default Values
|----------------------------------------------------------------------------|---------------------|---------------------------------------------------|
| Opens help menu                                                            | help                |                                                   |
| Sets the path of the files to be parsed                                    | path                |    default value = ../sample_kaggle_finance_data  |
| Sets the number of threads that will be used to parse the dataset          | parser-threads, pt  |    default value = 1                              |
| Sets the number of threads that will be used to fill the run-time database | filler-threads, ft  |    default value = 1                              |
| Prints the run-time database to the console                                | print-database, pd  |                                                   |
| Opens the search console option that allows the user to enter a query      | search, s           |                                                   |
| Opens the default user interface console option                            | ui                  |                                                   |

### query formatting

| Query Format                                             |  Example                                      |
|----------------------------------------------------------|-----------------------------------------------|
| `category1: term1 term2`                                 | `values: german income`                       |
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
