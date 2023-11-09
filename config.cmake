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
#include(CheckIncludeFile)

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
