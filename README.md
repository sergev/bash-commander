Bash Commander is a fork of GNU Bourne Agail Shell.
It's main feature is a visual two-panel mode, much
like Midnight Commander and other text-mode visual shells.

By default, Bash Commander behaves exactly like traditional bash,
so it's safe to install it as a system-wide /bin/sh.
File panels are enabled only in interactive mode, when
an environment variable COMMANDER is set.
It is recommended to add the following lines to your ~/.bashrc script:

    declare -x EDITOR="le"
    declare -x VIEWER="le --read-only"
    . /usr/local/etc/bash_commander

Bash Commander also has scripting capabilities.
When function keys F1-F12 are pressed, it calls a shell function with
appropriate name like commander_f1(), in case it is defined.
When <Enter> is pressed, a function commander_start_file()
is called with a parameter - a name of the file.
You can define the functions in your ~/.bashrc file.
See /usr/local/etc/bash_commander as example.

When building Bach Commander from sources, make sure
the ncurses library is installed. On Ubuntu, you can install it
by:

    sudo apt-get install libncursesw5-dev


Best wishes,
--Serge Vakulenko
