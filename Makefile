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
	cmake -B $@ .

#
# Checkout Bash sources.
# Select a specific commit.
#
BASH_REPO           = https://git.savannah.gnu.org/git/bash.git
BASH_5_2_PATCH_21   = 2bb3cbefdb8fd019765b1a9cc42ecf37ff22fec6

bash:
	git clone --depth 30 $(BASH_REPO) $@
	(cd $@; git reset --hard $(BASH_5_2_PATCH_21))

#
# Create patches from C sources.
#
PATCHES = patches/bashline.diff \
          patches/display.diff \
          patches/terminal.diff \
          patches/locale.diff \
          patches/sig.diff \
          patches/variables.diff \
          patches/parse.diff
patches: bash $(PATCHES)

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

patches/parse.diff: bash/parse.y parse.y
	-diff -u -w bash/parse.y parse.y > $@

#
# Create C sources by applying patches to the original Bash code.
#
SOURCES = bashline.c-new \
          lib/readline/display.c-new \
          lib/readline/terminal.c-new \
          locale.c-new \
          sig.c-new \
          variables.c-new \
          parse.y-new
sources: bash $(SOURCES)
	@mv bashline.c-new bashline.c
	@mv lib/readline/display.c-new lib/readline/display.c
	@mv lib/readline/terminal.c-new lib/readline/terminal.c
	@mv locale.c-new locale.c
	@mv sig.c-new sig.c
	@mv variables.c-new variables.c
	@mv parse.y-new parse.y

bashline.c-new: bash/bashline.c
	patch --ignore-whitespace -o $@ bash/bashline.c patches/bashline.diff

lib/readline/display.c-new: bash/lib/readline/display.c
	patch --ignore-whitespace -o $@ bash/lib/readline/display.c patches/display.diff

lib/readline/terminal.c-new: bash/lib/readline/terminal.c
	patch --ignore-whitespace -o $@ bash/lib/readline/terminal.c patches/terminal.diff

locale.c-new: bash/locale.c
	patch --ignore-whitespace -o $@ bash/locale.c patches/locale.diff

sig.c-new: bash/sig.c
	patch --ignore-whitespace -o $@ bash/sig.c patches/sig.diff

variables.c-new: bash/variables.c
	patch --ignore-whitespace -o $@ bash/variables.c patches/variables.diff

parse.y-new: bash/parse.y
	patch --ignore-whitespace -o $@ bash/parse.y patches/parse.diff
