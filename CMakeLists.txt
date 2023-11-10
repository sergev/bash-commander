cmake_minimum_required(VERSION 3.15)

# Version of Bash Commander must match version of GNU Bash
# which we clone from savannah.gnu.org.
project(bashc VERSION 5.2 DESCRIPTION "Bash Commander")
set(CMAKE_C_STANDARD 11)

# Mask out most issues in Bash source code.
add_compile_options(-Wno-deprecated-non-prototype -Wno-parentheses -Wno-incompatible-pointer-types-discards-qualifiers)

# Configure parameters for your system.
include(config.cmake)

# Need Bison for grammar.
find_package(BISON 3.0 REQUIRED)
bison_target(parse parse.y parse.c)

# Need ncurses.
set(CURSES_NEED_NCURSES TRUE)
#set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)

# Create signames.h - a list of signal names.
add_executable(mksignames
    bash/support/mksignames.c
    bash/support/signames.c
)
add_custom_command(
    OUTPUT signames.h
    COMMAND ./mksignames signames.h
    DEPENDS mksignames
)

# Create syntax.c - helper tables for scanner.
add_executable(mksyntax bash/mksyntax.c)
add_custom_command(
    OUTPUT syntax.c
    COMMAND ./mksyntax -o syntax.c
    DEPENDS mksyntax ${CMAKE_CURRENT_SOURCE_DIR}/bash/syntax.h
)

# Build bashc executable.
add_executable(${PROJECT_NAME}
    bash/jobs.c
    bash/error.c
    bash/trap.c
    bash/copy_cmd.c
    bash/input.c
    bash/mailcheck.c
    bash/shell.c
    version.c
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
target_link_libraries(${PROJECT_NAME}
    builtins
    readline
    sh
    glob
    tilde
    ${Iconv_LIBRARY}
    ${CURSES_LIBRARIES}
    ${CMAKE_DL_LIBS}
)
include_directories(
    .
    bash
    bash/include
    bash/lib
    ${CMAKE_BINARY_DIR}
)

# Build required libraries.
add_subdirectory(builtins)
add_subdirectory(lib/readline)
add_subdirectory(lib/sh)
add_subdirectory(lib/glob)
add_subdirectory(lib/tilde)

# Install bashc and scripts.
install(TARGETS ${PROJECT_NAME}
    DESTINATION bin
)
install(FILES
    bash_commander
    bash_dialog
    DESTINATION etc
)
