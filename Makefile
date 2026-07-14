c_cmd			=	cc
cflags		=	-O2 -Wall -Wunused -lpthread $(shell pkg-config --cflags --libs gtk+-3.0 gtk-vnc-2.0)
#cflags		=	-O2 -Wall -Wunused -lpthread $(shell pkg-config --cflags --libs gtk4 gtk-vnc-2.0)
winstuff	=
dgb_flags	=	-g -O0
bindir		= /usr/local/bin
target		=	spiritvnc
src				=	$(wildcard src/*.c)
pkgconf		=	$(shell command -v pkg-config)
osname		= $(shell uname -s)

# check if we're on windowz
ifeq ($(OS),Windows_NT)
	winstuff += -lws2_32 -Wl,-subsystem,windows
else ifneq (,$(findstring MINGW,$(OS)))
  winstuff += -lws2_32 -Wl,-subsystem,windows
endif

# make teh thing
spiritvnc:
	@echo "Building on '$(osname)'"
	@echo

ifeq ($(pkgconf),)
	@echo
	@echo "**** ERROR: Unable to run 'pkg-config' ****"
	@echo "**** Be sure pkg-config is installed on your system ****"
	@exit 1
endif

	# compile resources first
	glib-compile-resources --generate-source --target=src/spiritvnc.gresource.c src/spiritvnc.gresource.xml

	# now compile app
	$(c_cmd) $(src) -o $(target) $(cflags) $(winstuff)
	@echo

# make teh debug thing
debug:
	@echo "Building debug on '$(osname)'"
	@echo

ifeq ($(pkgconf),)
	@echo
	@echo "**** ERROR: Unable to run 'pkg-config' ****"
	@echo "**** Be sure pkg-config is installed on your system ****"
	@exit 1
endif

	# compile resources first
	glib-compile-resources --generate-source --target=src/spiritvnc.gresource.c src/spiritvnc.gresource.xml

	# now compile app
	$(c_cmd) $(src) -o $(target) $(cflags) $(winstuff) $(dgb_flags)
	@echo

.PHONY: clean
clean::
	rm -f $(target)
	@echo

install:
	install -c -s -o root -m 555 $(target) $(bindir)
	@echo

uninstall:
	@if [ -f $(bindir)"/"$(target) ] ; then \
		rm -fv $(bindir)"/"$(target) ; \
	fi
	@echo
