ROOTDIR := ./
BUILDTYPE ?= debug
BINDIR := $(ROOTDIR)bin-$(BUILDTYPE)
OBJDIR := $(ROOTDIR)obj-$(BUILDTYPE)

PREFIX ?= 

ifeq ($(OS), Windows_NT)
	# Tool binary extension
	BINEXT = .exe
	md = C:\\msys64\\usr\\bin\\mkdir.exe -p $(1)
else
	md = mkdir -p $(1)
endif

DelocaliseEXE := $(BINDIR)/RmkDelocalise$(BINEXT)
TxtXyzEXE := $(BINDIR)/TxtXyz$(BINEXT)
RouteDecodeEXE := $(BINDIR)/RouteDecode$(BINEXT)

CC := $(PREFIX)gcc$(BINEXT)
CXX := $(PREFIX)g++$(BINEXT)

CFLAGS = \
	-fdiagnostics-color=always \
	-MMD -MP

LDFLAGS = \
	-lz

ifeq ($(BUILDTYPE), debug)
	CFLAGS += \
	-Og \
	-g
endif

# LTO has broken???

ifeq ($(BUILDTYPE), release)
	CFLAGS += \
	-O3 -Wa,--strip-local-absolute \
	-fdata-sections \
	-ffunction-sections
	LDFLAGS += \
	-static \
	-Wl,-gc-sections \
	-static-libgcc \
	-static-libstdc++
endif

include lcf.mk

DelocaliseSRCS = \
	utf8.rc \
	RmkDelocalise/md5.c \
	RmkDelocalise/RmkDelocalise.cpp \
	RmkDelocalise/find.cpp \
	RmkDelocalise/filetimes.cpp \
	$(liblcf_SRCS)
TxtXyzSRCS = \
	utf8.rc \
	TxtXyz/main.c
RouteDecodeSRCS = \
	utf8.rc \
	RouteDecode/main.cpp \
	$(liblcf_SRCS)
	
# in theory, expanded when needed?
DelocaliseOBJS = $(DelocaliseSRCS:%=$(OBJDIR)/%.o)
TxtXyzOBJS = $(TxtXyzSRCS:%=$(OBJDIR)/%.o)
RouteDecodeOBJS = $(RouteDecodeSRCS:%=$(OBJDIR)/%.o)

# i forgot if this is necessary
# Add more lines when i make more tools
DEPS += $(DelocaliseSRCS:%=$(OBJDIR)/%.d)
DEPS += $(TxtXyzSRCS:%=$(OBJDIR)/%.d)
DEPS += $(RouteDecodeSRCS:%=$(OBJDIR)/%.d)

.PHONY: clean

all: $(DelocaliseEXE) $(TxtXyzEXE) $(RouteDecodeEXE)

$(DelocaliseEXE): $(DelocaliseOBJS)
	$(call md,$(dir $@))
	$(CXX) $(DelocaliseOBJS) -o $@ $(LDFLAGS)

$(TxtXyzEXE): $(TxtXyzOBJS)
	$(call md,$(dir $@))
	$(CXX) $(TxtXyzOBJS) -o $@ $(LDFLAGS)
	
$(RouteDecodeEXE): $(RouteDecodeOBJS)
	$(call md,$(dir $@))
	$(CXX) $(RouteDecodeOBJS) -o $@ $(LDFLAGS)

$(OBJDIR)/%.c.o: %.c
	$(call md,$(dir $@))
	$(CC) $(CFLAGS) -std=gnu17 -c $< -o $@

$(OBJDIR)/%.cpp.o: %.cpp
	$(call md,$(dir $@))
	$(CXX) $(CFLAGS) $(CXXFLAGS) -std=c++17 -fpermissive -c $< -o $@
$(OBJDIR)/%.rc.o: %.rc
	$(call md,$(dir $@))
	windres $< -o $@
	
clean:
	-rm -rf $(OBJDIR)
	-rm -rf $(BINDIR)
	
# i yoinked this makefile from a dead project i don't know if this actually works
-include $(DEPS)