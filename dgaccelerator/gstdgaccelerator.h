/**
 * Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
 * Copyright (c) 2023 Stephan Sokolov < stephan@degirum.com >
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This software contains source code provided by NVIDIA Corporation.
 *
 */

#ifndef __GST_DGACCELERATOR_H__
#define __GST_DGACCELERATOR_H__

#define MAX_LABEL_SIZE 128

#include <gst/base/gstbasetransform.h>
#include <gst/video/video.h>

#include <cuda.h>
#include <cuda_runtime.h>
#include <memory>
#include "dgaccelerator_lib.h"
#include "gst-nvquery.h"
#include "gstnvdsmeta.h"
#include "nvbufsurface.h"
#include "nvbufsurftransform.h"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

/* Package and library details required for plugin_init */
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

/*
 * Possible colors for box-color property.
 */
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

struct _GstDgAccelerator
{
	GstBaseTransform base_trans;

	// Context of the DG model library
	DgAcceleratorCtx *dgacceleratorlib_ctx;

	// Unique ID of the element. The labels generated by the element will be
	// updated at index `unique_id` of attr_info array in NvDsObjectParams.
	guint unique_id;

	// Frame number of the current input buffer
	guint64 frame_num;

	// CUDA Stream used for allocating the CUDA task
	cudaStream_t cuda_stream;

	// RBG data in a buffer
	void *host_rgb_buf;

	// the intermediate scratch buffer for conversions RGBA
	NvBufSurface *inter_buf;

	// OpenCV mat containing RGB data
	cv::Mat *cvmat;

	// Input video info (resolution, color format, framerate, etc)
	GstVideoInfo video_info;

	// Resolution at which frames/objects should be processed
	gint processing_width;
	gint processing_height;

	// Flag for igpu/dgpu
	guint is_integrated;

	// Amount of objects processed in single call to algorithm
	guint batch_size;

	// GPU ID on which we expect to execute the task
	guint gpu_id;

	// Model name
	char *model_name;

	// Server IP
	char *server_ip;

	// Cloud Token
	char *cloud_token;

	// Skip frames toggle
	bool drop_frames;

	// Box Color
	GstDgAcceleratorBoxColor box_color;
	// box color converted
	NvOSD_ColorParams color = ( NvOSD_ColorParams ){ 1, 0, 0, 1 };
};

// Boiler plate stuff
struct _GstDgAcceleratorClass
{
	GstBaseTransformClass parent_class;
};

GType gst_dgaccelerator_get_type( void );

G_END_DECLS
#endif /* __GST_DGACCELERATOR_H__ */
