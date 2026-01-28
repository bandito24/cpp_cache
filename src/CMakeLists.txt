

set(SOURCE_FILES
db.cpp
http_client.cpp
)
add_library(SRC ${SOURCE_FILES})
target_include_directories(SRC PUBLIC ${PROJECT_SOURCE_DIR}/include)



target_compile_definitions(SRC PUBLIC
    PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)
#target_link_libraries(${PROJECT_NAME} PRIVATE CORE)
