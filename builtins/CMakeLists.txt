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
    ${CMAKE_SOURCE_DIR}/bash/builtins/bashgetopt.c
    caller.c
    kill.c
    let.c
    pushd.c
    times.c
    enable.c
    wait.c
    colon.c
    fc.c
    ${CMAKE_SOURCE_DIR}/bash/builtins/evalfile.c
    source.c
    test.c
    return.c
    echo.c
    ${CMAKE_SOURCE_DIR}/bash/builtins/getopt.c
    getopts.c
    shift.c
    builtins.c
    setattr.c
    printf.c
    history.c
    ${CMAKE_SOURCE_DIR}/bash/builtins/common.c
    eval.c
    exec.c
    exit.c
    break.c
    command.c
    ${CMAKE_SOURCE_DIR}/bash/builtins/evalstring.c
    bind.c
    suspend.c
    read.c
    hash.c
    mapfile.c
    set.c
    shopt.c
    pipesize.h
)
target_include_directories(builtins BEFORE PUBLIC
    .
    ${CMAKE_SOURCE_DIR}/bash/builtins
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(mkbuiltins ${CMAKE_SOURCE_DIR}/bash/builtins/mkbuiltins.c)
add_executable(psize.aux ${CMAKE_SOURCE_DIR}/bash/builtins/psize.c)

set(DEFS
    ${CMAKE_SOURCE_DIR}/bash/builtins/alias.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/bind.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/break.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/builtin.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/caller.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/cd.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/colon.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/command.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/declare.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/echo.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/enable.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/eval.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/getopts.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/exec.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/exit.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/fc.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/fg_bg.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/hash.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/help.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/history.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/jobs.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/kill.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/let.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/read.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/return.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/set.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/setattr.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/shift.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/source.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/suspend.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/test.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/times.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/trap.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/type.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/ulimit.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/umask.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/wait.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/reserved.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/pushd.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/shopt.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/printf.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/complete.def
    ${CMAKE_SOURCE_DIR}/bash/builtins/mapfile.def
)

add_custom_command(
    OUTPUT builtins.c builtext.h
    COMMAND ./mkbuiltins -externfile builtext.h -structfile builtins.c ${DEFS}
    DEPENDS mkbuiltins ${DEFS}
)

# Technically this is wrong; the pipe size should be for the target system,
# not the build host.
add_custom_command(
    OUTPUT pipesize.h
    COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/bash/builtins/psize.sh > pipesize.h
    DEPENDS psize.aux
)

function(MAKE_BUILTIN NAME)
    add_custom_command(
        OUTPUT ${NAME}.c
        COMMAND ./mkbuiltins -D . ${CMAKE_SOURCE_DIR}/bash/builtins/${NAME}.def
        DEPENDS mkbuiltins ${CMAKE_SOURCE_DIR}/bash/builtins/${NAME}.def
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
