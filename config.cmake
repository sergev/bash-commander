# Get git revision count.
execute_process(
    COMMAND git rev-list HEAD --count
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE GIT_REVCOUNT
)

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
include(CheckStructHasMember)
include(CheckIncludeFile)

# Check for arc4random() function.
check_symbol_exists(arc4random "stdlib.h" HAVE_ARC4RANDOM)
if(HAVE_ARC4RANDOM)
    add_definitions(-DHAVE_ARC4RANDOM)
endif()

# Check for sys_siglist[] array.
check_symbol_exists(sys_siglist "signal.h" HAVE_SYS_SIGLIST)
if(HAVE_SYS_SIGLIST)
    add_definitions(-DHAVE_SYS_SIGLIST=1)
endif()

# Check for field d_namlen in struct dirent.
check_struct_has_member("struct dirent" d_namlen "sys/dirent.h" HAVE_STRUCT_DIRENT_D_NAMLEN)
if(HAVE_STRUCT_DIRENT_D_NAMLEN)
    add_definitions(-DHAVE_STRUCT_DIRENT_D_NAMLEN=1)
endif()

# Check for macro TIOCSTAT.
check_symbol_exists(TIOCSTAT "sys/ioctl.h" TIOCSTAT_IN_SYS_IOCTL)
if(TIOCSTAT_IN_SYS_IOCTL)
    add_definitions(-DTIOCSTAT_IN_SYS_IOCTL=1)
endif()

# Check for macro TIOCGWINSZ.
check_symbol_exists(TIOCGWINSZ "sys/ioctl.h" GWINSZ_IN_SYS_IOCTL)
if(GWINSZ_IN_SYS_IOCTL)
    add_definitions(-DGWINSZ_IN_SYS_IOCTL=1)
endif()

# Check for macro TCSETAW.
check_symbol_exists(TCSETAW "termio.h" TERMIO_LDISC)
if(TERMIO_LDISC)
    add_definitions(-DTERMIO_LDISC=1)
endif()

# Check for macro TCSADRAIN.
check_symbol_exists(TCSADRAIN "termios.h" TERMIOS_LDISC)
if(TERMIOS_LDISC)
    add_definitions(-DTERMIOS_LDISC=1)
endif()

# Check for field st_atim in struct stat.
check_struct_has_member("struct stat" st_atim "sys/stat.h" HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
if(HAVE_STRUCT_STAT_ST_ATIM_TV_NSEC)
    add_definitions(-DHAVE_STRUCT_STAT_ST_ATIM_TV_NSEC=1)
    add_definitions(-DTYPEOF_STRUCT_STAT_ST_ATIM_IS_STRUCT_TIMESPEC=1)
endif()

# Check for field st_atimespec in struct stat.
check_struct_has_member("struct stat" st_atimespec "sys/stat.h" HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC)
if(HAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC)
    add_definitions(-DHAVE_STRUCT_STAT_ST_ATIMESPEC_TV_NSEC=1)
endif()

# Check for macro AUDIT_USER_TTY.
check_symbol_exists(AUDIT_USER_TTY "linux/audit.h" HAVE_DECL_AUDIT_USER_TTY)
if(HAVE_DECL_AUDIT_USER_TTY)
    add_definitions(-DHAVE_DECL_AUDIT_USER_TTY=1)
else()
    add_definitions(-DHAVE_DECL_AUDIT_USER_TTY=0)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Whether to use pipes to communicate with the process group leader.
    add_definitions(-DPGRP_PIPE=1)

    # Use ulimit(4, 0) to get the maximum number of files that the process can open.
    add_definitions(-DULIMIT_MAXFDS=1)
endif()

# Check for eaccess() function.
check_symbol_exists(eaccess "unistd.h" HAVE_EACCESS)
if(HAVE_EACCESS)
    add_definitions(-DHAVE_EACCESS=1)
endif()

# Check for fpurge() function.
check_symbol_exists(fpurge "stdio.h" HAVE_FPURGE)
if(HAVE_FPURGE)
    add_definitions(-DHAVE_FPURGE=1)
    add_definitions(-DHAVE_DECL_FPURGE=1)
else()
    add_definitions(-DHAVE_DECL_FPURGE=0)
endif()

# Check for __fpurge() function.
check_symbol_exists(__fpurge "stdio_ext.h" HAVE___FPURGE)
if(HAVE___FPURGE)
    add_definitions(-DHAVE___FPURGE=1)
endif()

# Check for getrandom() function.
check_symbol_exists(getrandom "sys/random.h" HAVE_GETRANDOM)
if(HAVE_GETRANDOM)
    add_definitions(-DHAVE_GETRANDOM=1)
endif()

# Check for iconv() function.
check_symbol_exists(iconv "iconv.h" HAVE_ICONV)
if(HAVE_ICONV)
    add_definitions(-DHAVE_ICONV=1)
endif()

# Check for setresgid() function.
check_symbol_exists(setresgid "unistd.h" HAVE_SETRESGID)
if(HAVE_SETRESGID)
    add_definitions(-DHAVE_SETRESGID=1)
endif()

# Check for setresuid() function.
check_symbol_exists(setresuid "unistd.h" HAVE_SETRESUID)
if(HAVE_SETRESUID)
    add_definitions(-DHAVE_SETRESUID=1)
endif()

# Check for strchrnul() function.
check_symbol_exists(strchrnul "string.h" HAVE_STRCHRNUL)
if(HAVE_STRCHRNUL)
    add_definitions(-DHAVE_STRCHRNUL=1)
endif()

# Check for <termio.h> header file.
check_include_file("termio.h" HAVE_TERMIO_H)
if(HAVE_TERMIO_H)
    add_definitions(-DHAVE_TERMIO_H=1)
endif()

# Check for <termios.h> header file.
check_include_file("termios.h" HAVE_TERMIOS_H)
if(HAVE_TERMIOS_H)
    add_definitions(-DHAVE_TERMIOS_H=1)
endif()

# Check for <argz.h> header file.
check_include_file("argz.h" HAVE_ARGZ_H)
if(HAVE_ARGZ_H)
    add_definitions(-DHAVE_ARGZ_H=1)
endif()

# Check for <malloc.h> header file.
check_include_file("malloc.h" HAVE_MALLOC_H)
if(HAVE_MALLOC_H)
    add_definitions(-DHAVE_MALLOC_H=1)
endif()

# Check for <stdio_ext.h> header file.
check_include_file("stdio_ext.h" HAVE_STDIO_EXT_H)
if(HAVE_STDIO_EXT_H)
    add_definitions(-DHAVE_STDIO_EXT_H=1)
endif()

# Check for mempcpy() function.
check_symbol_exists(mempcpy "string.h" HAVE_MEMPCPY)
if(HAVE_MEMPCPY)
    add_definitions(-DHAVE_MEMPCPY=1)
endif()

# Check for mremap() function.
check_symbol_exists(mremap "sys/mman.h" HAVE_MREMAP)
if(HAVE_MREMAP)
    add_definitions(-DHAVE_MREMAP=1)
endif()

# Check for __argz_count() function.
check_symbol_exists(__argz_count "argz.h" HAVE___ARGZ_COUNT)
if(HAVE___ARGZ_COUNT)
    add_definitions(-DHAVE___ARGZ_COUNT=1)
endif()

# Check for __argz_next() function.
check_symbol_exists(__argz_next "argz.h" HAVE___ARGZ_NEXT)
if(HAVE___ARGZ_NEXT)
    add_definitions(-DHAVE___ARGZ_NEXT=1)
endif()

# Check for __argz_stringify() function.
check_symbol_exists(__argz_stringify "argz.h" HAVE___ARGZ_STRINGIFY)
if(HAVE___ARGZ_STRINGIFY)
    add_definitions(-DHAVE___ARGZ_STRINGIFY=1)
endif()
