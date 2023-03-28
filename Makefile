################################################################################
# Copyright (c) 2017-2022, NVIDIA CORPORATION.  All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#################################################################################
#enable this flag to use optimized dgfilternv plugin
#it can also be exported from command line

WITH_OPENCV?=1
USE_OPTIMIZED_DGFILTERNV?=0
CUDA_VER?=11.4
ifeq ($(CUDA_VER),)
  $(error "CUDA_VER is not set")
endif
TARGET_DEVICE = $(shell gcc -dumpmachine | cut -f1 -d -)
CXX:= g++

ifeq ($(USE_OPTIMIZED_DGFILTERNV),1)
  SRCS:= gstdgfilternv_optimized.cpp include/client/dg_client.cpp include/client/dg_model_api.cpp
else
  SRCS:= gstdgfilternv.cpp include/client/dg_client.cpp include/client/dg_model_api.cpp 
endif
# headers go here?
INCS:= $(wildcard *.h) 
include_HEADERS:= $(wildcard include/Utilities/*.h)
include_HEADERS+= $(wildcard include/DglibInterface/*.h)
include_HEADERS+= $(wildcard include/client/*.h)
LIB:=libnvdsgst_dgfilternv.so
INCS+=include_HEADERS
NVDS_VERSION:=6.2

DEP:=dgfilternv_lib/libdgfilternv.a

DEP_FILES-=$(DEP)

CXXFLAGS+= -fPIC -DDS_VERSION=\"6.2.0\" \
	 -I /usr/local/cuda-$(CUDA_VER)/include \
	 -I /opt/nvidia/deepstream/deepstream-6.2/sources/includes -Iinclude -std=c++17 \
	 -Iinclude/Utilities -Iinclude/DglibInterface -Iinclude/client -Iinclude/json

GST_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/gst-plugins/
LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/

LIBS := -shared -Wl,-no-undefined \
	-L dgfilternv_lib -ldgfilternv  -L/include/Utilities -L/include/DglibInterface/ -L/include -L/include/client \
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

all: $(LIB)

%.o: %.cpp $(INCS) Makefile
	@echo $(CXXFLAGS)
	$(CXX) -c -o $@ $(CXXFLAGS) $<

$(LIB): $(OBJS) $(DEP) Makefile
	@echo $(CXXFLAGS)
	$(CXX) -o $@ $(OBJS) $(LIBS)

$(DEP): $(DEP_FILES) 
	$(MAKE) -C  dgfilternv_lib/

install: $(LIB)
	cp -rv $(LIB) $(GST_INSTALL_DIR)

clean:
	rm -rf $(OBJS) $(LIB)
