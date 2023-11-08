add_library(tilde STATIC
    tilde.c
)
target_include_directories(tilde BEFORE PUBLIC
    .
)
