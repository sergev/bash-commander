add_library(tilde STATIC
    ${CMAKE_SOURCE_DIR}/bash/lib/tilde/tilde.c
)
target_include_directories(tilde BEFORE PUBLIC
    ${CMAKE_SOURCE_DIR}/bash/lib/tilde
)
