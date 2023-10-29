PREFIX ?= /usr/local
DESTDIR ?= ok

OS = $(shell uname)
CC ?= clang
AR = ar
LN = ln
RM = rm
VALGRIND ?= valgrind
STRIP = strip

LIB_NAME = ok
VERSION_MAJOR = 0
VERSION_MINOR = 6

TARGET_NAME = lib$(LIB_NAME)
TARGET_STATIC = $(TARGET_NAME).a
TARGET_DSOLIB = $(TARGET_NAME).so.$(VERSION_MAJOR).$(VERSION_MINOR)
TARGET_DYLIB = $(TARGET_NAME).$(VERSION_MAJOR).$(VERSION_MINOR).dylib
TARGET_DSO = $(TARGET_NAME).so

CFLAGS += -I.
CFLAGS += -std=c99 -Wall -O2
CFLAGS += -fvisibility=hidden
CFLAGS += -fPIC -pedantic

LDFLAGS += -shared -soname $(TARGET_DSO).$(VERSION_MAJOR)

OSX_LDFLAGS += -lc -Wl,-install_name,$(TARGET_DSO) -o $(TARGET_DSOLIB)

SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)

TEST_MAIN = ok-test

ifneq ("Darwin","$(OS)")
	CFLAGS += -lm
endif

ifdef DEBUG
	CFLAGS += -DOK_DEBUG
endif

all: $(TARGET_STATIC) $(TARGET_DSO)

$(TARGET_STATIC): $(OBJS)
	@echo "  LIBTOOL-STATIC $(TARGET_STATIC)"
	@$(AR) crus $(TARGET_STATIC) $(OBJS)

$(TARGET_DSO): $(OBJS)
	@echo "  LIBTOOL-SHARED $(TARGET_DSOLIB)"
	@echo "  LIBTOOL-SHARED $(TARGET_DSO)"
	@echo "  LIBTOOL-SHARED $(TARGET_DSO).$(VERSION_MAJOR)"
ifeq ("Darwin","$(OS)")
	@$(CC) -shared $(OBJS) $(OSX_LDFLAGS) -o $(TARGET_DSOLIB)
	@$(LN) -s $(TARGET_DSOLIB) $(TARGET_DSO)
	@$(LN) -s $(TARGET_DSOLIB) $(TARGET_DSO).$(VERSION_MAJOR)
else
	@$(CC) -shared $(OBJS) -o $(TARGET_DSOLIB)
	@$(LN) -s $(TARGET_DSOLIB) $(TARGET_DSO)
	@$(LN) -s $(TARGET_DSOLIB) $(TARGET_DSO).$(VERSION_MAJOR)
	@$(STRIP) --strip-unneeded $(TARGET_DSO)
endif

$(OBJS):
	@echo "  CC(target) $@"
	@$(CC) $(CFLAGS) -c -o $@ $(@:.o=.c)

check: test
	$(VALGRIND) --leak-check=full ./$(TEST_MAIN)

test:
	@echo "  LINK(target) ($(TEST_MAIN))"
	@$(CC) test.c ./$(TARGET_STATIC) $(CFLAGS) -o $(TEST_MAIN)
	@./$(TEST_MAIN)

clean:
	@for item in \
		$(TEST_MAIN) $(OBJS) $(TARGET_STATIC) \
		$(TARGET_DSOLIB) $(TARGET_DSO).$(VERSION_MAJOR) \
		$(TARGET_DSO) $(TARGET_DYLIB) \
		; do \
		echo "  RM $$item"; \
		$(RM) -rf $$item; \
	done;


install: all
	@test -d $(PREFIX)/$(DESTDIR) || mkdir $(PREFIX)/$(DESTDIR)
	@cp *.h $(PREFIX)/include/$(DESTDIR)
	@echo "  INSTALL $(LIB_NAME).h";
	@install $(LIB_NAME).h $(PREFIX)/include
	@echo "  INSTALL $(TARGET_STATIC)";
	@install $(TARGET_STATIC) $(PREFIX)/lib
	@echo "  INSTALL $(TARGET_DSO)";
	@install $(TARGET_DSO) $(PREFIX)/lib

uninstall:
	rm -rf $(PREFIX)/$(DESTDIR)
	rm -f $(PREFIX)/lib/$(TARGET_STATIC)
	rm -f $(PREFIX)/lib/$(TARGET_DSO)

