cmake_minimum_required(VERSION 3.15)

project(bashc VERSION 4.2 DESCRIPTION "Bash Commander")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
#TODO: add_compile_options(-Wall -Werror -Wshadow)

add_definitions(-DCONF_MACHTYPE="${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_C_COMPILER_ID}-${CMAKE_SYSTEM}")
add_definitions(-DLOCALEDIR="/usr/local/share/locale")
add_definitions(-DPACKAGE="bashc")

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
    ./lib/sh/timers.c
    ./lib/sh/snprintf.c
    ./lib/sh/zgetline.c
    ./lib/sh/zwrite.c
    ./lib/sh/wcsnwidth.c
    ./lib/sh/winsize.c
    ./lib/sh/tmpfile.c
    ./lib/sh/makepath.c
    ./lib/sh/fmtullong.c
    ./lib/sh/strtrans.c
    ./lib/sh/clktck.c
    ./lib/sh/spell.c
    ./lib/sh/oslib.c
    ./lib/sh/shtty.c
    ./lib/sh/stringvec.c
    ./lib/sh/mbschr.c
    ./lib/sh/shquote.c
    ./lib/sh/stringlist.c
    ./lib/sh/zread.c
    ./lib/sh/netconn.c
    ./lib/sh/mbscasecmp.c
    ./lib/sh/uconvert.c
    ./lib/sh/strvis.c
    ./lib/sh/ufuncs.c
    ./lib/sh/zmapfd.c
    ./lib/sh/fnxform.c
    ./lib/sh/eaccess.c
    ./lib/sh/unicode.c
    ./lib/sh/timeval.c
    ./lib/sh/pathcanon.c
    ./lib/sh/strchrnul.c
    ./lib/sh/fmtumax.c
    ./lib/sh/zcatfd.c
    ./lib/sh/mktime.c
    ./lib/sh/shmbchar.c
    ./lib/sh/wcsdup.c
    ./lib/sh/setlinebuf.c
    ./lib/sh/itos.c
    ./lib/sh/gettimeofday.c
    ./lib/sh/strtoimax.c
    ./lib/sh/netopen.c
    ./lib/sh/input_avail.c
    ./lib/sh/pathphys.c
    ./lib/sh/strnlen.c
    ./lib/sh/shmatch.c
    ./lib/sh/getcwd.c
    ./lib/sh/casemod.c
    ./lib/sh/fmtulong.c
    ./lib/sh/fpurge.c
    ./lib/sh/clock.c
    ./lib/sh/getenv.c
    ./lib/sh/random.c
    ./lib/sh/mailstat.c
    ./lib/sh/utf8.c
    ./lib/sh/mbscmp.c
    ./lib/glob/smatch.c
    ./lib/glob/strmatch.c
    ./lib/glob/gmisc.c
    ./lib/glob/xmbsrtowcs.c
    ./lib/glob/glob.c
    ./lib/tilde/tilde.c
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
target_link_libraries(${PROJECT_NAME} builtins ncurses) # TODO: readline history sh glob tilde
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

#./builtins/libbuiltins.a
#./lib/readline/libreadline.a
#./lib/readline/libhistory.a
#./lib/sh/libsh.a
#./lib/glob/libglob.a
#./lib/tilde/libtilde.a

add_subdirectory(builtins)
#add_subdirectory(lib/readline)
#add_subdirectory(lib/sh)
#add_subdirectory(lib/glob)
#add_subdirectory(lib/tilde)
