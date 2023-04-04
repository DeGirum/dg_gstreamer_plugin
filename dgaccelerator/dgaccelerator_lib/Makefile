PKGS:= opencv4
CXXFLAGS+= -fPIC -DDS_VERSION=\"6.2.0\" \
	 -I /usr/local/cuda-$(CUDA_VER)/include \
	 -I /opt/nvidia/deepstream/deepstream-6.2/sources/includes -I../include -std=c++17
CXXFLAGS+=$(shell pkg-config --cflags $(PKGS))

# Define source files and library file
SRCS := dgaccelerator_lib.cpp
LIB := libdgaccelerator.a

# Define objects files based on source files
OBJS := $(SRCS:.cpp=.o)

# This makefile code defines a single target named all 
# that generates a static library named libdgaccelerator.a.
all: $(LIB)

# Define a rule to build the library file
$(LIB): $(OBJS)
	ar rcs $@ $^

# Define a rule to build object files from source files
%.o: %.cpp
	g++ -ggdb $(CXXFLAGS) -c -o $@ $< -std=c++17 -DWITH_OPENCV

# Define a rule to clean up object and library files
clean:
	rm -f $(OBJS) $(LIB)
