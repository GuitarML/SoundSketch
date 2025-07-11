# Project Name
TARGET = earth

CPP_STANDARD = -std=c++20

#APP_TYPE = BOOT_SRAM

USE_DAISYSP_LGPL = 1

# Compiler options
#OPT = -O1
OPT=-Ofast

# Sources
CPP_SOURCES = earth.cpp
CPP_SOURCES += Dattorro/dsp/filters/OnePoleFilters.cpp
CPP_SOURCES += Dattorro/dsp/delays/InterpDelay.cpp
CPP_SOURCES += Dattorro/Dattorro.cpp

# Library Locations
LIBDAISY_DIR = ../../deps/libDaisy
DAISYSP_DIR = ../../deps/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile

C_INCLUDES += -I../../deps/q/q_lib/include
C_INCLUDES += -I../../deps/gcem/include
C_INCLUDES += -I../../deps/infra/include

# Include soundsketch.h
C_INCLUDES += -I../../include

