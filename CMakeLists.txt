cmake_minimum_required(VERSION 3.15)

project(bashc VERSION 4.2 DESCRIPTION "Bash Commander")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
#TODO: add_compile_options(-Wall -Werror -Wshadow)
add_compile_options(-Wno-deprecated-non-prototype -Wno-parentheses)

add_definitions(-DMACOSX)           # TODO: obtain from CMake
add_definitions(-DCONF_MACHTYPE="${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_C_COMPILER_ID}-${CMAKE_SYSTEM}")
add_definitions(-DCONF_HOSTTYPE="${CMAKE_SYSTEM_PROCESSOR}")
add_definitions(-DCONF_VENDOR="${CMAKE_C_COMPILER_ID}")
add_definitions(-DCONF_OSTYPE="${CMAKE_SYSTEM}")
add_definitions(-DLOCALEDIR="/usr/local/share/locale")
add_definitions(-DPACKAGE="bashc")
add_definitions(-DHAVE_CONFIG_H)
add_definitions(-DSHELL)
#add_definitions(-DCOMMANDER)       # TODO: enable

add_executable(${PROJECT_NAME}
    ./jobs.c
    ./error.c
    ./trap.c
    ./copy_cmd.c
    ./input.c
    ./mailcheck.c
    ./shell.c
    ./version.c
    ./dispose_cmd.c
    ./syntax.c
    ./xmalloc.c
    ./alias.c
    ./pathexp.c
    ./locale.c
    ./commander.c
    ./execute_cmd.c
    ./pcomplete.c
    ./general.c
    ./flags.c
    ./test.c
    ./subst.c
    ./braces.c
    ./findcmd.c
    ./expr.c
    ./assoc.c
    ./bashhist.c
    ./arrayfunc.c
    ./list.c
    ./make_cmd.c
    ./sig.c
    ./eval.c
    ./support/man2html.c
    ./pcomplib.c
    ./array.c
    ./bashline.c
    ./lib/readline/complete.c
    ./lib/readline/input.c
    ./lib/readline/shell.c
    ./lib/readline/isearch.c
    ./lib/readline/histsearch.c
    ./lib/readline/text.c
    ./lib/readline/xmalloc.c
    ./lib/readline/funmap.c
    ./lib/readline/kill.c
    ./lib/readline/readline.c
    ./lib/readline/parse-colors.c
    ./lib/readline/keymaps.c
    ./lib/readline/misc.c
    ./lib/readline/tilde.c
    ./lib/readline/signals.c
    ./lib/readline/macro.c
    ./lib/readline/colors.c
    ./lib/readline/vi_mode.c
    ./lib/readline/histfile.c
    ./lib/readline/rltty.c
    ./lib/readline/mbutil.c
    ./lib/readline/display.c
    ./lib/readline/history.c
    ./lib/readline/xfree.c
    ./lib/readline/terminal.c
    ./lib/readline/compat.c
    ./lib/readline/undo.c
    ./lib/readline/util.c
    ./lib/readline/histexpand.c
    ./lib/readline/bind.c
    ./lib/readline/nls.c
    ./lib/readline/search.c
    ./lib/readline/callback.c
    ./lib/readline/savestring.c
    ./lib/readline/parens.c
    ./hashcmd.c
    ./y.tab.c
    ./variables.c
    ./unwind_prot.c
    ./hashlib.c
    ./print_cmd.c
    ./stringlib.c
    ./bracecomp.c
    ./redir.c
)
target_link_libraries(${PROJECT_NAME} builtins sh glob tilde ncurses) # TODO: readline history
include_directories(. include lib)

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

add_executable(mksignames
    support/mksignames.c
    support/signames.c
)
#target_include_directories(mksignames PRIVATE .)

add_executable(bashversion
    support/bashversion.c
    version.c
)
#target_include_directories(bashversion PRIVATE . include)

#./lib/readline/libreadline.a
#./lib/readline/libhistory.a

add_subdirectory(builtins)
#add_subdirectory(lib/readline)
add_subdirectory(lib/sh)
add_subdirectory(lib/glob)
add_subdirectory(lib/tilde)
