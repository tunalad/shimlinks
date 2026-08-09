# shimlinks version
VERSION = 0.1.0

# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# compiler and flags
CXX = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -MMD -MP -DVERSION=\"$(VERSION)\"
LDFLAGS =

# debug (uncomment)
#CXXFLAGS += -g
