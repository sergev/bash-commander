Bash Commander is a version of the GNU Bourne Again Shell, with one extra feature:
a visual mode with two panels for browsing folders and navigating file systems.
This makes Bash Commander similar to other text-mode visual shells like Midnight Commander.

![bashc](https://raw.githubusercontent.com/wiki/sergev/bash-commander/images/two-panels.jpg)

By default, Bash Commander behaves exactly like traditional `bash`,
so it's safe to install it as a system-wide `/bin/sh`.
File panels are enabled only in interactive mode, when
an environment variable COMMANDER is set.
It is recommended to add the following lines to your `~/.bashrc` script:

    declare -x EDITOR="le"
    declare -x VIEWER="le --read-only"
    . /usr/local/etc/bash_commander

Bash Commander also has scripting capabilities.
When function keys F1-F12 are pressed, it calls a shell function with
appropriate name like `commander_f1()`, in case it is defined.
When <Enter> is pressed, a function `commander_start_file()`
is called with a parameter - a name of the file.
You can define the functions in your `~/.bashrc` file.
See [/usr/local/etc/bash_commander](https://github.com/sergev/bash-commander/blob/master/bash_commander)
as an example.

## Prerequisites

When building Bach Commander from sources, a few tools and libraries
must be pre-installed.

On Ubuntu:

    sudo apt install build-essential git cmake bison libncursesw5-dev dialog

On MacOS:

    brew install git cmake bison ncurses dialog

On FreeBSD:

    pkg install git cmake bison cdialog

## Build and Install

    make
    make install

Three files are installed:

  * `/usr/local/bin/bashc` - binary executable of Bash Commander
  * `/usr/local/etc/bash_commander` - startup script with F1-F12 keys defined
  * `/usr/local/etc/bash_dialog` - configuration script for `/usr/bin/dialog` with proper colors
