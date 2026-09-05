#---------------------------------------------------------------------------------
# Cell-8 - minimal PSL1GHT homebrew: PPU hello world + RSX clear/flip + intro screen
#---------------------------------------------------------------------------------

TARGET	:= Cell-8

OFILES	:= source/main.o source/rsxutil.o source/font/font5x7.o

INCLUDES := -Iinclude -I$(PS3DEV)/ppu/include

# -lio: needed for io/pad.h (detecting the intro-screen button press)
LIBS	 := -lrsx -lgcm_sys -lsysutil -lio -lrt -llv2 -lm

BUILDDIR := build

TITLE	:= Cell-8
APPID	:= CELL80001

# Local RPCS3 install, used by the "run" target below. Override on the
# command line (make RPCS3=/other/path run) if it ever moves.
RPCS3	?= /Users/ryanradder/projects/PS3_development/rpcs3-v0.0.42-19931-de6c1b52_macos_aarch64/RPCS3.app/Contents/MacOS/rpcs3

include $(PSL1GHT)/ppu_rules

# MACHDEP is only defined by ppu_rules (included above), so CFLAGS/CXXFLAGS
# must be assigned after the include -- assigning them earlier with ":="
# would have expanded $(MACHDEP) as empty.
CFLAGS	 := $(INCLUDES) -Wall $(MACHDEP)
CXXFLAGS := $(CFLAGS)
LDFLAGS	 := $(MACHDEP)

# Apple's bundled GNU Make 3.81 has a built-in default of LD=ld, which
# defeats base_rules' "LD ?= $(PREFIX)gcc" (the ?= only fires on an
# *unset* variable, and make's own built-in default counts as set).
# Force the PPU cross-linker explicitly. Use g++ since main.cpp/rsxutil.cpp
# are C++.
LD := ppu-g++

# ppu_rules' generic %.self pattern rule also generates a legacy
# "fake self" via the fself tool, for jailbroken-console fake-signed
# loading. On this ARM64 ps3dev build, fself crashes (bus error) on
# valid ELFs -- a toolchain bug unrelated to this app, and the fake
# self isn't needed to run under RPCS3. This explicit (non-pattern)
# rule takes precedence over that pattern rule and skips the fself step.
$(TARGET).self: $(TARGET).elf
	@echo CEX self ... $(notdir $@)
	@mkdir -p $(BUILDDIR)
	@$(STRIP) $< -o $(BUILDDIR)/$(notdir $<)
	@$(SPRX) $(BUILDDIR)/$(notdir $<)
	@$(SELF) $(BUILDDIR)/$(notdir $<) $@

all: $(TARGET).self

pkg: $(TARGET).pkg

run: $(TARGET).self
	"$(RPCS3)" "$(CURDIR)/$(TARGET).self"

clean:
	rm -rf $(TARGET).elf $(TARGET).self $(TARGET).pkg $(TARGET)*.gnpdrm.pkg $(TARGET).fake.self $(BUILDDIR) $(OFILES)
