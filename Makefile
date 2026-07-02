CC				=	cc
CFLAGS		=	-O2 -Wall -Wunused -lpthread $(shell pkg-config --cflags --libs gtk+-3.0 gtk-vnc-2.0)
#CFLAGS		=	-O2 -Wall -Wunused -lpthread $(shell pkg-config --cflags --libs gtk4 gtk-vnc-2.0)
WINSTUFF	=
DEBUGFLGS	=	-g -O0
BINDIR		= /usr/local/bin
TARGET		=	spiritvnc
SRC				=	$(wildcard src/*.c)
PKGCONF		=	$(shell command -v pkg-config)
OSNAME		= $(shell uname -s)

ifeq ($(OS),Windows_NT)
	WINSTUFF += -lws2_32
else ifneq (,$(findstring MINGW,$(OS)))
  WINSTUFF += -lws2_32
endif

# make teh thing
spiritvnc:
	@echo "Building on '$(OSNAME)'"
	@echo

ifeq ($(PKGCONF),)
	@echo
	@echo "**** ERROR: Unable to run 'pkg-config' ****"
	@exit 1
endif

	# compile resources first
	glib-compile-resources --generate-source --target=src/spiritvnc.gresource.c src/spiritvnc.gresource.xml

	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(WINSTUFF)
	@echo

# make teh debug thing
debug:
	@echo "Building debug on '$(OSNAME)'"
	@echo

ifeq ($(PKGCONF),)
	@echo
	@echo "**** ERROR: Unable to run 'pkg-config' ****"
	@exit 1
endif

	# compile resources first
	glib-compile-resources --generate-source --target=src/spiritvnc.gresource.c src/spiritvnc.gresource.xml

	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(WINSTUFF) $(DEBUGFLGS)
	@echo

.PHONY: clean
clean::
	rm -f $(TARGET)
	@echo

install:
	install -c -s -o root -m 555 $(TARGET) $(BINDIR)
	@echo

uninstall:
	@if [ -f $(BINDIR)"/"$(TARGET) ] ; then \
		rm -fv $(BINDIR)"/"$(TARGET) ; \
	fi
	@echo
