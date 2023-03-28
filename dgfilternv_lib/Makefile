PKGS:= opencv4
CXXFLAGS+= -fPIC -DDS_VERSION=\"6.2.0\" \
	 -I /usr/local/cuda-$(CUDA_VER)/include \
	 -I /opt/nvidia/deepstream/deepstream-6.2/sources/includes -I../include -std=c++17
CXXFLAGS+=$(shell pkg-config --cflags $(PKGS))

# This makefile code defines a single target named all 
# that generates a static library named libdgfilternv.a.
all:
	g++ -ggdb $(CXXFLAGS) -c -o dgfilternv_lib.o -fPIC dgfilternv_lib.cpp -std=c++17 -DWITH_OPENCV
	ar rcs libdgfilternv.a dgfilternv_lib.o