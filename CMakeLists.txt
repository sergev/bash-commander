cmake_minimum_required(VERSION 3.15)

project(bashc VERSION 5.2 DESCRIPTION "Bash Commander")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
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

find_package(BISON 3.0 REQUIRED)
bison_target(parse parse.y parse.c)

add_executable(${PROJECT_NAME}
    bash/jobs.c
    bash/error.c
    bash/trap.c
    bash/copy_cmd.c
    bash/input.c
    bash/mailcheck.c
    bash/shell.c
    bash/version.c
    bash/dispose_cmd.c
    syntax.c
    bash/xmalloc.c
    bash/alias.c
    bash/pathexp.c
    locale.c
    commander.c
    bash/execute_cmd.c
    bash/pcomplete.c
    bash/general.c
    bash/flags.c
    bash/test.c
    bash/subst.c
    bash/braces.c
    bash/findcmd.c
    bash/expr.c
    bash/assoc.c
    bash/bashhist.c
    bash/arrayfunc.c
    bash/list.c
    bash/make_cmd.c
    sig.c
    bash/eval.c
    bash/pcomplib.c
    bash/array.c
    bashline.c
    bash/hashcmd.c
    ${BISON_parse_OUTPUT_SOURCE}
    variables.c
    bash/unwind_prot.c
    bash/hashlib.c
    bash/print_cmd.c
    bash/stringlib.c
    bash/bracecomp.c
    bash/redir.c
    signames.h
)
target_link_libraries(${PROJECT_NAME} builtins readline sh glob tilde ncurses)
include_directories(
    .
    bash
    bash/include
    bash/lib
    ${CMAKE_BINARY_DIR}
)

install(TARGETS ${PROJECT_NAME}
    DESTINATION bin
)
install(FILES
    bash_commander
    bash_dialog
    DESTINATION etc
)

add_executable(mksyntax bash/mksyntax.c)
add_executable(mksignames
    bash/support/mksignames.c
    bash/support/signames.c
)
add_executable(bashversion
    bash/support/bashversion.c
    bash/version.c
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
    DEPENDS mksyntax ${CMAKE_CURRENT_SOURCE_DIR}/bash/syntax.h
)

add_custom_command(
    OUTPUT version.h
    COMMAND ${CMAKE_SOURCE_DIR}/bash/support/mkversion.sh -b -S ${CMAKE_CURRENT_SOURCE_DIR}/bash -d ${PROJECT_VERSION} -o version.h
    DEPENDS bash/support/mkversion.sh config.h bash/patchlevel.h
)

add_custom_command(
    OUTPUT signames.h
    COMMAND ./mksignames signames.h
    DEPENDS mksignames
)
