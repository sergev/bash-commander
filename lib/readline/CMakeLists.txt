add_library(readline STATIC
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/readline.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/vi_mode.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/funmap.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/keymaps.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/parens.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/search.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/rltty.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/complete.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/bind.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/isearch.c
    display.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/signals.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/util.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/kill.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/undo.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/macro.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/input.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/callback.c
    terminal.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/text.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/nls.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/misc.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/history.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/histexpand.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/histfile.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/histsearch.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/savestring.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/mbutil.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/tilde.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/colors.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/parse-colors.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/xmalloc.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/xfree.c
    ${CMAKE_SOURCE_DIR}/bash/lib/readline/compat.c
)
target_include_directories(readline BEFORE PUBLIC
    ${CMAKE_SOURCE_DIR}/bash/lib/readline
)
