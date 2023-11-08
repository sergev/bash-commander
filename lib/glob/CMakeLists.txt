add_library(glob STATIC
    ${CMAKE_SOURCE_DIR}/bash/lib/glob/glob.c
    ${CMAKE_SOURCE_DIR}/bash/lib/glob/strmatch.c
    ${CMAKE_SOURCE_DIR}/bash/lib/glob/smatch.c
    ${CMAKE_SOURCE_DIR}/bash/lib/glob/xmbsrtowcs.c
    ${CMAKE_SOURCE_DIR}/bash/lib/glob/gmisc.c
)
target_include_directories(glob BEFORE PUBLIC
    ${CMAKE_SOURCE_DIR}/bash/lib/glob
)
