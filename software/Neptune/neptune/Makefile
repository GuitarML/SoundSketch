# Project Name
TARGET = neptune

USE_DAISYSP_LGPL = 1

APP_TYPE = BOOT_SRAM

# Compiler options
#OPT = -O1
OPT=-Ofast


# Sources
CPP_SOURCES = neptune.cpp

# Library Locations
LIBDAISY_DIR = ../../../deps/libDaisy
DAISYSP_DIR = ../../../deps/DaisySP
CLOUDSEED_DIR = ../CloudSeed

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


# Include funbox.h
C_INCLUDES += -I../../../include -I../CloudSeed \

LIBS += -lcloudseed
LIBDIR += -L../CloudSeed/build 