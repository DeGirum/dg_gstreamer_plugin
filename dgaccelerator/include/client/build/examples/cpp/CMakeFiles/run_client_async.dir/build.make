# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.25

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/stephan/miniconda3/lib/python3.10/site-packages/cmake/data/bin/cmake

# The command to remove a file.
RM = /home/stephan/miniconda3/lib/python3.10/site-packages/cmake/data/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/stephan/Documents/Dev/CppSDK

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/stephan/Documents/Dev/CppSDK/client/build

# Include any dependencies generated for this target.
include examples/cpp/CMakeFiles/run_client_async.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include examples/cpp/CMakeFiles/run_client_async.dir/compiler_depend.make

# Include the progress variables for this target.
include examples/cpp/CMakeFiles/run_client_async.dir/progress.make

# Include the compile flags for this target's objects.
include examples/cpp/CMakeFiles/run_client_async.dir/flags.make

examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o: examples/cpp/CMakeFiles/run_client_async.dir/flags.make
examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o: /home/stephan/Documents/Dev/CppSDK/examples/cpp/dg_core_client_async.cpp
examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o: examples/cpp/CMakeFiles/run_client_async.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/stephan/Documents/Dev/CppSDK/client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o"
	cd /home/stephan/Documents/Dev/CppSDK/client/build/examples/cpp && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o -MF CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o.d -o CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o -c /home/stephan/Documents/Dev/CppSDK/examples/cpp/dg_core_client_async.cpp

examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.i"
	cd /home/stephan/Documents/Dev/CppSDK/client/build/examples/cpp && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/stephan/Documents/Dev/CppSDK/examples/cpp/dg_core_client_async.cpp > CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.i

examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.s"
	cd /home/stephan/Documents/Dev/CppSDK/client/build/examples/cpp && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/stephan/Documents/Dev/CppSDK/examples/cpp/dg_core_client_async.cpp -o CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.s

# Object files for target run_client_async
run_client_async_OBJECTS = \
"CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o"

# External object files for target run_client_async
run_client_async_EXTERNAL_OBJECTS =

/home/stephan/Documents/Dev/CppSDK/bin/run_client_async: examples/cpp/CMakeFiles/run_client_async.dir/dg_core_client_async.cpp.o
/home/stephan/Documents/Dev/CppSDK/bin/run_client_async: examples/cpp/CMakeFiles/run_client_async.dir/build.make
/home/stephan/Documents/Dev/CppSDK/bin/run_client_async: /home/stephan/Documents/Dev/CppSDK/bin/libaiclientlib.a
/home/stephan/Documents/Dev/CppSDK/bin/run_client_async: examples/cpp/CMakeFiles/run_client_async.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/stephan/Documents/Dev/CppSDK/client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable /home/stephan/Documents/Dev/CppSDK/bin/run_client_async"
	cd /home/stephan/Documents/Dev/CppSDK/client/build/examples/cpp && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/run_client_async.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
examples/cpp/CMakeFiles/run_client_async.dir/build: /home/stephan/Documents/Dev/CppSDK/bin/run_client_async
.PHONY : examples/cpp/CMakeFiles/run_client_async.dir/build

examples/cpp/CMakeFiles/run_client_async.dir/clean:
	cd /home/stephan/Documents/Dev/CppSDK/client/build/examples/cpp && $(CMAKE_COMMAND) -P CMakeFiles/run_client_async.dir/cmake_clean.cmake
.PHONY : examples/cpp/CMakeFiles/run_client_async.dir/clean

examples/cpp/CMakeFiles/run_client_async.dir/depend:
	cd /home/stephan/Documents/Dev/CppSDK/client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/stephan/Documents/Dev/CppSDK /home/stephan/Documents/Dev/CppSDK/examples/cpp /home/stephan/Documents/Dev/CppSDK/client/build /home/stephan/Documents/Dev/CppSDK/client/build/examples/cpp /home/stephan/Documents/Dev/CppSDK/client/build/examples/cpp/CMakeFiles/run_client_async.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : examples/cpp/CMakeFiles/run_client_async.dir/depend
