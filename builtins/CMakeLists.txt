add_library(builtins STATIC
    jobs.c
    type.c
    help.c
    trap.c
    declare.c
    umask.c
    complete.c
    ulimit.c
    cd.c
    fg_bg.c
    alias.c
    builtin.c
    bashgetopt.c
    caller.c
    kill.c
    let.c
    pushd.c
    times.c
    enable.c
    wait.c
    colon.c
    fc.c
    evalfile.c
    source.c
    test.c
    return.c
    echo.c
    getopt.c
    getopts.c
    shift.c
    builtins.c
    setattr.c
    printf.c
    history.c
    common.c
    eval.c
    exec.c
    exit.c
    break.c
    command.c
    evalstring.c
    mkbuiltins.c
    bind.c
    suspend.c
    read.c
    hash.c
    mapfile.c
    set.c
    shopt.c
)
target_include_directories(builtins BEFORE PUBLIC
    .
)

add_executable(mkbuiltins mkbuiltins.c)
#target_include_directories(mkbuiltins PRIVATE ${CMAKE_SOURCE_DIR})

#add_custom_command(
#    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/elftc_version.c
#    COMMAND sed -e 's:ELFTC_VERSION:\"${MLA_VERSION}.${GIT_REVCOUNT}-${GIT_BRANCH}-${GIT_HASH}\":'
#                ${CMAKE_CURRENT_SOURCE_DIR}/libelftc/elftc_version.c
#                > ${CMAKE_CURRENT_BINARY_DIR}/elftc_version.c
#    DEPENDS ${CMAKE_SOURCE_DIR}/.git/HEAD
#            ${CMAKE_SOURCE_DIR}/.git/
#)

function(MAKE_BUILTIN NAME)
    add_custom_command(
        OUTPUT ${NAME}.c
        COMMAND ./mkbuiltins -D . ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.def
        DEPENDS mkbuiltins ${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.def
    )
endfunction()

make_builtin(alias)
make_builtin(bind)
make_builtin(break)
make_builtin(builtin)
make_builtin(caller)
make_builtin(cd)
make_builtin(colon)
make_builtin(command)
make_builtin(declare)
make_builtin(echo)
make_builtin(enable)
make_builtin(eval)
make_builtin(exec)
make_builtin(exit)
make_builtin(fc)
make_builtin(fg_bg)
make_builtin(hash)
make_builtin(help)
make_builtin(history)
make_builtin(jobs)
make_builtin(kill)
make_builtin(let)
make_builtin(mapfile)
make_builtin(printf)
make_builtin(pushd)
make_builtin(read)
make_builtin(return)
make_builtin(set)
make_builtin(setattr)
make_builtin(shift)
make_builtin(shopt)
make_builtin(source)
make_builtin(suspend)
make_builtin(test)
make_builtin(times)
make_builtin(trap)
make_builtin(type)
make_builtin(ulimit)
make_builtin(umask)
make_builtin(wait)
make_builtin(getopts)
make_builtin(reserved)
make_builtin(complete)
