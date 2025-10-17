TARGET := snake3ds
BUILD := build
SOURCE := source
INCLUDES := include
LIBS := -lcitro2d -lcitro3d -lctru -lm

ARCH := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS := -Wall -O2 -mword-relocations $(ARCH) $(INCLUDE) -DARM11 -D_3DS
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions

include $(DEVKITARM)/3ds_rules
