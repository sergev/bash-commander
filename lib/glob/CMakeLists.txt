add_library(glob STATIC
    glob.c
    strmatch.c
    smatch.c
    xmbsrtowcs.c
    gmisc.c
)
target_include_directories(glob BEFORE PUBLIC
    .
)
