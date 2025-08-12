# project name (generate executable with this name)
# TARGET   = gurux.dlms.client.bin
TARGET   = evs2_comms_dlms_wasion

CC       = gcc


LINKER   = gcc -o

# debug mode
# CFLAGS = -g -I$(SRCDIR) -I../include -Wall
# LFLAGS= -L../development/lib -lpthread 

#release mode
CFLAGS = -Os -flto -I$(SRCDIR) -I../include -Wall
LFLAGS = -L../development/lib -lpthread -s -flto

# change these to set the proper directories where each files should be

SRCDIR   = src
OBJDIR   = obj
BINDIR   = .

SOURCES  := $(wildcard $(SRCDIR)/*.c)
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
rm       = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS)
	@$(LINKER) $@ $(LFLAGS) $(OBJECTS) -lgurux_dlms_c -lm
	@echo "Linking complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONEY: clean
clean:
	@$(rm) $(OBJECTS)
	@echo "Cleanup complete!" 
	@echo $(OBJECTS)

.PHONEY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET)
	@echo "Executable removed!"