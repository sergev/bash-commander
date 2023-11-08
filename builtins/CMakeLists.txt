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
    pipesize.h
)
target_include_directories(builtins BEFORE PUBLIC
    .
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(mkbuiltins mkbuiltins.c)
add_executable(psize.aux psize.c)

set(DEFS
    ${CMAKE_CURRENT_SOURCE_DIR}/alias.def ${CMAKE_CURRENT_SOURCE_DIR}/bind.def
    ${CMAKE_CURRENT_SOURCE_DIR}/break.def ${CMAKE_CURRENT_SOURCE_DIR}/builtin.def
    ${CMAKE_CURRENT_SOURCE_DIR}/caller.def ${CMAKE_CURRENT_SOURCE_DIR}/cd.def ${CMAKE_CURRENT_SOURCE_DIR}/colon.def
    ${CMAKE_CURRENT_SOURCE_DIR}/command.def ${CMAKE_CURRENT_SOURCE_DIR}/declare.def ${CMAKE_CURRENT_SOURCE_DIR}/echo.def
    ${CMAKE_CURRENT_SOURCE_DIR}/enable.def ${CMAKE_CURRENT_SOURCE_DIR}/eval.def ${CMAKE_CURRENT_SOURCE_DIR}/getopts.def
    ${CMAKE_CURRENT_SOURCE_DIR}/exec.def ${CMAKE_CURRENT_SOURCE_DIR}/exit.def ${CMAKE_CURRENT_SOURCE_DIR}/fc.def
    ${CMAKE_CURRENT_SOURCE_DIR}/fg_bg.def ${CMAKE_CURRENT_SOURCE_DIR}/hash.def ${CMAKE_CURRENT_SOURCE_DIR}/help.def
    ${CMAKE_CURRENT_SOURCE_DIR}/history.def ${CMAKE_CURRENT_SOURCE_DIR}/jobs.def ${CMAKE_CURRENT_SOURCE_DIR}/kill.def
    ${CMAKE_CURRENT_SOURCE_DIR}/let.def ${CMAKE_CURRENT_SOURCE_DIR}/read.def ${CMAKE_CURRENT_SOURCE_DIR}/return.def
    ${CMAKE_CURRENT_SOURCE_DIR}/set.def ${CMAKE_CURRENT_SOURCE_DIR}/setattr.def ${CMAKE_CURRENT_SOURCE_DIR}/shift.def
    ${CMAKE_CURRENT_SOURCE_DIR}/source.def ${CMAKE_CURRENT_SOURCE_DIR}/suspend.def ${CMAKE_CURRENT_SOURCE_DIR}/test.def
    ${CMAKE_CURRENT_SOURCE_DIR}/times.def ${CMAKE_CURRENT_SOURCE_DIR}/trap.def ${CMAKE_CURRENT_SOURCE_DIR}/type.def
    ${CMAKE_CURRENT_SOURCE_DIR}/ulimit.def ${CMAKE_CURRENT_SOURCE_DIR}/umask.def ${CMAKE_CURRENT_SOURCE_DIR}/wait.def
    ${CMAKE_CURRENT_SOURCE_DIR}/reserved.def ${CMAKE_CURRENT_SOURCE_DIR}/pushd.def ${CMAKE_CURRENT_SOURCE_DIR}/shopt.def
    ${CMAKE_CURRENT_SOURCE_DIR}/printf.def ${CMAKE_CURRENT_SOURCE_DIR}/complete.def ${CMAKE_CURRENT_SOURCE_DIR}/mapfile.def
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
    COMMAND /bin/sh ${CMAKE_CURRENT_SOURCE_DIR}/psize.sh > pipesize.h
    DEPENDS psize.aux
)

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
