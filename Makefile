# Production Makefile for pam_shepherd
# PAM module to start GNU Shepherd daemon on user login

DESTDIR =
PREFIX = /usr
PREFIX_MAN = /usr/local
MANDIR = share/man

# Auto-detect PAM module directory based on system architecture
ARCH := $(shell uname -m)
ifeq ($(ARCH),x86_64)
    # 64-bit systems typically use /lib64
    ifneq ($(wildcard /lib64/security),)
        PAMDIR = /lib64/security
    else ifneq ($(wildcard /usr/lib64/security),)
        PAMDIR = /usr/lib64/security
    else
        PAMDIR = /usr/lib/security
    endif
else
    # 32-bit or other architectures
    ifneq ($(wildcard /lib/security),)
        PAMDIR = /lib/security
    else
        PAMDIR = /usr/lib/security
    endif
endif

# Allow override via environment
ifdef PAM_MODULE_DIR
    PAMDIR = $(PAM_MODULE_DIR)
endif

# Compiler settings
CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra
CFLAGS += -fPIC -fno-strict-aliasing
CPPFLAGS += -D_GNU_SOURCE -D_DEFAULT_SOURCE
LDFLAGS += -shared -Wl,-x -Wl,--no-as-needed
LIBS = -lpam

# Strip in production
STRIP ?= strip

# Module name
MODULE = pam_shepherd.so
SOURCE = pam_shepherd.c
MANPAGE = pam_shepherd.8

# Default target
all: $(MODULE)

$(MODULE): $(SOURCE)
	@echo "Building $(MODULE) for $(ARCH)"
	@echo "PAM directory: $(PAMDIR)"
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)
	$(STRIP) --strip-unneeded $@
	@echo "Module built successfully"

clean:
	rm -f $(MODULE) *.o

install: $(MODULE)
	@echo "Installing PAM module..."
	install -d $(DESTDIR)$(PAMDIR)
	install -m 0644 $(MODULE) $(DESTDIR)$(PAMDIR)/
	@echo "Installing manual page..."
	install -d $(DESTDIR)$(PREFIX_MAN)/$(MANDIR)/man8
	install -m 0644 $(MANPAGE) $(DESTDIR)$(PREFIX_MAN)/$(MANDIR)/man8/ 2>/dev/null || true
	@echo
	@echo "=== Installation Complete ==="
	@echo "Module installed to: $(DESTDIR)$(PAMDIR)/$(MODULE)"
	@echo
	@echo "Configuration:"
	@echo "  Add to /etc/pam.d/system-login (or equivalent):"
	@echo "    session  optional  pam_shepherd.so"
	@echo
	@echo "Options:"
	@echo "  debug   - Enable debug logging to syslog"
	@echo "  quiet   - Suppress non-error messages"
	@echo "  wait=N  - Wait N milliseconds for shepherd to start (default 200)"
	@echo
	@echo "Example:"
	@echo "  session  optional  pam_shepherd.so quiet wait=500"

uninstall:
	rm -f $(DESTDIR)$(PAMDIR)/$(MODULE)
	rm -f $(DESTDIR)$(PREFIX_MAN)/$(MANDIR)/man8/$(MANPAGE)
	@echo "Uninstalled pam_shepherd"

test-build: $(MODULE)
	@echo "Checking module symbols..."
	@nm -D $(MODULE) | grep pam_sm_ || (echo "ERROR: Missing PAM symbols" && exit 1)
	@echo "Checking dependencies..."
	@ldd $(MODULE)
	@echo "Module size: $$(stat -c%s $(MODULE)) bytes"
	@echo "Build test passed"

show-paths:
	@echo "System: $(ARCH)"
	@echo "PAM directory: $(PAMDIR)"
	@echo "Manual directory: $(PREFIX_MAN)/$(MANDIR)/man8"
	@echo
	@echo "To override PAM directory:"
	@echo "  make PAM_MODULE_DIR=/your/path install"

# Development target (with debug symbols, no strip)
dev: CFLAGS += -g -DDEBUG
dev: STRIP = true
dev: $(MODULE)
	@echo "Built development version with debug symbols"

.PHONY: all clean install uninstall test-build show-paths dev