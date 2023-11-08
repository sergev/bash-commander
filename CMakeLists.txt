cmake_minimum_required(VERSION 3.15)

project(bashc VERSION 4.2 DESCRIPTION "Bash Commander")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
#TODO: add_compile_options(-Wall -Werror -Wshadow)
add_compile_options(-Wno-deprecated-non-prototype -Wno-parentheses)

add_definitions(-DCONF_MACHTYPE="${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_C_COMPILER_ID}-${CMAKE_SYSTEM}")
add_definitions(-DCONF_HOSTTYPE="${CMAKE_SYSTEM_PROCESSOR}")
add_definitions(-DCONF_VENDOR="${CMAKE_C_COMPILER_ID}")
add_definitions(-DCONF_OSTYPE="${CMAKE_SYSTEM}")
add_definitions(-DLOCALEDIR="/usr/local/share/locale")
add_definitions(-DPACKAGE="bashc")
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DSHELL)
add_definitions(-DCOMMANDER)

add_executable(${PROJECT_NAME}
    jobs.c
    error.c
    trap.c
    copy_cmd.c
    input.c
    mailcheck.c
    shell.c
    version.c
    dispose_cmd.c
    syntax.c
    xmalloc.c
    alias.c
    pathexp.c
    locale.c
    commander.c
    execute_cmd.c
    pcomplete.c
    general.c
    flags.c
    test.c
    subst.c
    braces.c
    findcmd.c
    expr.c
    assoc.c
    bashhist.c
    arrayfunc.c
    list.c
    make_cmd.c
    sig.c
    eval.c
    pcomplib.c
    array.c
    bashline.c
    hashcmd.c
    y.tab.c
    variables.c
    unwind_prot.c
    hashlib.c
    print_cmd.c
    stringlib.c
    bracecomp.c
    redir.c
)
target_link_libraries(${PROJECT_NAME} builtins readline sh glob tilde ncurses)
include_directories(. include lib ${CMAKE_BINARY_DIR})

install(TARGETS ${PROJECT_NAME}
    DESTINATION bin
)
#install(FILES deco.1 deco-ru.1
#    DESTINATION share/man/man1
#)
#install(FILES menu profile
#    DESTINATION lib/deco
#)
#install(FILES help/e.general help/e.edit help/e.view help/e.hexview
#    DESTINATION lib/deco/help
#)

add_executable(mksyntax mksyntax.c)
add_executable(mksignames
    support/mksignames.c
    support/signames.c
)
add_executable(bashversion
    support/bashversion.c
    version.c
    version.h
)

add_subdirectory(builtins)
add_subdirectory(lib/readline)
add_subdirectory(lib/sh)
add_subdirectory(lib/glob)
add_subdirectory(lib/tilde)

add_custom_command(
    OUTPUT syntax.c
    COMMAND ./mksyntax -o syntax.c
    DEPENDS mksyntax ${CMAKE_CURRENT_SOURCE_DIR}/syntax.h
)

add_custom_command(
    OUTPUT version.h
    COMMAND ${CMAKE_SOURCE_DIR}/support/mkversion.sh -b -S ${CMAKE_CURRENT_SOURCE_DIR} -d ${PROJECT_VERSION} -o version.h
    DEPENDS support/mkversion.sh config.h patchlevel.h
)
