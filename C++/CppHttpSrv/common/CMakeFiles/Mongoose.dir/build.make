# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/yingww/GitProj/Learning/C++/CppHttpSrv

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/yingww/GitProj/Learning/C++/CppHttpSrv

# Include any dependencies generated for this target.
include common/CMakeFiles/Mongoose.dir/depend.make

# Include the progress variables for this target.
include common/CMakeFiles/Mongoose.dir/progress.make

# Include the compile flags for this target's objects.
include common/CMakeFiles/Mongoose.dir/flags.make

common/CMakeFiles/Mongoose.dir/mongoose.c.o: common/CMakeFiles/Mongoose.dir/flags.make
common/CMakeFiles/Mongoose.dir/mongoose.c.o: common/mongoose.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/yingww/GitProj/Learning/C++/CppHttpSrv/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object common/CMakeFiles/Mongoose.dir/mongoose.c.o"
	cd /home/yingww/GitProj/Learning/C++/CppHttpSrv/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/Mongoose.dir/mongoose.c.o   -c /home/yingww/GitProj/Learning/C++/CppHttpSrv/common/mongoose.c

common/CMakeFiles/Mongoose.dir/mongoose.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/Mongoose.dir/mongoose.c.i"
	cd /home/yingww/GitProj/Learning/C++/CppHttpSrv/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/yingww/GitProj/Learning/C++/CppHttpSrv/common/mongoose.c > CMakeFiles/Mongoose.dir/mongoose.c.i

common/CMakeFiles/Mongoose.dir/mongoose.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/Mongoose.dir/mongoose.c.s"
	cd /home/yingww/GitProj/Learning/C++/CppHttpSrv/common && /usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/yingww/GitProj/Learning/C++/CppHttpSrv/common/mongoose.c -o CMakeFiles/Mongoose.dir/mongoose.c.s

common/CMakeFiles/Mongoose.dir/mongoose.c.o.requires:

.PHONY : common/CMakeFiles/Mongoose.dir/mongoose.c.o.requires

common/CMakeFiles/Mongoose.dir/mongoose.c.o.provides: common/CMakeFiles/Mongoose.dir/mongoose.c.o.requires
	$(MAKE) -f common/CMakeFiles/Mongoose.dir/build.make common/CMakeFiles/Mongoose.dir/mongoose.c.o.provides.build
.PHONY : common/CMakeFiles/Mongoose.dir/mongoose.c.o.provides

common/CMakeFiles/Mongoose.dir/mongoose.c.o.provides.build: common/CMakeFiles/Mongoose.dir/mongoose.c.o


# Object files for target Mongoose
Mongoose_OBJECTS = \
"CMakeFiles/Mongoose.dir/mongoose.c.o"

# External object files for target Mongoose
Mongoose_EXTERNAL_OBJECTS =

common/libMongoose.a: common/CMakeFiles/Mongoose.dir/mongoose.c.o
common/libMongoose.a: common/CMakeFiles/Mongoose.dir/build.make
common/libMongoose.a: common/CMakeFiles/Mongoose.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/yingww/GitProj/Learning/C++/CppHttpSrv/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking C static library libMongoose.a"
	cd /home/yingww/GitProj/Learning/C++/CppHttpSrv/common && $(CMAKE_COMMAND) -P CMakeFiles/Mongoose.dir/cmake_clean_target.cmake
	cd /home/yingww/GitProj/Learning/C++/CppHttpSrv/common && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/Mongoose.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
common/CMakeFiles/Mongoose.dir/build: common/libMongoose.a

.PHONY : common/CMakeFiles/Mongoose.dir/build

common/CMakeFiles/Mongoose.dir/requires: common/CMakeFiles/Mongoose.dir/mongoose.c.o.requires

.PHONY : common/CMakeFiles/Mongoose.dir/requires

common/CMakeFiles/Mongoose.dir/clean:
	cd /home/yingww/GitProj/Learning/C++/CppHttpSrv/common && $(CMAKE_COMMAND) -P CMakeFiles/Mongoose.dir/cmake_clean.cmake
.PHONY : common/CMakeFiles/Mongoose.dir/clean

common/CMakeFiles/Mongoose.dir/depend:
	cd /home/yingww/GitProj/Learning/C++/CppHttpSrv && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/yingww/GitProj/Learning/C++/CppHttpSrv /home/yingww/GitProj/Learning/C++/CppHttpSrv/common /home/yingww/GitProj/Learning/C++/CppHttpSrv /home/yingww/GitProj/Learning/C++/CppHttpSrv/common /home/yingww/GitProj/Learning/C++/CppHttpSrv/common/CMakeFiles/Mongoose.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : common/CMakeFiles/Mongoose.dir/depend

