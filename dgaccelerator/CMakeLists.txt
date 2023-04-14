cmake_minimum_required( VERSION 3.16 )
project(dgaccelerator VERSION 0.0.1)


set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)
# find_package(GStreamer REQUIRED)
SET(NVDS_INSTALL_DIR /opt/nvidia/deepstream/deepstream)

# List all libraries in deepstream.
SET(NVDS_LIBS
  nvdsgst_meta
  nvdsgst_helper
  nvdsgst_smartrecord
  nvds_meta
  nvds_msgbroker
  nvds_utils
  nvbufsurface 
  nvbufsurftransform
)

# Find all libraries in list.
foreach(LIB ${NVDS_LIBS})
  find_library(${LIB}_PATH NAMES ${LIB} PATHS ${NVDS_INSTALL_DIR}/lib)
  if(${LIB}_PATH)
    set(NVDS_LIBRARIES ${NVDS_LIBRARIES} ${${LIB}_PATH})
  else()
    message(FATAL ERROR " Unable to find lib: ${LIB}")
    set(NVDS_LIBRARIES FALSE)
    break()
  endif()
endforeach()

# Find include directories.
find_path(NVDS_INCLUDE_DIRS
  NAMES
    nvds_version.h
  HINTS
    ${NVDS_INSTALL_DIR}/sources/includes
)

# Check libraries and includes.
if (NVDS_LIBRARIES AND NVDS_INCLUDE_DIRS)
  set(NVDS_FOUND TRUE)
else()
  message(FATAL ERROR " Unable to find NVDS")
endif()



# find_package(NVDS REQUIRED)
find_package(CUDA REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-base-1.0 gstreamer-video-1.0)
# pkg_check_modules(DEEPSTREAM REQUIRED deepstream-6.2)


set(SRCs
    dgaccelerator_lib.h
	dgaccelerator_lib.cpp
	gstdgaccelerator.h
	gstdgaccelerator.cpp
    ../CppSDK/inc/Utilities/dg_file_utilities.h
    ../CppSDK/inc/DglibInterface/dg_model_api.h
    ../CppSDK/client/dg_client.h
    ../CppSDK/inc/Utilities/json.hpp
    )
    
add_library(nvdsgst_dgaccelerator SHARED ${SRCs})

# Make sure you have correctly defined the target before adding any compile options to it.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(nvdsgst_dgaccelerator PUBLIC -Wclass-memaccess -fPIC)
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID}: compiler not supported")
endif()
    
target_include_directories(nvdsgst_dgaccelerator PUBLIC
    ${OpenCV_INCLUDE_DIRS}
    ${GSTREAMER_INCLUDE_DIRS}
    ${GSTREAMER_VIDEO_INCLUDE_DIRS}
    ${GSTREAMER_RTSPSERVER_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${NVDS_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    /opt/nvidia/deepstream/deepstream/sources/includes
)

target_link_libraries(nvdsgst_dgaccelerator aiclientlib pthread
${OpenCV_LIBS}
${GSTREAMER_LIBRARIES}
${GSTREAMER_VIDEO_LIBRARIES}
${GLIB_LIBRARIES}
${NVDS_LIBRARIES}
${CUDA_LIBRARIES}
${CMAKE_DL_LIBS}

)
# Set the installation paths
set(GST_INSTALL_DIR "/opt/nvidia/deepstream/deepstream-6.2/lib/gst-plugins/")
set(LIB_INSTALL_DIR "/opt/nvidia/deepstream/deepstream-6.2/lib/")


install(TARGETS nvdsgst_dgaccelerator 
LIBRARY DESTINATION ${GST_INSTALL_DIR} 
ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)