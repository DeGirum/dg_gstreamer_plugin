//////////////////////////////////////////////////////////////////////
///  \file  gstdgaccelerator.cpp
///  \brief DgAccelerator wrapper element header file
///
///  Copyright 2023 DeGirum Corporation
///
///  Permission is hereby granted, free of charge, to any person obtaining a
///  copy of this software and associated documentation files (the "Software"),
///  to deal in the Software without restriction, including without limitation
///  the rights to use, copy, modify, merge, publish, distribute, sublicense,
///  and/or sell copies of the Software, and to permit persons to whom the
///  Software is furnished to do so, subject to the following conditions:
///
///  The above copyright notice and this permission notice shall be included in
///  all copies or substantial portions of the Software.
///
///  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
///  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
///  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
///  DEALINGS IN THE SOFTWARE.
///

#ifndef __GST_DGACCELERATOR_H__
#define __GST_DGACCELERATOR_H__

#define MAX_LABEL_SIZE 128

#include <memory>

// GStreamer
#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

// NVIDIA
#include <cuda_runtime.h>
#include "gst-nvquery.h"
#include "gstnvdsinfer.h"
#include "gstnvdsmeta.h"
#include "nvbufsurface.h"
#include "nvbufsurftransform.h"
#include "nvdsinfer_context.h"
// OpenCV
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

// Degirum
#include "dgaccelerator_lib.h"

// Package and library details boilerplate for plugin_init
#define PACKAGE        "dgaccelerator"
#define VERSION        "1.0"
#define LICENSE        "LGPL"
#define DESCRIPTION    "This plugin permits DeepStream pipelines to connect to DeGirum's AI inference models."
#define BINARY_PACKAGE "NVIDIA DeepStream 3rdparty IP integration"
#define URL            "http://degirum.ai/"

G_BEGIN_DECLS
typedef struct _GstDgAccelerator GstDgAccelerator;
typedef struct _GstDgAcceleratorClass GstDgAcceleratorClass;

#define GST_TYPE_DGACCELERATOR              ( gst_dgaccelerator_get_type() )
#define GST_DGACCELERATOR( obj )            ( G_TYPE_CHECK_INSTANCE_CAST( ( obj ), GST_TYPE_DGACCELERATOR, GstDgAccelerator ) )
#define GST_DGACCELERATOR_CLASS( klass )    ( G_TYPE_CHECK_CLASS_CAST( ( klass ), GST_TYPE_DGACCELERATOR, GstDgAcceleratorClass ) )
#define GST_DGACCELERATOR_GET_CLASS( obj )  ( G_TYPE_INSTANCE_GET_CLASS( ( obj ), GST_TYPE_DGACCELERATOR, GstDgAcceleratorClass ) )
#define GST_IS_DGACCELERATOR( obj )         ( G_TYPE_CHECK_INSTANCE_TYPE( ( obj ), GST_TYPE_DGACCELERATOR ) )
#define GST_IS_DGACCELERATOR_CLASS( klass ) ( G_TYPE_CHECK_CLASS_TYPE( ( klass ), GST_TYPE_DGACCELERATOR ) )
#define GST_DGACCELERATOR_CAST( obj )       ( (GstDgAccelerator *)( obj ) )

// Possible colors for box-color property.
typedef enum
{
	DGACCELERATOR_BOX_COLOR_RED,
	DGACCELERATOR_BOX_COLOR_GREEN,
	DGACCELERATOR_BOX_COLOR_BLUE,
	DGACCELERATOR_BOX_COLOR_YELLOW,
	DGACCELERATOR_BOX_COLOR_CYAN,
	DGACCELERATOR_BOX_COLOR_PINK,
	DGACCELERATOR_BOX_COLOR_BLACK
} GstDgAcceleratorBoxColor;

/// \brief Structure for the dgaccelerator element
struct _GstDgAccelerator
{
	GstBaseTransform base_trans;                                    //!< Gstreamer BaseTransform boilerplate
	DgAcceleratorCtx *dgacceleratorlib_ctx;                         //!< Context of the DG model library
	guint unique_id;                                                //!< Unique ID of the element
	guint64 frame_num;                                              //!< Frame number of the current input buffer
	cudaStream_t cuda_stream;                                       //!< CUDA Stream used for allocating the CUDA task
	void *host_rgb_buf;                                             //!< RBG data in a buffer
	NvBufSurface *inter_buf;                                        //!< The intermediate buffer surface for converting RGBA->BGR
	cv::Mat *cvmat;                                                 //!< OpenCV mat containing RGB data
	GstVideoInfo video_info;                                        //!< Input video info (resolution, color format, framerate, etc)
	gint processing_width;                                          //!< Resolution at which frames/objects should be processed
	gint processing_height;                                         //!< Resolution at which frames/objects should be processed
	guint is_integrated;                                            //!< Flag for igpu/dgpu
	guint batch_size;                                               //!< Amount of objects processed in single call to algorithm
	guint gpu_id;                                                   //!< GPU ID on which we expect to execute the task
	char *model_name;                                               //!< The full name of the model to be used for inference
	char *server_ip;                                                //!< The server ip address to connect to for running inference
	char *cloud_token;                                              //!< The token needed to allow connection to cloud models
	bool drop_frames;                                               //!< Skip frames toggle
	GstDgAcceleratorBoxColor box_color;                             //!< Box Color for visualization
	NvOSD_ColorParams color = ( NvOSD_ColorParams ){ 1, 0, 0, 1 };  //!< Box Color converted into a NvOSD_ColorParams (default red)
};

/// \brief GStreamer boilerplate structure
struct _GstDgAcceleratorClass
{
	GstBaseTransformClass parent_class;  //!< gstreamer boilerplate
};

GType gst_dgaccelerator_get_type( void );

G_END_DECLS
#endif /* __GST_DGACCELERATOR_H__ */
