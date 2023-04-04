WITH_OPENCV?=1
USE_OPTIMIZED_DGACCELERATOR?=0
# Set the CUDA Version of your system here: 
# To find your CUDA version, run in a terminal window: nvcc --version
CUDA_VER?=11.4
ifeq ($(CUDA_VER),)
  $(error "CUDA_VER is not set")
endif
TARGET_DEVICE = $(shell gcc -dumpmachine | cut -f1 -d -)
CXX:= g++

ifeq ($(USE_OPTIMIZED_DGACCELERATOR),1)
  SRCS:= gstdgaccelerator_optimized.cpp include/client/dg_client.cpp include/client/dg_model_api.cpp
else
  SRCS:= gstdgaccelerator.cpp include/client/dg_client.cpp include/client/dg_model_api.cpp 
endif
# headers go here?
INCS:= $(wildcard *.h) 
include_HEADERS:= $(wildcard include/Utilities/*.h)
include_HEADERS+= $(wildcard include/DglibInterface/*.h)
include_HEADERS+= $(wildcard include/client/*.h)
LIB:=libnvdsgst_dgaccelerator.so
INCS+=include_HEADERS
NVDS_VERSION:=6.2
# defines the DEP_FILES variable to hold the path to the dependency files for
# the dgaccelerator_lib library.
DEP:=dgaccelerator_lib/libdgaccelerator.a

DEP_FILES-=$(DEP)

CXXFLAGS+= -fPIC -DDS_VERSION=\"6.2.0\" \
	 -I /usr/local/cuda-$(CUDA_VER)/include \
	 -I /opt/nvidia/deepstream/deepstream-6.2/sources/includes -Iinclude -std=c++17 \
	 -Iinclude/Utilities -Iinclude/DglibInterface -Iinclude/client -Iinclude/json

GST_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/gst-plugins/
LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/

LIBS := -shared -Wl,-no-undefined \
	-L dgaccelerator_lib -ldgaccelerator  -L/include/Utilities -L/include/DglibInterface/ -L/include -L/include/client \
	-L/usr/local/cuda-$(CUDA_VER)/lib64/ -lcudart -ldl \
	-lnppc -lnppig -lnpps -lnppicc -lnppidei -lpthread \
	-L$(LIB_INSTALL_DIR) -lnvdsgst_helper -lnvdsgst_meta -lnvds_meta -lnvbufsurface -lnvbufsurftransform\
	-Wl,-rpath,$(LIB_INSTALL_DIR)

OBJS:= $(SRCS:.cpp=.o)

PKGS:= gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0


CXXFLAGS+= -DWITH_OPENCV
PKGS+= opencv4


CXXFLAGS+=$(shell pkg-config --cflags $(PKGS))
LIBS+=$(shell pkg-config --libs $(PKGS))

# This line declares a target all that depends on $(LIB). 
# When make is invoked with the all target, it will build the library.
all: $(LIB)

# This is a pattern rule that describes how to build an object file (%.o) 
# from a C++ source file (%.cpp). The rule depends on the source file, any 
# header files ($(INCS)), and the Makefile itself. The rule first echoes the
# value of $(CXXFLAGS), which should be a variable containing compiler flags 
# like -g or -O2. Then, it invokes the C++ compiler ($(CXX)) to compile the 
# source file into an object file (-c -o $@), using the compiler flags ($(CXXFLAGS)) 
# and the first prerequisite ($<, which is the source file itself).
%.o: %.cpp $(INCS) Makefile
	@echo $(CXXFLAGS)
	$(CXX) -c -o $@ $(CXXFLAGS) $<

# This rule describes how to build the library ($(LIB)) from the object 
# files ($(OBJS)) and any dependencies ($(DEP)) specified in the Makefile. 
# The rule first echoes the value of $(CXXFLAGS). Then, it invokes the C++ 
# compiler to link the object files into the library (-o $@), using any library flags ($(LIBS)).
$(LIB): $(OBJS) $(DEP) Makefile
	@echo $(CXXFLAGS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

# This rule specifies a dependency file ($(DEP)) that depends on some other 
# files ($(DEP_FILES)). When this dependency is needed, make will run the make
# command in the dgaccelerator_lib directory to generate the dependency file.
$(DEP): $(DEP_FILES) 
	$(MAKE) -C  dgaccelerator_lib/

# This rule describes how to install the library by copying 
# it to the $(GST_INSTALL_DIR) directory.
install: $(LIB)
	cp -rv $(LIB) $(GST_INSTALL_DIR)

# This rule describes how to clean up the object files and 
# library. When this target is invoked, it removes the object 
# files ($(OBJS)) and the library ($(LIB)) using the rm command.
# The -rf flag ensures that any subdirectories created by 
# the build process are also removed.
clean:
	rm -rf $(OBJS) $(LIB)
