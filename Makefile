# Optimization level, can be [0, 1, 2, 3, s]. 
#     0 = turn off optimization. s = optimize for size.
# 
OPT = -O2 -flto
BOARD ?= REV_A

# Object files directory
# Warning: this will be removed by make clean!
#
OBJDIR = obj_app

# Target file name (without extension)
TARGET = $(OBJDIR)/drquad

# Define all C source files (dependencies are generated automatically)
#
INCDIRS += Source

SOURCES += Source/main.c
SOURCES += Source/gpn_foo.c
SOURCES += Source/flight_ctrl.c
SOURCES += Source/bldc_driver.c
SOURCES += Source/bldc_task.c
SOURCES += Source/i2c_driver.c
SOURCES += Source/i2c_mpu9150.c
SOURCES += Source/i2c_ak8975.c
SOURCES += Source/i2c_bmp180.c
SOURCES += Source/sensors.c
SOURCES += Source/scope.c
SOURCES += Source/debug_dac.c

SOURCES += Source/dma_io_driver.c
SOURCES += Source/dma_io_ws2812.c
SOURCES += Source/dma_io_servo_in.c
SOURCES += Source/dma_io_servo_out.c

SOURCES += Source/rc_input.c
SOURCES += Source/rc_ppm.c
SOURCES += Source/rc_sbus.c
SOURCES += Source/rc_dsm2.c

SOURCES += Source/tetris.c

SOURCES += Source/board.c
SOURCES += Source/syscalls.c
SOURCES += Source/ustime.c
SOURCES += Source/term_xbee.c
SOURCES += Source/term_usb.c

SOURCES += Source/attitude.c
SOURCES += Source/command.c
SOURCES += Source/fault_handler.c
SOURCES += Source/led_task.c
SOURCES += Source/param_table.c
SOURCES += Source/parameter.c
SOURCES += Source/readline.c
SOURCES += Source/shell_task.c
SOURCES += Source/util.c
SOURCES += Source/filter.c
SOURCES += Source/watchdog.c

SOURCES += Source/version.c

# CRC functions
#
CRC_DIR = Libraries/crc
INCDIRS += $(CRC_DIR)
SOURCES += $(CRC_DIR)/crc16.c
SOURCES += $(CRC_DIR)/crc32.c

# FatFS
#
FATFS_DIR  = Libraries/FatFs-0.10c/src

INCDIRS   += $(FATFS_DIR)
SOURCES   += $(FATFS_DIR)/ff.c

# FreeRTOS
#
FREERTOS_BASE = Libraries/FreeRTOSV8.1.2
FREERTOS_DIR = $(FREERTOS_BASE)/FreeRTOS/Source

INCDIRS += $(FREERTOS_DIR)/include
INCDIRS += $(FREERTOS_DIR)/portable/GCC/ARM_CM4F

SOURCES += $(FREERTOS_DIR)/tasks.c
SOURCES += $(FREERTOS_DIR)/queue.c
SOURCES += $(FREERTOS_DIR)/list.c
SOURCES += $(FREERTOS_DIR)/croutine.c
SOURCES += $(FREERTOS_DIR)/portable/GCC/ARM_CM4F/port.c

# Tracealyzer
#
TRACE_DIR = $(FREERTOS_BASE)/FreeRTOS-Plus/Source/FreeRTOS-Plus-Trace
INCDIRS += $(TRACE_DIR)/Include

SOURCES += $(TRACE_DIR)/trcBase.c
SOURCES += $(TRACE_DIR)/trcKernel.c
SOURCES += $(TRACE_DIR)/trcUser.c
SOURCES += $(TRACE_DIR)/trcHardwarePort.c
SOURCES += $(TRACE_DIR)/trcKernelPort.c

# USB library stuff
#
USB_VCP_DIR = Libraries/STM32_USB_Device_VCP-1.1.0

INCDIRS += $(USB_VCP_DIR)/inc
SOURCES += $(USB_VCP_DIR)/src/usbd_desc.c
SOURCES += $(USB_VCP_DIR)/src/usbd_usr.c

USB_DEVICE_DIR = Libraries/STM32_USB_Device_Library-1.1.0

INCDIRS += $(USB_DEVICE_DIR)/Class/cdc/inc
SOURCES += $(USB_DEVICE_DIR)/Class/cdc/src/usbd_cdc_core.c

INCDIRS += $(USB_DEVICE_DIR)/Core/inc
SOURCES += $(USB_DEVICE_DIR)/Core/src/usbd_core.c
SOURCES += $(USB_DEVICE_DIR)/Core/src/usbd_ioreq.c
SOURCES += $(USB_DEVICE_DIR)/Core/src/usbd_req.c

USB_DRIVER_DIR = Libraries/STM32_USB_OTG_Driver-2.1.0

INCDIRS += $(USB_DRIVER_DIR)/inc
SOURCES += $(USB_DRIVER_DIR)/src/usb_core.c
SOURCES += $(USB_DRIVER_DIR)/src/usb_dcd.c
SOURCES += $(USB_DRIVER_DIR)/src/usb_dcd_int.c

# Standard peripheral library
#
CPPFLAGS += -DUSE_STDPERIPH_DRIVER
CPPFLAGS += -DUSE_FULL_ASSERT

STDPERIPH_DIR = Libraries/STM32F4xx_StdPeriph_Driver-1.4.0

INCDIRS += $(STDPERIPH_DIR)/inc

SOURCES += $(STDPERIPH_DIR)/src/misc.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_spi.c
SOURCES += $(STDPERIPH_DIR)/src/stm32f4xx_i2c.c
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
LDSCRIPT = Source/stm32f4xx_app.ld


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
CPPFLAGS += -g
# CPPFLAGS += -save-temps=obj

#---------------- C Compiler Options ----------------
#  -O*            optimization level
#  -f...          tuning, see GCC documentation
#  -Wall...       warning level
#
CFLAGS += $(OPT)
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

# LDFLAGS += -specs=nano.specs -u _printf_float -u _scanf_float
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
STLINK   = Tools/st-link/ST-LINK_CLI.exe

# Compiler flags to generate dependency files
#
GENDEPFLAGS = -MMD -MP

# Combine all necessary flags and optional flags
# Add target processor to flags.
#
# CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
CPU = -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -mfpu=fpv4-sp-d16

CFLAGS   += $(CPU)
CXXFLAGS += $(CPU)
ASFLAGS  += $(CPU)
LDFLAGS  += $(CPU)

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


boot:
	$(MAKE) -f Bootloader/Makefile

boot_clean:
	$(MAKE) -f Bootloader/Makefile clean

boot_flash:
	$(MAKE) -f Bootloader/Makefile flash


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
	$(STLINK) -c SWD -P $(TARGET).hex -Run

# Target: clean project
#
clean:
	@echo Cleaning project:
	rm -rf $(OBJDIR)
	rm -f Source/git_version.h
	
# Create version information file
#
Source/version.c: Source/git_version.h

Source/git_version.h: $(filter-out $(OBJDIR)/Source/version.o, $(OBJECTS))
	@echo
	@echo Generating Version Info: $@
	@$(MKDIR) -p $(dir $@)
	
	@echo "#pragma once" > $@
	@echo >> $@
	@echo "// AUTO GENERATED BY MAKEFILE" >> $@
	@echo "//" >> $@
	@echo "#define GIT_VERSION \"$(shell git describe --always --dirty)\"" >> $@ 
	@echo "#define BUILD_USER  \"$(shell id -un)\"" >> $@ 
	@echo "#define BUILD_HOST  \"$(shell hostname)\"" >> $@ 
	@echo "#define BUILD_DATE  \"$(shell date +%Y-%m-%d)\"" >> $@
	@echo "#define BUILD_TIME  \"$(shell date +%H:%M:%S)\"" >> $@


# Include the base rules
#
include base.mak

# Include the dependency files
#
-include $(OBJECTS:.o=.d)

# Listing of phony targets
#
.PHONY: all build flash clean \
        boot boot_clean boot_flash \
        doxygen elf lss sym \
        showsize gccversion
