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
include CMakeFiles/aiclientlib.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include CMakeFiles/aiclientlib.dir/compiler_depend.make

# Include the progress variables for this target.
include CMakeFiles/aiclientlib.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/aiclientlib.dir/flags.make

CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o: CMakeFiles/aiclientlib.dir/flags.make
CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o: /home/stephan/Documents/Dev/CppSDK/client/dg_client.cpp
CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o: CMakeFiles/aiclientlib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/stephan/Documents/Dev/CppSDK/client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o -MF CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o.d -o CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o -c /home/stephan/Documents/Dev/CppSDK/client/dg_client.cpp

CMakeFiles/aiclientlib.dir/client/dg_client.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/aiclientlib.dir/client/dg_client.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/stephan/Documents/Dev/CppSDK/client/dg_client.cpp > CMakeFiles/aiclientlib.dir/client/dg_client.cpp.i

CMakeFiles/aiclientlib.dir/client/dg_client.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/aiclientlib.dir/client/dg_client.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/stephan/Documents/Dev/CppSDK/client/dg_client.cpp -o CMakeFiles/aiclientlib.dir/client/dg_client.cpp.s

CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o: CMakeFiles/aiclientlib.dir/flags.make
CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o: /home/stephan/Documents/Dev/CppSDK/client/dg_model_api.cpp
CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o: CMakeFiles/aiclientlib.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/stephan/Documents/Dev/CppSDK/client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Building CXX object CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o -MF CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o.d -o CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o -c /home/stephan/Documents/Dev/CppSDK/client/dg_model_api.cpp

CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.i"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/stephan/Documents/Dev/CppSDK/client/dg_model_api.cpp > CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.i

CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.s"
	/usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/stephan/Documents/Dev/CppSDK/client/dg_model_api.cpp -o CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.s

# Object files for target aiclientlib
aiclientlib_OBJECTS = \
"CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o" \
"CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o"

# External object files for target aiclientlib
aiclientlib_EXTERNAL_OBJECTS =

/home/stephan/Documents/Dev/CppSDK/bin/libaiclientlib.a: CMakeFiles/aiclientlib.dir/client/dg_client.cpp.o
/home/stephan/Documents/Dev/CppSDK/bin/libaiclientlib.a: CMakeFiles/aiclientlib.dir/client/dg_model_api.cpp.o
/home/stephan/Documents/Dev/CppSDK/bin/libaiclientlib.a: CMakeFiles/aiclientlib.dir/build.make
/home/stephan/Documents/Dev/CppSDK/bin/libaiclientlib.a: CMakeFiles/aiclientlib.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/stephan/Documents/Dev/CppSDK/client/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Linking CXX static library /home/stephan/Documents/Dev/CppSDK/bin/libaiclientlib.a"
	$(CMAKE_COMMAND) -P CMakeFiles/aiclientlib.dir/cmake_clean_target.cmake
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/aiclientlib.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/aiclientlib.dir/build: /home/stephan/Documents/Dev/CppSDK/bin/libaiclientlib.a
.PHONY : CMakeFiles/aiclientlib.dir/build

CMakeFiles/aiclientlib.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/aiclientlib.dir/cmake_clean.cmake
.PHONY : CMakeFiles/aiclientlib.dir/clean

CMakeFiles/aiclientlib.dir/depend:
	cd /home/stephan/Documents/Dev/CppSDK/client/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/stephan/Documents/Dev/CppSDK /home/stephan/Documents/Dev/CppSDK /home/stephan/Documents/Dev/CppSDK/client/build /home/stephan/Documents/Dev/CppSDK/client/build /home/stephan/Documents/Dev/CppSDK/client/build/CMakeFiles/aiclientlib.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/aiclientlib.dir/depend

