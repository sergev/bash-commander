add_library(sh STATIC
    clktck.c
    clock.c
    getenv.c
    oslib.c
    setlinebuf.c
#   strnlen.c
    itos.c
    zread.c
    zwrite.c
    shtty.c
    shmatch.c
    eaccess.c
    netconn.c
    netopen.c
    timeval.c
    makepath.c
    pathcanon.c
    pathphys.c
    tmpfile.c
    stringlist.c
    stringvec.c
    spell.c
    shquote.c
    strtrans.c
#   snprintf.c
    mailstat.c
    fmtulong.c
    fmtullong.c
    fmtumax.c
    zcatfd.c
    zmapfd.c
    winsize.c
#   wcsdup.c
    fpurge.c
    zgetline.c
    mbscmp.c
    uconvert.c
    ufuncs.c
    casemod.c
    input_avail.c
    mbscasecmp.c
    fnxform.c
    unicode.c
    shmbchar.c
    strvis.c
    utf8.c
    random.c
#   gettimeofday.c
    timers.c
    wcsnwidth.c
    strchrnul.c
    mbschr.c
)
target_include_directories(sh BEFORE PUBLIC
    .
)
