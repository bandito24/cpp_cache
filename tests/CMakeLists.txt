include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.8.1
)

FetchContent_Declare(
  fakeit
  GIT_REPOSITORY https://github.com/eranpeer/FakeIt.git
  GIT_TAG master
)

FetchContent_MakeAvailable(Catch2 fakeit)

file(GLOB TEST_FILES ./*.cpp)
add_executable(tests ${TEST_FILES})

target_link_libraries(tests PRIVATE SRC Catch2::Catch2WithMain)

target_include_directories(tests PRIVATE
    ${fakeit_SOURCE_DIR}/single_header/catch
    ${PROJECT_SOURCE_DIR}/include
)

message(STATUS "fakeit_SOURCE_DIR = ${fakeit_SOURCE_DIR}")
target_compile_definitions(tests PRIVATE
    PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)

include(CTest)
include(Catch)

