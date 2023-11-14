# File: Standard Makefile for CSC415
#
# Description - This make file should be used for all your projects
# It should be modified as needed for each homework
#
# ROOTNAME should be set you your lastname_firstname_HW.  Except for
# and group projects, this will not change throughout the semester
#
# HW should be set to the assignment number (i.e. 1, 2, 3, etc.)
#
# FOPTION can be set to blank (nothing) or to any thing starting with an 
# underscore (_).  This is the suffix of your file name.
#
# With these three options above set your filename for your homework
# assignment will look like:  bierman_robert_HW1_main.c 
#
# RUNOPTIONS can be set to default values you want passed into the program
# this can also be overridden on the command line
#
# OBJ - You can append to this line for additional files necessary for
# your program, but only when you have multiple files.  Follow the convention
# but hard code the suffix as needed.
#
# To Use the Makefile - Edit as above
# then from the command line run:  make
# That command will build your program, and the program will be named the same
# as your main c file without an extension.
#
# You can then execute from the command line: make run
# This will actually run your program
#
# Using the command: make clean
# will delete the executable and any object files in your directory.
#


ROOTNAME=fsshell
BUILDPATH=build/
TESTPATH=tests/
OBJECTPATH=build/objects/
SRCPATH=src/
DEPSPATH=build/deps/

HW=
FOPTION=
RUNOPTIONS=SampleVolume 10000000 512
CC=gcc
CFLAGS=-g -I.
LIBS=pthread
DEPS= 
# Add any additional objects to this list
ADDOBJ=fsInit.o mfs.o
ARCH=$(shell uname -m)

ifeq ($(ARCH), aarch64)
	ARCHOBJ=fsLowM1.o
else
	ARCHOBJ=fsLow.o
endif

# Create all object files
# Ex: OBJ = build/fsshell.o build/fsInit.o build/mfs.o fsLow.o 
OBJ = $(OBJECTPATH)$(ROOTNAME)$(HW)$(FOPTION).o $(addprefix $(OBJECTPATH),$(ADDOBJ)) $(ARCHOBJ)

# Locate all c files in src and create o files in build/objects/ and
# Locate all dependencies in build/deps/ and create o files in build/objects
# $@ is replaced by the target name
# $< is the name of the first prerequisite
$(OBJECTPATH)%.o: $(addprefix $(SRCPATH),%.c) $(addprefix $(DEPSPATH),$(DEPS))
	$(CC) -c -o $@ $< $(CFLAGS) 

# Create our executable in build/
# $^ is replaced by all prerequisites. $^ is replaced by $(OBJ) which is a string containing
# All object files
# Change path of object file to go into ./objects
$(BUILDPATH)$(ROOTNAME)$(HW)$(FOPTION): $(OBJ)
	@echo objects: $(OBJ)
	@echo all stuff: $^
	$(CC) -o $@ $^ $(CFLAGS) -lm -l readline -l $(LIBS)

clean:
	rm $(OBJECTPATH)$(ROOTNAME)$(HW)$(FOPTION).o $(addprefix $(OBJECTPATH),$(ADDOBJ)) $(BUILDPATH)$(ROOTNAME)$(HW)$(FOPTION)

run: $(BUILDPATH)$(ROOTNAME)$(HW)$(FOPTION)
# @mkdir -p $(BUILDPATH)
	./$(BUILDPATH)$(ROOTNAME)$(HW)$(FOPTION) $(RUNOPTIONS)
	
vrun: $(ROOTNAME)$(HW)$(FOPTION)
	valgrind ./$(BUILDPATH)$(ROOTNAME)$(HW)$(FOPTION) $(RUNOPTIONS)
