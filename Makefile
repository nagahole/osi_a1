###############################################
# C                                           #
###############################################
CC=gcc
CFLAGS=-Wall -Werror=vla -Werror=pointer-arith -std=c2x -fsanitize=address
LDFLAGS=-lpthread
TARGET=main

###############################################
# Detect files and set paths accordingly      #
###############################################

LIBDIR=lib
SRCDIR=src
BUILDDIR=objs

INCLUDE=-I$(LIBDIR)

SRCFILES=$(wildcard $(SRCDIR)/*.c)
OBJFILES=$(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCFILES))

MKDIR_P=mkdir -p

###############################################
# Compile files                               #
###############################################

all: $(TARGET) Makefile

$(TARGET): $(TARGET).c $(OBJFILES)
	@$(CC) $(TARGET).c $(CFLAGS) $(LDFLAGS) $(INCLUDE) $(wordlist 2,$(words $^),$^) -o $@
	@echo "Target binary \"$(TARGET)\" built"

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@$(MKDIR_P) $(BUILDDIR)
	@echo "[Building Object File]" $@
	@$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@

###############################################

.PHONY:
clean:
	@rm -f pkgchk.o
	@rm -f $(PKG_CHKR)
	@rm -f $(TARGET)
	@rm -f $(BUILDDIR)/*.o
	@rm -f $(PACKET_CHECKER)
	@rmdir $(BUILDDIR)
	@echo "Clean complete!"

###############################################
