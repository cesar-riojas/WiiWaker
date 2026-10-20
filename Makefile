#---------------------------------------------------------------------------------
# WiiWaker - Native Nintendo Wii Port of The Legend of Zelda: The Wind Waker
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to devkitPPC>")
endif

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to devkitPro>")
endif

export TARGET   := WiiWaker
export BUILD    := build
export SOURCES  := wii/src game/src
export INCLUDES := wii/include game/include $(DEVKITPRO)/libogc/include

PREFIX := $(DEVKITPPC)/bin/powerpc-eabi-
CC     := $(PREFIX)gcc
CXX    := $(PREFIX)g++
AR     := $(PREFIX)ar
OBJCOPY:= $(PREFIX)objcopy
ELF2DOL:= $(DEVKITPRO)/tools/bin/elf2dol

MACHDEP := -DGEKKO -D__wii__ -mrvl -mcpu=750 -meabi -mhard-float
CFLAGS  := -O2 -Wall $(MACHDEP) $(foreach dir,$(INCLUDES),-I$(dir))
CXXFLAGS:= $(CFLAGS) -std=c++17 -fno-rtti -fno-exceptions

LIBPATHS := -L$(DEVKITPRO)/libogc/lib/wii
LIBS     := -lwiiuse -lbte -lfat -logc -lm

WII_SRCS := $(wildcard wii/src/*.cpp)
WII_OBJS := $(patsubst wii/src/%.cpp,$(BUILD)/wii/%.o,$(WII_SRCS))

all: $(TARGET).dol

$(BUILD)/wii/%.o: wii/src/%.cpp
	@mkdir -p $(dir $@)
	@echo " [CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).elf: $(WII_OBJS)
	@echo " [LD]  $@"
	@$(CXX) $(CFLAGS) $(WII_OBJS) $(LIBPATHS) $(LIBS) -o $@

$(TARGET).dol: $(TARGET).elf
	@echo " [DOL] $@"
	@$(ELF2DOL) $< $@
	@echo "==> Build complete: $(TARGET).dol"

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).dol

.PHONY: all clean
