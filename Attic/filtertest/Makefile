# Optimization level, can be [0, 1, 2, 3, s]. 
#     0 = turn off optimization. s = optimize for size.
# 
OPT = 1

# Object files directory
# Warning: this will be removed by make clean!
#
OBJDIR = obj

# Target file name (without extension)
TARGET = $(OBJDIR)/filtertest

# Define all C source files (dependencies are generated automatically)
#
INCDIRS += .
SOURCES += filter.c
SOURCES += filtertest.c

#============================================================================
#
OBJECTS  += $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))
CPPFLAGS += $(addprefix -I,$(INCDIRS))

#---------------- Preprocessor Options ----------------
#  -g             generate debugging information
#  -save-temps    preserve .s and .i-files
#
CPPFLAGS += -g
# CPPFLAGS += -save-temps=obj

#---------------- C Compiler Options ----------------
#  -O*            optimization level
#  -f...          tuning, see GCC documentation
#  -Wall...       warning level
#
CFLAGS  = -O$(OPT)
CFLAGS += -std=gnu1x
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -Wall
CFLAGS += -Wstrict-prototypes
#CFLAGS += -Wextra
#CFLAGS += -Wpointer-arith
#CFLAGS += -Winline
#CFLAGS += -Wunreachable-code
#CFLAGS += -Wundef


#---------------- C++ Compiler Options ----------------
#
CXXFLAGS  = -O$(OPT)
CXXFLAGS += -ffunction-sections
CXXFLAGS += -fdata-sections
CXXFLAGS += -Wall

#---------------- Assembler Options ----------------
#  -Wa,...    tell GCC to pass this to the assembler
#

#---------------- Linker Options ----------------
#  -Wl,...      tell GCC to pass this to linker
#    -Map       create map file
#    --cref     add cross reference to  map file
#
LDFLAGS += -lm
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections

#============================================================================


# Define programs and commands
CC      = gcc
SIZE    = size
OBJDUMP = objdump
NM       = nm
MKDIR    = mkdir
GNUPLOT = "/cygdrive/c/Program Files (x86)/gnuplot/bin/wgnuplot.exe"

# Compiler flags to generate dependency files
#
GENDEPFLAGS = -MMD -MP

# Default target
#
all:  gccversion build showsize

build:  $(TARGET) lss sym
lss: $(TARGET).lss
sym: $(TARGET).sym


clean:
	@echo Cleaning project:
	rm -rf $(OBJDIR)

test.plt: $(TARGET)
	$(TARGET) > test.log

plot: test.plt
	$(GNUPLOT)  test.plt


# Display compiler version information
#
gccversion: 
	@$(CC) --version


# Show the final program size
#
showsize: build
	@echo
	@$(SIZE) $(TARGET) 2>/dev/null


# Link: create ELF output file from object files
#
$(TARGET): $(OBJECTS)
	@echo
	@echo Linking: $@
	@$(MKDIR) -p $(dir $@)
	$(CC) $(OBJECTS) $(LDFLAGS) --output $@ 

# Create extended listing file from ELF output file
#
%.lss: $(TARGET)
	@echo
	@echo Creating Extended Listing: $@
	@$(MKDIR) -p $(dir $@)
	$(OBJDUMP) -h -S -z $< > $@

# Create a symbol table from ELF output file
#
%.sym: $(TARGET)
	@echo
	@echo Creating Symbol Table: $@
	@$(MKDIR) -p $(dir $@)
	$(NM) -n $< > $@

# Compile: create object files from C source files
#
$(OBJDIR)/%.o : %.c
	@echo
	@echo Compiling C: $<
	@$(MKDIR) -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $(GENDEPFLAGS) $< -o $@ 

$(OBJDIR)/%.o : %.cpp
	@echo
	@echo Compiling C++: $<
	@$(MKDIR) -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CXXFLAGS) $(GENDEPFLAGS) $< -o $@ 

# Assemble: create object files from assembler source files
#
$(OBJDIR)/%.o : %.s
	@echo
	@echo Assembling: $<
	@$(MKDIR) -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(ASFLAGS) $(GENDEPFLAGS) $< -o $@

# Make everything depend on the makefile
#
$(OBJECTS): $(MAKEFILE_LIST)

# Include the dependency files
#
-include $(OBJECTS:.o=.d)

# Listing of phony targets
.PHONY: all clean plot
