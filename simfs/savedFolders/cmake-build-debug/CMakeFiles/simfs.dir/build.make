# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.12

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
CMAKE_COMMAND = /home/samier/.local/share/Trash/files/clion-2018.3/bin/cmake/linux/bin/cmake

# The command to remove a file.
RM = /home/samier/.local/share/Trash/files/clion-2018.3/bin/cmake/linux/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug

# Include any dependencies generated for this target.
include CMakeFiles/simfs.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/simfs.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/simfs.dir/flags.make

CMakeFiles/simfs.dir/test_simfs.c.o: CMakeFiles/simfs.dir/flags.make
CMakeFiles/simfs.dir/test_simfs.c.o: ../test_simfs.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building C object CMakeFiles/simfs.dir/test_simfs.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/simfs.dir/test_simfs.c.o   -c /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/test_simfs.c

CMakeFiles/simfs.dir/test_simfs.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/simfs.dir/test_simfs.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/test_simfs.c > CMakeFiles/simfs.dir/test_simfs.c.i

CMakeFiles/simfs.dir/test_simfs.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/simfs.dir/test_simfs.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/test_simfs.c -o CMakeFiles/simfs.dir/test_simfs.c.s

CMakeFiles/simfs.dir/simfs.c.o: CMakeFiles/simfs.dir/flags.make
CMakeFiles/simfs.dir/simfs.c.o: ../simfs.c
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building C object CMakeFiles/simfs.dir/simfs.c.o"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -o CMakeFiles/simfs.dir/simfs.c.o   -c /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/simfs.c

CMakeFiles/simfs.dir/simfs.c.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing C source to CMakeFiles/simfs.dir/simfs.c.i"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -E /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/simfs.c > CMakeFiles/simfs.dir/simfs.c.i

CMakeFiles/simfs.dir/simfs.c.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling C source to assembly CMakeFiles/simfs.dir/simfs.c.s"
	/usr/bin/cc $(C_DEFINES) $(C_INCLUDES) $(C_FLAGS) -S /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/simfs.c -o CMakeFiles/simfs.dir/simfs.c.s

# Object files for target simfs
simfs_OBJECTS = \
"CMakeFiles/simfs.dir/test_simfs.c.o" \
"CMakeFiles/simfs.dir/simfs.c.o"

# External object files for target simfs
simfs_EXTERNAL_OBJECTS =

bin/simfs: CMakeFiles/simfs.dir/test_simfs.c.o
bin/simfs: CMakeFiles/simfs.dir/simfs.c.o
bin/simfs: CMakeFiles/simfs.dir/build.make
bin/simfs: /usr/lib/x86_64-linux-gnu/libfuse.so
bin/simfs: CMakeFiles/simfs.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking C executable bin/simfs"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/simfs.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/simfs.dir/build: bin/simfs

.PHONY : CMakeFiles/simfs.dir/build

CMakeFiles/simfs.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/simfs.dir/cmake_clean.cmake
.PHONY : CMakeFiles/simfs.dir/clean

CMakeFiles/simfs.dir/depend:
	cd /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug /home/samier/Desktop/School/Fall2018/Operating-Systems/Project1/simfs/cmake-build-debug/CMakeFiles/simfs.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/simfs.dir/depend

