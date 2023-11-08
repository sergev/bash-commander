add_library(sh STATIC
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/clktck.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/clock.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/getenv.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/oslib.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/setlinebuf.c
#   ${CMAKE_SOURCE_DIR}/bash/lib/sh/strnlen.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/itos.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/zread.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/zwrite.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/shtty.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/shmatch.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/eaccess.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/netconn.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/netopen.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/timeval.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/makepath.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/pathcanon.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/pathphys.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/tmpfile.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/stringlist.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/stringvec.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/spell.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/shquote.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/strtrans.c
#   ${CMAKE_SOURCE_DIR}/bash/lib/sh/snprintf.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/mailstat.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/fmtulong.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/fmtullong.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/fmtumax.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/zcatfd.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/zmapfd.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/winsize.c
#   ${CMAKE_SOURCE_DIR}/bash/lib/sh/wcsdup.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/fpurge.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/zgetline.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/mbscmp.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/uconvert.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/ufuncs.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/casemod.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/input_avail.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/mbscasecmp.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/fnxform.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/unicode.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/shmbchar.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/strvis.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/utf8.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/random.c
#   ${CMAKE_SOURCE_DIR}/bash/lib/sh/gettimeofday.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/timers.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/wcsnwidth.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/strchrnul.c
    ${CMAKE_SOURCE_DIR}/bash/lib/sh/mbschr.c
)
target_include_directories(sh BEFORE PUBLIC
    ${CMAKE_SOURCE_DIR}/bash/lib/sh
)
