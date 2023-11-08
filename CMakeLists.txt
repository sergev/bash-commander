cmake_minimum_required(VERSION 3.15)

# Version of Bash Commander must match version of GNU Bash
# which we clone from savannah.gnu.org.
project(bashc VERSION 5.2 DESCRIPTION "Bash Commander")
set(CMAKE_C_STANDARD 11)

# Mask out most issues in Bash source code.
add_compile_options(-Wno-deprecated-non-prototype -Wno-parentheses -Wno-incompatible-pointer-types-discards-qualifiers)

# Get git revision count.
execute_process(
    COMMAND git rev-list HEAD --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_REVCOUNT
)

# Configure parameters required by Bash.
add_definitions(-DCONF_MACHTYPE="${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_C_COMPILER_ID}-${CMAKE_SYSTEM}")
add_definitions(-DCONF_HOSTTYPE="${CMAKE_SYSTEM_PROCESSOR}")
add_definitions(-DCONF_VENDOR="${CMAKE_C_COMPILER_ID}")
add_definitions(-DCONF_OSTYPE="${CMAKE_SYSTEM}")
add_definitions(-DLOCALEDIR="/usr/local/share/locale")
add_definitions(-DPACKAGE="bashc")
add_definitions(-DDISTVERSION="${CMAKE_PROJECT_VERSION}")
add_definitions(-DDEFAULT_COMPAT_LEVEL=${CMAKE_PROJECT_VERSION_MAJOR}*10+${CMAKE_PROJECT_VERSION_MINOR})
add_definitions(-DBUILDVERSION=${GIT_REVCOUNT})
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DSHELL)
add_definitions(-DCOMMANDER)

include(CheckSymbolExists)
check_symbol_exists(arc4random "stdlib.h" HAVE_ARC4RANDOM)
if(HAVE_ARC4RANDOM)
    add_definitions(-DHAVE_ARC4RANDOM)
endif()

# Need Bison for grammar.
find_package(BISON 3.0 REQUIRED)
bison_target(parse parse.y parse.c)

# Need ncurses.
set(CURSES_NEED_NCURSES TRUE)
#set(CURSES_NEED_WIDE TRUE)
find_package(Curses REQUIRED)

# Create empty version.h - it's required by Bash sources.
file(CONFIGURE
    OUTPUT version.h
    CONTENT "/* This file in unused in Bash Commander. */"
)

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
target_link_libraries(${PROJECT_NAME} builtins readline sh glob tilde ${CURSES_LIBRARIES})
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
