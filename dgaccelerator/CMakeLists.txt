cmake_minimum_required( VERSION 3.16 )

project(dgaccelerator VERSION 0.0.1)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	target_compile_options(dgaccelerator PRIVATE -Wclass-memaccess -fPIC)
else()
	message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID}: compiler not supported")
endif()


find_package(OpenCV REQUIRED)
find_package(GStreamer REQUIRED)
find_package(DeepstreamAppCommon REQUIRED)
find_package(NVDS REQUIRED)
find_package(CUDA REQUIRED)
find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)


set(SRCs
	dgaccelerator_lib.h
	dgaccelerator_lib.cpp
	gstdgaccelerator.h
	gstdgaccelerator.cpp
)

add_library(dgaccelerator SHARED ${SRCs})

target_include_directories(dgaccelerator 
	${OpenCV_INCLUDE_DIRS})
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${GSTREAMER_RTSPSERVER_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${DEEPSTREAM_APP_COMMON_INCLUDE_DIRS}
    ${NVDS_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${X11_INCLUDE_DIR}
)

target_link_libraries(dgaccelerator aiclientlib pthread m
	${OpenCV_LIBS}
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER_VIDEO_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${NVDS_LIBRARIES}
    ${CUDA_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${X11_LIBRARIES}
)
