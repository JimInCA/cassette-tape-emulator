#
# You can set this Makefile to build hci_dfu_send_hex for Linux
# or for LMVP. The desired build is set by selecting either the
# arm-linux-gcc or gcc compiler.
#

APP        = test-bit-boffer

CC         = gcc
LINKER     = $(CC) -o
RM         = rm -f
MKDIR      = mkdir -p
RMDIR      = rmdir

SRCDIR     = src
INCDIR     = include
OBJDIR     = obj
BINDIR     = bin

CFLAGS    := -I./$(INCDIR)
LFLAGS    :=
OBJECTS   := $(OBJDIR)/test-bit-boffer.o
DIRS      := $(OBJDIR) $(BINDIR)

all: directories $(BINDIR)/$(APP)

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "Compiled "$@" successfully!"

$(BINDIR)/$(APP): $(OBJECTS)
	$(LINKER) $(LDLAGS) $@ $^
	@echo "Linking "$@" complete!"

.PHONEY: directories
directories: $(DIRS)

$(OBJDIR):
	@$(MKDIR) $(OBJDIR)

$(BINDIR):
	@$(MKDIR) $(BINDIR)

.PHONEY: clean
clean:
	$(RM) $(OBJDIR)/*
	@echo "Derived objects removed!"

.PHONEY: realclean
realclean: clean
	@$(RM) $(BINDIR)/*
	@$(RMDIR) $(OBJDIR)
	@$(RMDIR) $(BINDIR)
	@echo "Binaries removed!"

