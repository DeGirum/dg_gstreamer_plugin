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

#include <string.h>
#include <string>
#include <sstream>
#include <iostream>
#include <ostream>
#include <fstream>
#include "gstdgaccelerator.h"
#include <sys/time.h>
#include "include/Utilities/dg_tracing_facility.h"

GST_DEBUG_CATEGORY_STATIC (gst_dgaccelerator_debug);
#define GST_CAT_DEFAULT gst_dgaccelerator_debug
#define USE_EGLIMAGE 1
static GQuark _dsmeta_quark = 0;

DG_TRC_GROUP_DEF(DgAccelerator)


/* Enum to identify properties */
enum
{
  PROP_0,
  PROP_UNIQUE_ID,
  PROP_PROCESSING_WIDTH,
  PROP_PROCESSING_HEIGHT,
  PROP_MODEL_NAME,
  PROP_SERVER_IP,
  PROP_CLOUD_TOKEN,
  PROP_DROP_FRAMES,
  PROP_GPU_DEVICE_ID
};

#define CHECK_NVDS_MEMORY_AND_GPUID(object, surface)  \
  ({ int _errtype=0;\
   do {  \
    if ((surface->memType == NVBUF_MEM_DEFAULT || surface->memType == NVBUF_MEM_CUDA_DEVICE) && \
        (surface->gpuId != object->gpu_id))  { \
    GST_ELEMENT_ERROR (object, RESOURCE, FAILED, \
        ("Input surface gpu-id doesnt match with configured gpu-id for element," \
         " please allocate input using unified memory, or use same gpu-ids"),\
        ("surface-gpu-id=%d,%s-gpu-id=%d",surface->gpuId,GST_ELEMENT_NAME(object),\
         object->gpu_id)); \
    _errtype = 1;\
    } \
    } while(0); \
    _errtype; \
  })

/* DEFAULT PROPERTY VALUES */
#define DEFAULT_UNIQUE_ID 15
#define DEFAULT_PROCESSING_WIDTH 512
#define DEFAULT_PROCESSING_HEIGHT 512
#define DEFAULT_GPU_ID 0
#define DEFAULT_MODEL_NAME "yolo_v5s_coco--512x512_quant_n2x_orca_1"
#define DEFAULT_SERVER_IP "100.122.112.76"
#define DEFAULT_CLOUD_TOKEN ""
#define DEFAULT_DROP_FRAMES true

#define RGB_BYTES_PER_PIXEL 3
#define RGBA_BYTES_PER_PIXEL 4
#define Y_BYTES_PER_PIXEL 1
#define UV_BYTES_PER_PIXEL 2

#define MIN_INPUT_OBJECT_WIDTH 16
#define MIN_INPUT_OBJECT_HEIGHT 16

#define CHECK_NPP_STATUS(npp_status,error_str) do { \
  if ((npp_status) != NPP_SUCCESS) { \
    g_print ("Error: %s in %s at line %d: NPP Error %d\n", \
        error_str, __FILE__, __LINE__, npp_status); \
    goto error; \
  } \
} while (0)

#define CHECK_CUDA_STATUS(cuda_status,error_str) do { \
  if ((cuda_status) != cudaSuccess) { \
    g_print ("Error: %s in %s at line %d (%s)\n", \
        error_str, __FILE__, __LINE__, cudaGetErrorName(cuda_status)); \
    goto error; \
  } \
} while (0)

/* By default NVIDIA Hardware allocated memory flows through the pipeline. We
 * will be processing on this type of memory only. */
#define GST_CAPS_FEATURE_MEMORY_NVMM "memory:NVMM"

// Templates for sink and source pad
static GstStaticPadTemplate gst_dgaccelerator_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_NVMM,
            "{ NV12, RGBA, I420 }")));

static GstStaticPadTemplate gst_dgaccelerator_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE_WITH_FEATURES
        (GST_CAPS_FEATURE_MEMORY_NVMM,
            "{ NV12, RGBA, I420 }")));

#define gst_dgaccelerator_parent_class parent_class
G_DEFINE_TYPE (GstDgAccelerator, gst_dgaccelerator, GST_TYPE_BASE_TRANSFORM);

static void gst_dgaccelerator_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_dgaccelerator_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_dgaccelerator_set_caps (GstBaseTransform * btrans,
    GstCaps * incaps, GstCaps * outcaps);
static gboolean gst_dgaccelerator_start (GstBaseTransform * btrans);
static gboolean gst_dgaccelerator_stop (GstBaseTransform * btrans);
static GstFlowReturn gst_dgaccelerator_transform_ip (GstBaseTransform *
    btrans, GstBuffer * inbuf);
static void
attach_metadata_full_frame (GstDgAccelerator * dgaccelerator, NvDsFrameMeta *frame_meta,
    gdouble scale_ratio, DgAcceleratorOutput * output, guint batch_id);
// Installs the object and BaseTransform properties along with pads
static void
gst_dgaccelerator_class_init (GstDgAcceleratorClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseTransformClass *gstbasetransform_class;

  /* Indicates we want to use DS buf api */
  g_setenv ("DS_NEW_BUFAPI", "1", TRUE);

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasetransform_class = (GstBaseTransformClass *) klass;

  /* Overide base class functions */
  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_dgaccelerator_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_dgaccelerator_get_property);

  gstbasetransform_class->set_caps = GST_DEBUG_FUNCPTR (gst_dgaccelerator_set_caps);
  gstbasetransform_class->start = GST_DEBUG_FUNCPTR (gst_dgaccelerator_start);
  gstbasetransform_class->stop = GST_DEBUG_FUNCPTR (gst_dgaccelerator_stop);

  gstbasetransform_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_dgaccelerator_transform_ip);

  /* Install properties */
  g_object_class_install_property (gobject_class, PROP_UNIQUE_ID,
      g_param_spec_uint ("unique-id",
          "Unique ID",
          "Unique ID for the element. Can be used to identify output of the"
          " element", 0, G_MAXUINT, DEFAULT_UNIQUE_ID, (GParamFlags)
          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_PROCESSING_WIDTH,
      g_param_spec_int ("processing-width",
          "Processing Width",
          "Width of the input buffer to algorithm",
          1, G_MAXINT, DEFAULT_PROCESSING_WIDTH, (GParamFlags)
          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_PROCESSING_HEIGHT,
      g_param_spec_int ("processing-height",
          "Processing Height",
          "Height of the input buffer to algorithm",
          1, G_MAXINT, DEFAULT_PROCESSING_HEIGHT, (GParamFlags)
          (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class, PROP_MODEL_NAME,
      g_param_spec_string ("model_name", "model_name", "Full model name",
          DEFAULT_MODEL_NAME, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_SERVER_IP,
      g_param_spec_string ("server_ip", "server_ip", "Full server IP",
          DEFAULT_SERVER_IP, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_CLOUD_TOKEN,
      g_param_spec_string ("cloud_token", "cloud_token", "Cloud token for non-local inference",
          DEFAULT_CLOUD_TOKEN, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_DROP_FRAMES,
      g_param_spec_boolean ("drop_frames", "drop_frames", "Toggle for skipping buffers if required for performance. Keep this on for visualization pipelines.",
          DEFAULT_DROP_FRAMES, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_GPU_DEVICE_ID,
      g_param_spec_uint ("gpu-id",
          "Set GPU Device ID",
          "Set GPU Device ID", 0,
          G_MAXUINT, 0,
          GParamFlags
          (G_PARAM_READWRITE |
              G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY)));
  /* Set sink and src pad capabilities */
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_dgaccelerator_src_template));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&gst_dgaccelerator_sink_template));

  /* Set metadata describing the element */
  gst_element_class_set_details_simple (gstelement_class,
      "DgAccelerator plugin",
      "DgAccelerator Plugin",
      "Uses NVIDIA's 3rdparty algorithm wrapper to process video frames",
      "Stephan Sokolov < stephan@degirum.ai >");
}
// Initialize the element. Set the BaseTransform properties to work in in-place mode
static void
gst_dgaccelerator_init (GstDgAccelerator * dgaccelerator)
{
  GstBaseTransform *btrans = GST_BASE_TRANSFORM (dgaccelerator);
  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (btrans), TRUE);
  gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (btrans), TRUE);

  /* Initialize all property variables to default values */
  dgaccelerator->unique_id = DEFAULT_UNIQUE_ID;
  dgaccelerator->processing_width = DEFAULT_PROCESSING_WIDTH;
  dgaccelerator->processing_height = DEFAULT_PROCESSING_HEIGHT;
  dgaccelerator->gpu_id = DEFAULT_GPU_ID;
  dgaccelerator->model_name = const_cast<char*>(DEFAULT_MODEL_NAME);
  dgaccelerator->server_ip = const_cast<char*>(DEFAULT_SERVER_IP);

  dgaccelerator->drop_frames = DEFAULT_DROP_FRAMES;

  /* This quark is required to identify NvDsMeta when iterating through
   * the buffer metadatas */
  if (!_dsmeta_quark)
    _dsmeta_quark = g_quark_from_static_string (NVDS_META_STRING);
}
// Set property function
static void
gst_dgaccelerator_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR (object);
  switch (prop_id) {
    case PROP_UNIQUE_ID:
      dgaccelerator->unique_id = g_value_get_uint (value);
      break;
    case PROP_PROCESSING_WIDTH:
      dgaccelerator->processing_width = g_value_get_int (value);
      break;
    case PROP_PROCESSING_HEIGHT:
      dgaccelerator->processing_height = g_value_get_int (value);
      break;
    case PROP_GPU_DEVICE_ID:
      dgaccelerator->gpu_id = g_value_get_uint (value);
      break;
    case PROP_MODEL_NAME:
      // Don't allow >128 characters!
      if (strlen(g_value_get_string(value))+1 > 128)
      {
        std::cout << "\n\nModel Name is too long! Setting the model to the default value!\n\n";
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
      }
      dgaccelerator->model_name = new char[strlen(g_value_get_string(value))+1];
      strcpy(dgaccelerator->model_name, g_value_get_string(value));
      break;
    case PROP_SERVER_IP:
      // Don't allow >128 characters!
      if (strlen(g_value_get_string(value))+1 > 128)
      {
        std::cout << "\n\nServer IP is too long! Setting the server ip to the default value!\n\n";
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
      }
      dgaccelerator->server_ip = new char[strlen(g_value_get_string(value))+1];
      strcpy(dgaccelerator->server_ip, g_value_get_string(value));
      break;
    case PROP_CLOUD_TOKEN:
      // Don't allow > 128 characters!
      if (strlen(g_value_get_string(value))+1 > 128)
      {
        std::cout << "\n\nCloud Token is too long! Setting the cloud token to the default value!\n\n";
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
      }
      dgaccelerator->cloud_token = new char[strlen(g_value_get_string(value))+1];
      strcpy(dgaccelerator->cloud_token, g_value_get_string(value));
      break;
    case PROP_DROP_FRAMES:
      dgaccelerator->drop_frames = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

// Get property function
static void
gst_dgaccelerator_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR (object);

  switch (prop_id) {
    case PROP_UNIQUE_ID:
      g_value_set_uint (value, dgaccelerator->unique_id);
      break;
    case PROP_PROCESSING_WIDTH:
      g_value_set_int (value, dgaccelerator->processing_width);
      break;
    case PROP_PROCESSING_HEIGHT:
      g_value_set_int (value, dgaccelerator->processing_height);
      break;
    case PROP_GPU_DEVICE_ID:
      g_value_set_uint (value, dgaccelerator->gpu_id);
      break;
    case PROP_MODEL_NAME:
      g_value_set_string(value, dgaccelerator->model_name);
      break;
    case PROP_SERVER_IP:
      g_value_set_string(value, dgaccelerator->server_ip);
      break;
    case PROP_CLOUD_TOKEN:
      g_value_set_string(value, dgaccelerator->cloud_token);
      break;
    case PROP_DROP_FRAMES:
      g_value_set_boolean(value, dgaccelerator->drop_frames);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/**
 * Initialize all the parameters and CUDA stream.
 * Called as a result of BaseTransform class changing states in pipeline
 */
static gboolean
gst_dgaccelerator_start (GstBaseTransform * btrans)
{
  GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR (btrans);
  NvBufSurfaceCreateParams create_params;

  DgAcceleratorInitParams init_params =
  { dgaccelerator->processing_width, dgaccelerator->processing_height,
    "", "", 0, "", dgaccelerator->drop_frames
  };
  snprintf(init_params.model_name, 128, "%s", dgaccelerator->model_name); // Sets the model name
  snprintf(init_params.server_ip, 128, "%s", dgaccelerator->server_ip); // Sets the server ip
  snprintf(init_params.cloud_token, 128, "%s", dgaccelerator->cloud_token); // Sets the cloud token

  GstQuery *queryparams = NULL;
  guint batch_size = 1;
  int val = -1;


  GST_DEBUG_OBJECT (dgaccelerator, "ctx lib %p \n", dgaccelerator->dgacceleratorlib_ctx);

  CHECK_CUDA_STATUS (cudaSetDevice (dgaccelerator->gpu_id),
      "Unable to set cuda device");

  cudaDeviceGetAttribute (&val, cudaDevAttrIntegrated, dgaccelerator->gpu_id);
  dgaccelerator->is_integrated = val;
  
  // queries the batch size of the input buffers from the upstream element and
  // sets the batch size for the plugin accordingly
  dgaccelerator->batch_size = 1;
  queryparams = gst_nvquery_batch_size_new ();
  if (gst_pad_peer_query (GST_BASE_TRANSFORM_SINK_PAD (btrans), queryparams)
      || gst_pad_peer_query (GST_BASE_TRANSFORM_SRC_PAD (btrans), queryparams)) {
    if (gst_nvquery_batch_size_parse (queryparams, &batch_size)) {
      dgaccelerator->batch_size = batch_size;
    }
  }
  GST_DEBUG_OBJECT (dgaccelerator, "Setting batch-size %d \n",
      dgaccelerator->batch_size);
  gst_query_unref (queryparams);

  init_params.numInputStreams = batch_size; // Sets the number of input streams

  dgaccelerator->dgacceleratorlib_ctx = DgAcceleratorCtxInit (&init_params);


  CHECK_CUDA_STATUS (cudaStreamCreate (&dgaccelerator->cuda_stream),
      "Could not create cuda stream");

  if (dgaccelerator->inter_buf)
    NvBufSurfaceDestroy (dgaccelerator->inter_buf);
  dgaccelerator->inter_buf = NULL;

  /* An intermediate buffer for NV12/RGBA to BGR conversion  will be
   * required. Can be skipped if custom algorithm can work directly on NV12/RGBA. */
  create_params.gpuId  = dgaccelerator->gpu_id;
  create_params.width  = dgaccelerator->processing_width;
  create_params.height = dgaccelerator->processing_height;
  create_params.size = 0;
  create_params.colorFormat = NVBUF_COLOR_FORMAT_RGBA;
  create_params.layout = NVBUF_LAYOUT_PITCH;

  if(dgaccelerator->is_integrated) {
    create_params.memType = NVBUF_MEM_DEFAULT;
  }
  else {
    create_params.memType = NVBUF_MEM_CUDA_PINNED;
  }

  if (NvBufSurfaceCreate (&dgaccelerator->inter_buf, 1,
          &create_params) != 0) {
    GST_ERROR ("Error: Could not allocate internal buffer for dgaccelerator");
    goto error;
  }

  /* Create host memory for storing converted/scaled interleaved RGB data */
  CHECK_CUDA_STATUS (cudaMallocHost (&dgaccelerator->host_rgb_buf,
          dgaccelerator->processing_width * dgaccelerator->processing_height *
          RGB_BYTES_PER_PIXEL), "Could not allocate cuda host buffer");

  GST_DEBUG_OBJECT (dgaccelerator, "allocated cuda buffer %p \n",
      dgaccelerator->host_rgb_buf);

  /* CV Mat containing interleaved RGB data. This call does not allocate memory.
   * It uses host_rgb_buf as data. */
  dgaccelerator->cvmat =
      new cv::Mat (dgaccelerator->processing_height, dgaccelerator->processing_width,
      CV_8UC3, dgaccelerator->host_rgb_buf,
      dgaccelerator->processing_width * RGB_BYTES_PER_PIXEL);

  if (!dgaccelerator->cvmat)
    goto error;

  GST_DEBUG_OBJECT (dgaccelerator, "created CV Mat\n");


  return TRUE;
error:
  if (dgaccelerator->host_rgb_buf) {
    cudaFreeHost (dgaccelerator->host_rgb_buf);
    dgaccelerator->host_rgb_buf = NULL;
  }

  if (dgaccelerator->cuda_stream) {
    cudaStreamDestroy (dgaccelerator->cuda_stream);
    dgaccelerator->cuda_stream = NULL;
  }
  if (dgaccelerator->dgacceleratorlib_ctx)
    DgAcceleratorCtxDeinit (dgaccelerator->dgacceleratorlib_ctx);
  return FALSE;
}

// Stop the element, free memory, deinitialize our library
static gboolean
gst_dgaccelerator_stop (GstBaseTransform * btrans)
{
  GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR (btrans);

  if (dgaccelerator->inter_buf)
    NvBufSurfaceDestroy(dgaccelerator->inter_buf);
  dgaccelerator->inter_buf = NULL;

  if (dgaccelerator->cuda_stream)
    cudaStreamDestroy (dgaccelerator->cuda_stream);
  dgaccelerator->cuda_stream = NULL;

  delete dgaccelerator->cvmat;
  dgaccelerator->cvmat = NULL;

  if (dgaccelerator->host_rgb_buf) {
    cudaFreeHost (dgaccelerator->host_rgb_buf);
    dgaccelerator->host_rgb_buf = NULL;
  }

  GST_DEBUG_OBJECT (dgaccelerator, "deleted CV Mat \n");

  /* Deinit the algorithm library */
  DgAcceleratorCtxDeinit (dgaccelerator->dgacceleratorlib_ctx);
  dgaccelerator->dgacceleratorlib_ctx = NULL;

  GST_DEBUG_OBJECT (dgaccelerator, "ctx lib released \n");

  return TRUE;
}

/**
 * Called when source / sink pad capabilities have been negotiated.
 */
static gboolean
gst_dgaccelerator_set_caps (GstBaseTransform * btrans, GstCaps * incaps,
    GstCaps * outcaps)
{
  GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR (btrans);
  /* Save the input video information, since this will be required later. */
  gst_video_info_from_caps (&dgaccelerator->video_info, incaps);

  return TRUE;

error:
  return FALSE;
}

/**
 * Scale the entire frame to the processing resolution maintaining aspect ratio.
 * Or crop and scale objects to the processing resolution maintaining the aspect
 * ratio. Remove the padding required by hardware and convert from RGBA to RGB
 * using openCV. These steps can be skipped if the algorithm can work with
 * padded data and/or can work with RGBA.
 *
 * The input NvBufSurface object is modified in-place to contain the output
 * buffer. The function uses NVIDIA's NvBufSurfTransform API to perform
 * scaling, format conversion, and cropping, as well as OpenCV's cvtColor
 * function to convert the RGBA format to BGR format. The input buffer is
 * cropped according to the provided crop_rect_params rectangle, and the aspect
 * ratio is maintained while scaling to a destination resolution specified by
 * processing_width and processing_height parameters of dgaccelerator structure.
 */
static GstFlowReturn
get_converted_mat (GstDgAccelerator * dgaccelerator, NvBufSurface *input_buf, gint idx,
    NvOSD_RectParams * crop_rect_params, gdouble & ratio, gint input_width,
    gint input_height)
{
  DG_TRC_BLOCK(DgAccelerator, get_converted_mat, DGTrace::lvlBasic);

  NvBufSurfTransform_Error err;
  NvBufSurfTransformConfigParams transform_config_params;
  NvBufSurfTransformParams transform_params;
  NvBufSurfTransformRect src_rect;
  NvBufSurfTransformRect dst_rect;
  NvBufSurface ip_surf;
  cv::Mat in_mat;
  ip_surf = *input_buf;

  ip_surf.numFilled = ip_surf.batchSize = 1;
  ip_surf.surfaceList = &(input_buf->surfaceList[idx]);

  gint src_left = GST_ROUND_UP_2((unsigned int)crop_rect_params->left);
  gint src_top = GST_ROUND_UP_2((unsigned int)crop_rect_params->top);
  gint src_width = GST_ROUND_DOWN_2((unsigned int)crop_rect_params->width);
  gint src_height = GST_ROUND_DOWN_2((unsigned int)crop_rect_params->height);

  /* Maintain aspect ratio */
  double hdest = dgaccelerator->processing_width * src_height / (double) src_width;
  double wdest = dgaccelerator->processing_height * src_width / (double) src_height;
  guint dest_width, dest_height;

  if (hdest <= dgaccelerator->processing_height) {
    dest_width = dgaccelerator->processing_width;
    dest_height = hdest;
  } else {
    dest_width = wdest;
    dest_height = dgaccelerator->processing_height;
  }

  /* Configure transform session parameters for the transformation */
  transform_config_params.compute_mode = NvBufSurfTransformCompute_Default;
  transform_config_params.gpu_id = dgaccelerator->gpu_id;
  transform_config_params.cuda_stream = dgaccelerator->cuda_stream;

  /* Set the transform session parameters for the conversions executed in this
   * thread. */
  err = NvBufSurfTransformSetSessionParams (&transform_config_params);
  if (err != NvBufSurfTransformError_Success) {
    GST_ELEMENT_ERROR (dgaccelerator, STREAM, FAILED,
        ("NvBufSurfTransformSetSessionParams failed with error %d", err), (NULL));
    goto error;
  }

  /* Calculate scaling ratio while maintaining aspect ratio */
  ratio = MIN (1.0 * dest_width/ src_width, 1.0 * dest_height / src_height);

  if ((crop_rect_params->width == 0) || (crop_rect_params->height == 0)) {
    GST_ELEMENT_ERROR (dgaccelerator, STREAM, FAILED,
        ("%s:crop_rect_params dimensions are zero",__func__), (NULL));
    goto error;
  }

#ifdef __aarch64__
  if (ratio <= 1.0 / 16 || ratio >= 16.0) {
    /* Currently cannot scale by ratio > 16 or < 1/16 for Jetson */
    goto error;
  }
#endif
  /* Set the transform ROIs for source and destination */
  src_rect = {(guint)src_top, (guint)src_left, (guint)src_width, (guint)src_height};
  dst_rect = {0, 0, (guint)dest_width, (guint)dest_height};

  /* Set the transform parameters */
  transform_params.src_rect = &src_rect;
  transform_params.dst_rect = &dst_rect;
  transform_params.transform_flag =
    NVBUFSURF_TRANSFORM_FILTER | NVBUFSURF_TRANSFORM_CROP_SRC |
      NVBUFSURF_TRANSFORM_CROP_DST;
  transform_params.transform_filter = NvBufSurfTransformInter_Default;

  /* Memset the memory */
  NvBufSurfaceMemSet (dgaccelerator->inter_buf, 0, 0, 0);

  GST_DEBUG_OBJECT (dgaccelerator, "Scaling and converting input buffer\n");

  /* Transformation scaling+format conversion if any. */
  err = NvBufSurfTransform (&ip_surf, dgaccelerator->inter_buf, &transform_params);
  if (err != NvBufSurfTransformError_Success) {
    GST_ELEMENT_ERROR (dgaccelerator, STREAM, FAILED,
        ("NvBufSurfTransform failed with error %d while converting buffer", err),
        (NULL));
    goto error;
  }
  /* Map the buffer so that it can be accessed by CPU */
  if (NvBufSurfaceMap (dgaccelerator->inter_buf, 0, 0, NVBUF_MAP_READ) != 0){
    goto error;
  }
  if(dgaccelerator->inter_buf->memType == NVBUF_MEM_SURFACE_ARRAY) {
    /* Cache the mapped data for CPU access */
    NvBufSurfaceSyncForCpu (dgaccelerator->inter_buf, 0, 0);
  }

  /* Use openCV to remove padding and convert RGBA to BGR. Can be skipped if
   * algorithm can handle padded RGBA data. */
  in_mat =
      cv::Mat (dgaccelerator->processing_height, dgaccelerator->processing_width,
      CV_8UC4, dgaccelerator->inter_buf->surfaceList[0].mappedAddr.addr[0],
      dgaccelerator->inter_buf->surfaceList[0].pitch);

#if (CV_MAJOR_VERSION >= 4)
  cv::cvtColor (in_mat, *dgaccelerator->cvmat, cv::COLOR_RGBA2BGR);
#else
  cv::cvtColor (in_mat, *dgaccelerator->cvmat, CV_RGBA2BGR);
#endif

    if (NvBufSurfaceUnMap (dgaccelerator->inter_buf, 0, 0)){
      goto error;
    }

  if(dgaccelerator->is_integrated) {
#ifdef __aarch64__
    /* To use the converted buffer in CUDA, create an EGLImage and then use
    * CUDA-EGL interop APIs */
    if (USE_EGLIMAGE) {
      if (NvBufSurfaceMapEglImage (dgaccelerator->inter_buf, 0) !=0 ) {
        goto error;
      }

      /* dgaccelerator->inter_buf->surfaceList[0].mappedAddr.eglImage
      * Use interop APIs cuGraphicsEGLRegisterImage and
      * cuGraphicsResourceGetMappedEglFrame to access the buffer in CUDA */

      /* Destroy the EGLImage */
      NvBufSurfaceUnMapEglImage (dgaccelerator->inter_buf, 0);
    }
#endif
  }
  return GST_FLOW_OK;

error:
  return GST_FLOW_ERROR;
}

/**
 * BaseTransform main processing function, called when input buffer received.
 * Transform the buffer in-place, using our static library
 */
static GstFlowReturn
gst_dgaccelerator_transform_ip (GstBaseTransform * btrans, GstBuffer * inbuf)
{
  DG_TRC_BLOCK(DgAccelerator, gst_dgaccelerator_transform_ip, DGTrace::lvlBasic);

  // initializes NVIDIA buffer and metadata variables.
  GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR (btrans);
  GstMapInfo in_map_info;
  GstFlowReturn flow_ret = GST_FLOW_ERROR;
  gdouble scale_ratio = 1.0;

  // Initialize an empty frame of objects
  DgAcceleratorOutput *output;

  NvBufSurface *surface = NULL;
  NvDsBatchMeta *batch_meta = NULL;
  NvDsFrameMeta *frame_meta = NULL;
  NvDsMetaList * l_frame = NULL;
  guint i = 0;

  dgaccelerator->frame_num++;
  CHECK_CUDA_STATUS (cudaSetDevice (dgaccelerator->gpu_id),
      "Unable to set cuda device");

  // maps the input buffer to get the input NvBufSurface.
  memset (&in_map_info, 0, sizeof (in_map_info));
  if (!gst_buffer_map (inbuf, &in_map_info, GST_MAP_READ)) {
    g_print ("Error: Failed to map gst buffer\n");
    goto error;
  }
  nvds_set_input_system_timestamp (inbuf, GST_ELEMENT_NAME (dgaccelerator));
  surface = (NvBufSurface *) in_map_info.data;
  GST_DEBUG_OBJECT (dgaccelerator,
      "Processing Frame %" G_GUINT64_FORMAT " Surface %p\n",
      dgaccelerator->frame_num, surface);

  // checks the NvBufSurface to ensure it is valid and that the memory is
  // allocated on the correct GPU
  if (CHECK_NVDS_MEMORY_AND_GPUID (dgaccelerator, surface))
    goto error;

  // retrieves the NvDsBatchMeta from the input buffer and processes each
  // NvDsFrameMeta in the batch
  batch_meta = gst_buffer_get_nvds_batch_meta (inbuf);
  if (batch_meta == nullptr) {
    GST_ELEMENT_ERROR (dgaccelerator, STREAM, FAILED,
        ("NvDsBatchMeta not found for input buffer."), (NULL));
    return GST_FLOW_ERROR;
  }

  // sets the scaling parameters for the frame and scales and converts the
  // frame using the get_converted_mat function
  for (l_frame = batch_meta->frame_meta_list; l_frame != NULL;
    l_frame = l_frame->next)
  {
    frame_meta = (NvDsFrameMeta *) (l_frame->data);
    NvOSD_RectParams rect_params;

    /* Scale the entire frame to processing resolution */
    rect_params.left = 0;
    rect_params.top = 0;
    rect_params.width = dgaccelerator->video_info.width;
    rect_params.height = dgaccelerator->video_info.height;

    /* Scale and convert the frame */
    if (get_converted_mat (dgaccelerator, surface, i, &rect_params,
          scale_ratio, dgaccelerator->video_info.width,
          dgaccelerator->video_info.height) != GST_FLOW_OK) {
      goto error;
    }
    // processes the frame using the DgAcceleratorProcess function and attaches
    // the metadata for the full frame
    // Output is a DgAcceleratorOutput object!
    output =
        DgAcceleratorProcess (dgaccelerator->dgacceleratorlib_ctx,
        dgaccelerator->cvmat->data);
    /* Attach the metadata for the full frame */
    attach_metadata_full_frame (dgaccelerator, frame_meta, scale_ratio, output, i);
    i++;

    // frees the output and returns the GstFlowReturn
    // free (output);
  }
  flow_ret = GST_FLOW_OK;

error:
  nvds_set_output_system_timestamp (inbuf, GST_ELEMENT_NAME (dgaccelerator));
  gst_buffer_unmap (inbuf, &in_map_info);
  return flow_ret;
}

/**
 * Attach metadata for the processed video frame, using NvDsBatch Meta
 */
static void
attach_metadata_full_frame (GstDgAccelerator * dgaccelerator, NvDsFrameMeta *frame_meta,
    gdouble scale_ratio, DgAcceleratorOutput * output, guint batch_id)
{
  DG_TRC_BLOCK(DgAccelerator, attach_metadata_full_frame, DGTrace::lvlBasic);
  
  NvDsBatchMeta *batch_meta = frame_meta->base_meta.batch_meta;
  NvDsObjectMeta *object_meta = NULL;
  static gchar font_name[] = "Serif";
  GST_DEBUG_OBJECT (dgaccelerator, "Attaching metadata %d\n", output->numObjects);

  for (gint i = 0; i < output->numObjects; i++) {
    DgAcceleratorObject *obj = &output->object[i];
    object_meta = nvds_acquire_obj_meta_from_pool(batch_meta);
    NvOSD_RectParams & rect_params = object_meta->rect_params;
    NvOSD_TextParams & text_params = object_meta->text_params;

    /* Assign bounding box coordinates */
    rect_params.left = obj->left;
    rect_params.top = obj->top;
    rect_params.width = obj->width;
    rect_params.height = obj->height;

    /* Semi-transparent yellow background */
    rect_params.has_bg_color = 0;
    rect_params.bg_color = (NvOSD_ColorParams) {
    1, 1, 0, 0.4};
    /* Red border of width 6 */
    rect_params.border_width = 3;
    rect_params.border_color = (NvOSD_ColorParams) {
    1, 0, 0, 1};

    /* Scale the bounding boxes proportionally based on how the object/frame was
     * scaled during input */
    rect_params.left /= scale_ratio;
    rect_params.top /= scale_ratio;
    rect_params.width /= scale_ratio;
    rect_params.height /= scale_ratio;
    GST_DEBUG_OBJECT (dgaccelerator, "Attaching rect%d of batch%u"
        "  left->%f top->%f width->%f"
        " height->%f label->%s\n", i, batch_id, rect_params.left,
        rect_params.top, rect_params.width, rect_params.height, obj->label);

    object_meta->object_id = UNTRACKED_OBJECT_ID;
    g_strlcpy (object_meta->obj_label, obj->label, MAX_LABEL_SIZE);
    /* display_text required heap allocated memory */
    text_params.display_text = g_strdup (obj->label);
    /* Display text above the left top corner of the object */
    text_params.x_offset = rect_params.left;
    text_params.y_offset = rect_params.top - 10;
    /* Set black background for the text */
    text_params.set_bg_clr = 1;
    text_params.text_bg_clr = (NvOSD_ColorParams) {
    0, 0, 0, 1};
    /* Font face, size and color */
    text_params.font_params.font_name = font_name;
    text_params.font_params.font_size = 11;
    text_params.font_params.font_color = (NvOSD_ColorParams) {
    1, 1, 1, 1};

    nvds_add_obj_meta_to_frame(frame_meta, object_meta, NULL);
    frame_meta->bInferDone = TRUE;
  }
}

// Register everything in the plugin
static gboolean
dgaccelerator_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_dgaccelerator_debug, "dgaccelerator", 0,
      "dgaccelerator plugin");

  return gst_element_register (plugin, "dgaccelerator", GST_RANK_PRIMARY,
      GST_TYPE_DGACCELERATOR);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    nvdsgst_dgaccelerator,
    DESCRIPTION, dgaccelerator_plugin_init, VERSION, LICENSE, BINARY_PACKAGE, URL)
