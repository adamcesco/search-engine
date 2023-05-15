cmake_minimum_required(VERSION 3.0.0)
project(search-engine-project VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
include(CTest)
enable_testing()

add_executable(search-engine-project main.cpp KaggleFinanceSourceEngine.cpp)

find_package(Boost COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(search-engine-project ${Boost_LIBRARIES})


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
