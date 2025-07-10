# Project Name
TARGET = mercury
#CPP_STANDARD = -std=c++20
CPP_STANDARD = -std=gnu++20
USE_DAISYSP_LGPL = 1

APP_TYPE = BOOT_SRAM

# Compiler options
#OPT = -O1
OPT=-Ofast

# Sources
CPP_SOURCES = mercury.cpp

# Library Locations
LIBDAISY_DIR = ../../libDaisy
DAISYSP_DIR = ../../DaisySP


# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile


# Include funbox.h
C_INCLUDES += -I../../infra/include
C_INCLUDES += -I../../q/q_lib/include
C_INCLUDES += -INeuralAudio
C_INCLUDES += -I../../include
C_INCLUDES += -I../../eigen
#C_INCLUDES += -INeuralAudio/deps/math_approx/include/math_approx
#C_INCLUDES += -I../../include -I../../RTNeural
#CPPFLAGS += -DRTNEURAL_DEFAULT_ALIGNMENT=8 -DRTNEURAL_NO_DEBUG=1 -DRTNEURAL_USE_EIGEN=1