#---------------------------------------------------------------------------------------------------
# Shell Lab                                   Fall 2023                          System Programming
#
# Makefile
#
# GNU make documentation: https://www.gnu.org/software/make/manual/make.html
#
# Typically, the only thing you need to modify in this Makefile is the list of source files.
#

#--- variable declarations

# directories
SRC_DIR=src
OBJ_DIR=obj
DEP_DIR=.deps

# C compiler and compilation flags
CC=gcc
CFLAGS=-Wall -O2 -g
DEPFLAGS=-MMD -MP -MT $@ -MF $(DEP_DIR)/$*.d

# make sure SOURCES includes ALL source files required to compile the project
SOURCES=csapsh.c jobcontrol.c parser.c
TARGET=csapsh

# derived variables
OBJECTS=$(SOURCES:%.c=$(OBJ_DIR)/%.o)
DEPS=$(SOURCES:%.c=$(DEP_DIR)/%.d)

#--- rules
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(DEP_DIR) $(OBJ_DIR)
	$(CC) $(CFLAGS) $(DEPFLAGS) -o $@ -c $<

$(DEP_DIR):
	@mkdir -p $(DEP_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

-include $(DEPS)

clean:
	rm -rf $(OBJ_DIR) $(DEP_DIR) $(TARGET)
