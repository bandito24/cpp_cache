cmake_minimum_required(VERSION 3.24)
project(proxy_proj LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 20)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)
add_subdirectory(tests)

#target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

#External libraries
find_package(CURL REQUIRED)
add_library(sqlite STATIC ${PROJECT_SOURCE_DIR}/external/sqlite/sqlite3.c)
target_include_directories(sqlite PUBLIC
      ${PROJECT_SOURCE_DIR}/external/sqlite)
add_library(ada STATIC ${PROJECT_SOURCE_DIR}/external/ada/ada.cpp)
target_include_directories(ada PUBLIC
      ${PROJECT_SOURCE_DIR}/external/ada)
target_link_libraries(SRC PRIVATE sqlite CURL::libcurl ada)


add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE SRC ada)
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/include)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

