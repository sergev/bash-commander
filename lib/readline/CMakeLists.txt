add_library(readline STATIC
    readline.c
    vi_mode.c
    funmap.c
    keymaps.c
    parens.c
    search.c
    rltty.c
    complete.c
    bind.c
    isearch.c
    display.c
    signals.c
    util.c
    kill.c
    undo.c
    macro.c
    input.c
    callback.c
    terminal.c
    text.c
    nls.c
    misc.c
    history.c
    histexpand.c
    histfile.c
    histsearch.c
    shell.c
    savestring.c
    mbutil.c
    tilde.c
    colors.c
    parse-colors.c
    xmalloc.c
    xfree.c
    compat.c
)
target_include_directories(readline BEFORE PUBLIC
    .
)

#add_library(history STATIC
#    history.c
#    histexpand.c
#    histfile.c
#    histsearch.c
#    shell.c
#    savestring.c
#    mbutil.c
#    xmalloc.c
#    xfree.c
#)
#target_include_directories(history BEFORE PUBLIC
#    .
#)
