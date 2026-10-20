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
export INCLUDES := wii/include wii/include/msl game/include $(DEVKITPRO)/libogc/include

PREFIX := $(DEVKITPPC)/bin/powerpc-eabi-
CC     := $(PREFIX)gcc
CXX    := $(PREFIX)g++
AR     := $(PREFIX)ar
OBJCOPY:= $(PREFIX)objcopy
ELF2DOL:= $(DEVKITPRO)/tools/bin/elf2dol

MACHDEP      := -DGEKKO -D__wii__ -mrvl -mcpu=750 -meabi -mhard-float -O2
COMMON_FLAGS := $(MACHDEP) -include $(CURDIR)/wii/include/wii_config.h $(foreach dir,$(INCLUDES),-I$(dir))
CFLAGS       := $(COMMON_FLAGS) -Wno-narrowing -Wno-multichar -Wno-write-strings
CXXFLAGS     := $(CFLAGS) -std=c++17 -fno-rtti -fno-exceptions -Wno-register -Wno-deprecated-declarations

LIBPATHS := -L$(DEVKITPRO)/libogc/lib/wii
LIBS     := -lwiiuse -lbte -lfat -logc -lm

# Wii Platform Layer Sources
WII_CXX_SRCS := $(wildcard wii/src/*.cpp)
WII_CXX_OBJS := $(patsubst wii/src/%.cpp,$(BUILD)/wii/%.o,$(WII_CXX_SRCS))

# Game Engine Integration Sources
GAME_CXX_SRCS := game/src/SSystem/SComponent/c_lib.cpp

GAME_OBJS := $(patsubst game/src/%.cpp,$(BUILD)/game/%.o,$(GAME_CXX_SRCS))

ALL_OBJS := $(WII_CXX_OBJS) $(GAME_OBJS)

all: $(TARGET).dol dist_package

$(BUILD)/wii/%.o: wii/src/%.cpp
	@mkdir -p $(dir $@)
	@echo " [CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/game/%.o: game/src/%.c
	@mkdir -p $(dir $@)
	@echo " [CC]  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/game/%.o: game/src/%.cpp
	@mkdir -p $(dir $@)
	@echo " [CXX] $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET).elf: $(ALL_OBJS)
	@echo " [LD]  $@"
	@$(CXX) $(CFLAGS) $(ALL_OBJS) $(LIBPATHS) $(LIBS) -o $@

$(TARGET).dol: $(TARGET).elf
	@echo " [DOL] $@"
	@$(ELF2DOL) $< $@
	@echo "==> Build complete: $(TARGET).dol"

dist_package: $(TARGET).dol
	@mkdir -p dist/apps/WiiWaker
	@cp $(TARGET).dol dist/apps/WiiWaker/boot.dol
	@cp tools/meta.xml dist/apps/WiiWaker/meta.xml
	@echo "==> SD Card Package updated at: dist/apps/WiiWaker/boot.dol"

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).dol

.PHONY: all clean dist_package
