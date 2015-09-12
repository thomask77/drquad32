# Optimization level, can be [0, 1, 2, 3, s]. 
#     0 = turn off optimization. s = optimize for size.
# 

# OPT = -O1         # for debugging
OPT = -Os

BOARD ?= REV_A

FLASH_ADDR = 0x08000000

# Object files directory
# Warning: this will be removed by make clean!
#
OBJDIR = obj_boot

# Target file name (without extension)
TARGET = $(OBJDIR)/f00b00t

# Define all C source files (dependencies are generated automatically)
#
INCDIRS += Bootloader
INCDIRS += Shared

SOURCES += Bootloader/syscalls.c
SOURCES += Bootloader/main.c
SOURCES += Bootloader/board.c
SOURCES += Bootloader/uart.c
SOURCES += Bootloader/msg_packet.c
SOURCES += Bootloader/version.c

SOURCES += Shared/cobsr.c
SOURCES += Shared/errors.c
SOURCES += Shared/crc16.c
SOURCES += Shared/crc32.c

# Standard peripheral library
#
CPPFLAGS += -DUSE_STDPERIPH_DRIVER
CPPFLAGS += -mfp16-format=ieee
# CPPFLAGS += -DUSE_FULL_ASSERT

STDPERIPH_DIR = Libraries/STM32F4xx_StdPeriph_Driver-1.4.0

INCDIRS += $(STDPERIPH_DIR)/inc

SOURCES += $(STDPERIPH_DIR)/src/misc.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_spi.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_dma.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_adc.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_dac.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_exti.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_iwdg.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_rcc.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_usart.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_gpio.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_flash.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_tim.c

# CMSIS-Library
#
CMSIS_DIR = Libraries/CMSIS-3.2.0

INCDIRS += $(CMSIS_DIR)/Include
INCDIRS += $(CMSIS_DIR)/Device/ST/STM32F4xx/Include

SOURCES += $(CMSIS_DIR)/Device/ST/STM32F4xx/Source/startup_stm32f40_41xxx.s

# Board support
#
ifeq ($(BOARD), DISCOVERY)
    CPPFLAGS += -DBOARD_DISCOVERY
    CPPFLAGS += -DHSE_VALUE=8000000
    SOURCES  += $(CMSIS_DIR)/Device/ST/STM32F4xx/Source/system_stm32f4xx_8_168.c
else
ifeq ($(BOARD), OLIMEX)
    CPPFLAGS += -DBOARD_REV_A
    CPPFLAGS += -DHSE_VALUE=12000000
    SOURCES  += $(CMSIS_DIR)/Device/ST/STM32F4xx/Source/system_stm32f4xx_12_168.c
else
ifeq ($(BOARD), REV_A)
    CPPFLAGS += -DBOARD_REV_A
    CPPFLAGS += -DHSE_VALUE=16000000
    SOURCES  += $(CMSIS_DIR)/Device/ST/STM32F4xx/Source/system_stm32f4xx_16_168.c
else
    $(error Unknown BOARD type: must be DISCOVERY or REV_A)
endif
endif
endif

CPPFLAGS += -DSTM32F40_41xxx
LDSCRIPT = Bootloader/stm32f4xx_boot.ld


#============================================================================
#
OBJECTS  += $(addprefix $(OBJDIR)/,$(addsuffix .o,$(basename $(SOURCES))))
CPPFLAGS += $(addprefix -I,$(INCDIRS))

#---------------- Preprocessor Options ----------------
#  -fsingle...    make better use of the single-precision FPU
#  -g             generate debugging information
#  -save-temps    preserve .s and .i-files
#
CPPFLAGS += -fsingle-precision-constant
CPPFLAGS += -mfp16-format=ieee
CPPFLAGS += -g
# CPPFLAGS += -save-temps=obj

#---------------- C Compiler Options ----------------
#  -O*            optimization level
#  -f...          tuning, see GCC documentation
#  -Wall...       warning level
#
CFLAGS += $(OPT)
CFLAGS += -std=gnu11
CFLAGS += -ffunction-sections
CFLAGS += -fdata-sections
CFLAGS += -Wall
CFLAGS += -Wstrict-prototypes
#CFLAGS += -Wextra
#CFLAGS += -Wpointer-arith
#CFLAGS += -Winline
#CFLAGS += -Wunreachable-code
#CFLAGS += -Wundef

# Use a friendly C dialect
CPPFLAGS += -fno-strict-aliasing
CPPFLAGS += -fwrapv

#---------------- C++ Compiler Options ----------------
#
CXXFLAGS += $(OPT)
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
LDFLAGS += $(OPT)
LDFLAGS += -lm
LDFLAGS += -Wl,-Map=$(TARGET).map,--cref
LDFLAGS += -Wl,--gc-sections

LDFLAGS += -specs=nano.specs        # -u _printf_float -u _scanf_float
LDFLAGS += -T$(LDSCRIPT)

#============================================================================

# Define programs and commands
#
TOOLCHAIN = arm-none-eabi-
CC       = $(TOOLCHAIN)gcc
OBJCOPY  = $(TOOLCHAIN)objcopy
OBJDUMP  = $(TOOLCHAIN)objdump
SIZE     = $(TOOLCHAIN)size
NM       = $(TOOLCHAIN)nm
MKDIR    = mkdir
DOXYGEN  = doxygen
STLINK_WIN = Tools/st-link/ST-LINK_CLI.exe
STLINK_LNX = st-flash
POSTLD   = Tools/add_version_info/add_version_info.py -v

# Compiler flags to generate dependency files
#
GENDEPFLAGS = -MMD -MP

# Combine all necessary flags and optional flags
# Add target processor to flags.
#
CPU = -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16

CFLAGS   += $(CPU)
CXXFLAGS += $(CPU)
ASFLAGS  += $(CPU)
LDFLAGS  += $(CPU)

ifeq ($(OS),Windows_NT)
	STLINK = $(STLINK_WIN) -c SWD -P $(TARGET).hex -Run
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		STLINK = $(STLINK_LNX) --reset write $(TARGET).bin $(FLASH_ADDR)
	endif
	ifeq ($(UNAME_S),Darwin)
		$(error Don't know how to flash under OSX)
	endif
endif

# Default target
#
all:  gccversion build showsize

build: elf hex bin lss sym

elf: $(TARGET).elf
hex: $(TARGET).hex
bin: $(TARGET).bin
lss: $(TARGET).lss
sym: $(TARGET).sym


doxygen:
	@echo
	@echo Creating Doxygen documentation
	@$(DOXYGEN)


# Display compiler version information
#
gccversion: 
	@$(CC) --version


# Show the final program size
#
showsize: build
	@echo
	@$(SIZE) $(TARGET).elf 2>/dev/null


# Flash the device  
#
flash: build showsize
	$(STLINK)

# Target: clean project
#
clean:
	@echo Cleaning project:
	rm -rf $(OBJDIR)

# Include the base rules
#
include base.mak

# Include the dependency files
#
-include $(OBJECTS:.o=.d)

# Listing of phony targets
#
.PHONY: all build flash clean \
        doxygen elf lss sym \
        showsize gccversion

