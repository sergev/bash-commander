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

.PHONY: all clean install

# Configure for build
build:  bash
	mkdir $@
	cmake -B $@ .

#
# Checkout Bash sources.
# Select a specific commit.
#
bash:
	git clone --depth 30 https://git.savannah.gnu.org/git/bash.git $@
	(cd $@; git reset --hard ec8113b9861375e4e17b3307372569d429dec814) # Bash 5.2 patch 15
