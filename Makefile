#
# make
# make all      -- build everything
#
# make install  -- install picosim binaries to /usr/local
#
# make clean    -- remove build files
#

all:    build
	$(MAKE) -C build $@

install: build
	$(MAKE) -C build $@

clean:
	rm -rf build

.PHONY: all clean install patches

# Configure for build
build:  bash
	mkdir $@
	cmake -B $@ .

#
# Checkout Bash sources.
# Select a specific commit.
#
BASH_REPO           = https://git.savannah.gnu.org/git/bash.git
BASH_5_2_PATCH_15   = ec8113b9861375e4e17b3307372569d429dec814
BASH_5_2_PATCH_21   = 2bb3cbefdb8fd019765b1a9cc42ecf37ff22fec6

bash:
	git clone --depth 30 $(BASH_REPO) $@
	(cd $@; git reset --hard $(BASH_5_2_PATCH_15))

#
# Create patches from C sources.
#
PATCHES = patches/bashline.diff \
          patches/display.diff \
          patches/terminal.diff \
          patches/locale.diff \
          patches/sig.diff \
          patches/variables.diff
patches: $(PATCHES)

patches/bashline.diff: bash/bashline.c bashline.c
	-diff -u -w bash/bashline.c bashline.c > $@

patches/display.diff: bash/lib/readline/display.c lib/readline/display.c
	-diff -u -w bash/lib/readline/display.c lib/readline/display.c > $@

patches/terminal.diff: bash/lib/readline/terminal.c lib/readline/terminal.c
	-diff -u -w bash/lib/readline/terminal.c lib/readline/terminal.c > $@

patches/locale.diff: bash/locale.c locale.c
	-diff -u -w bash/locale.c locale.c > $@

patches/sig.diff: bash/sig.c sig.c
	-diff -u -w bash/sig.c sig.c > $@

patches/variables.diff: bash/variables.c variables.c
	-diff -u -w bash/variables.c variables.c > $@
