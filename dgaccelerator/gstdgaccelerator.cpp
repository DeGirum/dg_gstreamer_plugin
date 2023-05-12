//////////////////////////////////////////////////////////////////////
///  \file  gstdgaccelerator.cpp
///  \brief DgAccelerator wrapper element implementation
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

#include <string.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include <ostream>
#include <sstream>
#include <string>

#include "gstdgaccelerator.h"
#include "nvdefines.h"

GST_DEBUG_CATEGORY_STATIC( gst_dgaccelerator_debug );
#define GST_CAT_DEFAULT gst_dgaccelerator_debug
#define USE_EGLIMAGE    1
static GQuark _dsmeta_quark = 0;

// Enum to identify properties
enum
{
	PROP_0,
	PROP_UNIQUE_ID,
	PROP_PROCESSING_WIDTH,
	PROP_PROCESSING_HEIGHT,
	PROP_MODEL_NAME,
	PROP_SERVER_IP,
	PROP_CLOUD_TOKEN,
	PROP_BOX_COLOR,
	PROP_DROP_FRAMES,
	PROP_GPU_DEVICE_ID
};

// DEFAULT PROPERTY VALUES
#define DEFAULT_UNIQUE_ID         15
#define DEFAULT_PROCESSING_WIDTH  512
#define DEFAULT_PROCESSING_HEIGHT 512
#define DEFAULT_GPU_ID            0
#define DEFAULT_MODEL_NAME        "yolo_v5s_coco--512x512_quant_n2x_orca_1"
#define DEFAULT_SERVER_IP         "127.0.0.1"
#define DEFAULT_CLOUD_TOKEN       ""
#define DEFAULT_DROP_FRAMES       true
#define DEFAULT_BOX_COLOR         DGACCELERATOR_BOX_COLOR_RED

// NVIDIA hardware-allocated memory
#define GST_CAPS_FEATURE_MEMORY_NVMM "memory:NVMM"

// Templates for sink and source pad
static GstStaticPadTemplate gst_dgaccelerator_sink_template = GST_STATIC_PAD_TEMPLATE(
	"sink",
	GST_PAD_SINK,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS( GST_VIDEO_CAPS_MAKE_WITH_FEATURES( GST_CAPS_FEATURE_MEMORY_NVMM, "{ NV12, RGBA, I420 }" ) ) );

static GstStaticPadTemplate gst_dgaccelerator_src_template = GST_STATIC_PAD_TEMPLATE(
	"src",
	GST_PAD_SRC,
	GST_PAD_ALWAYS,
	GST_STATIC_CAPS( GST_VIDEO_CAPS_MAKE_WITH_FEATURES( GST_CAPS_FEATURE_MEMORY_NVMM, "{ NV12, RGBA, I420 }" ) ) );

#define gst_dgaccelerator_parent_class parent_class
G_DEFINE_TYPE( GstDgAccelerator, gst_dgaccelerator, GST_TYPE_BASE_TRANSFORM );

static void gst_dgaccelerator_set_property( GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec );
static void gst_dgaccelerator_get_property( GObject *object, guint prop_id, GValue *value, GParamSpec *pspec );
static gboolean gst_dgaccelerator_set_caps( GstBaseTransform *btrans, GstCaps *incaps, GstCaps *outcaps );
static gboolean gst_dgaccelerator_start( GstBaseTransform *btrans );
static gboolean gst_dgaccelerator_stop( GstBaseTransform *btrans );
static GstFlowReturn gst_dgaccelerator_transform_ip( GstBaseTransform *btrans, GstBuffer *inbuf );
static void attach_metadata_full_frame(
	GstDgAccelerator *dgaccelerator,
	NvDsFrameMeta *frame_meta,
	gdouble scale_ratio,
	DgAcceleratorOutput *output,
	guint batch_id );
static void releaseSegmentationMeta( gpointer data, gpointer user_data );
static gpointer copySegmentationMeta( gpointer data, gpointer user_data );
void attachSegmentationMetadata( NvDsFrameMeta *frameMeta, guint64 frame_num, int width, int height, const int *class_map );

#define GST_TYPE_DGACCELERATOR_BOX_COLOR ( gst_dgaccelerator_box_color_get_type() )

/// \brief Box Color get type function
/// \return the color of the box
static GType gst_dgaccelerator_box_color_get_type( void )
{
	static GType dgaccelerator_box_color_type = 0;
	static const GEnumValue dgaccelerator_box_color[] = {
		{ DGACCELERATOR_BOX_COLOR_RED, "Red Box Color", "red" },
		{ DGACCELERATOR_BOX_COLOR_GREEN, "Green Box Color", "green" },
		{ DGACCELERATOR_BOX_COLOR_BLUE, "Blue Box Color", "blue" },
		{ DGACCELERATOR_BOX_COLOR_YELLOW, "Yellow Box Color", "yellow" },
		{ DGACCELERATOR_BOX_COLOR_CYAN, "Cyan Box Color", "cyan" },
		{ DGACCELERATOR_BOX_COLOR_PINK, "Pink Box Color", "pink" },
		{ DGACCELERATOR_BOX_COLOR_BLACK, "Black Box Color", "black" },
		{ 0, NULL, NULL },
	};

	if( !dgaccelerator_box_color_type )
	{
		dgaccelerator_box_color_type = g_enum_register_static( "GstDgAcceleratorBoxColor", dgaccelerator_box_color );
	}
	return dgaccelerator_box_color_type;
}

/// \brief Installs the object and BaseTransform properties along with pads
/// \param[in] klass gstreamer boilerplate input class
static void gst_dgaccelerator_class_init( GstDgAcceleratorClass *klass )
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;
	GstBaseTransformClass *gstbasetransform_class;

	// Indicates we want to use DS buf api
	g_setenv( "DS_NEW_BUFAPI", "1", TRUE );

	gobject_class = (GObjectClass *)klass;
	gstelement_class = (GstElementClass *)klass;
	gstbasetransform_class = (GstBaseTransformClass *)klass;

	// Overide base class functions
	gobject_class->set_property = GST_DEBUG_FUNCPTR( gst_dgaccelerator_set_property );
	gobject_class->get_property = GST_DEBUG_FUNCPTR( gst_dgaccelerator_get_property );

	gstbasetransform_class->set_caps = GST_DEBUG_FUNCPTR( gst_dgaccelerator_set_caps );
	gstbasetransform_class->start = GST_DEBUG_FUNCPTR( gst_dgaccelerator_start );
	gstbasetransform_class->stop = GST_DEBUG_FUNCPTR( gst_dgaccelerator_stop );

	gstbasetransform_class->transform_ip = GST_DEBUG_FUNCPTR( gst_dgaccelerator_transform_ip );

	// Install properties
	g_object_class_install_property(
		gobject_class,
		PROP_UNIQUE_ID,
		g_param_spec_uint(
			"unique-id",
			"Unique ID",
			"Unique ID for the element. Can be used to identify output of the"
			" element",
			0,
			G_MAXUINT,
			DEFAULT_UNIQUE_ID,
			(GParamFlags)( G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ) ) );

	g_object_class_install_property(
		gobject_class,
		PROP_PROCESSING_WIDTH,
		g_param_spec_int(
			"processing-width",
			"Processing Width",
			"Width of the input buffer to algorithm",
			1,
			G_MAXINT,
			DEFAULT_PROCESSING_WIDTH,
			(GParamFlags)( G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ) ) );

	g_object_class_install_property(
		gobject_class,
		PROP_PROCESSING_HEIGHT,
		g_param_spec_int(
			"processing-height",
			"Processing Height",
			"Height of the input buffer to algorithm",
			1,
			G_MAXINT,
			DEFAULT_PROCESSING_HEIGHT,
			(GParamFlags)( G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ) ) );

	g_object_class_install_property(
		gobject_class,
		PROP_MODEL_NAME,
		g_param_spec_string( "model_name", "model_name", "Full model name", DEFAULT_MODEL_NAME, G_PARAM_READWRITE ) );

	g_object_class_install_property(
		gobject_class,
		PROP_SERVER_IP,
		g_param_spec_string( "server_ip", "server_ip", "Full server IP", DEFAULT_SERVER_IP, G_PARAM_READWRITE ) );

	g_object_class_install_property(
		gobject_class,
		PROP_CLOUD_TOKEN,
		g_param_spec_string( "cloud_token", "cloud_token", "Cloud token for non-local inference", DEFAULT_CLOUD_TOKEN, G_PARAM_READWRITE ) );

	g_object_class_install_property(
		gobject_class,
		PROP_BOX_COLOR,
		g_param_spec_enum(
			"box-color",
			"Box Color",
			"Box Color for visualization",
			GST_TYPE_DGACCELERATOR_BOX_COLOR,
			DEFAULT_BOX_COLOR,
			GParamFlags( G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ) ) );

	g_object_class_install_property(
		gobject_class,
		PROP_DROP_FRAMES,
		g_param_spec_boolean(
			"drop_frames",
			"drop_frames",
			"Toggle for skipping buffers if required for performance. Keep this on for visualization pipelines.",
			DEFAULT_DROP_FRAMES,
			G_PARAM_READWRITE ) );

	g_object_class_install_property(
		gobject_class,
		PROP_GPU_DEVICE_ID,
		g_param_spec_uint(
			"gpu-id",
			"Set GPU Device ID",
			"Set GPU Device ID",
			0,
			G_MAXUINT,
			0,
			GParamFlags( G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | GST_PARAM_MUTABLE_READY ) ) );
	// Set sink and src pad capabilities
	gst_element_class_add_pad_template( gstelement_class, gst_static_pad_template_get( &gst_dgaccelerator_src_template ) );
	gst_element_class_add_pad_template( gstelement_class, gst_static_pad_template_get( &gst_dgaccelerator_sink_template ) );

	// Set metadata describing the element
	gst_element_class_set_details_simple(
		gstelement_class,
		"DgAccelerator plugin",
		"DgAccelerator Plugin",
		"Uses NVIDIA's 3rdparty algorithm wrapper to process video frames",
		"Stephan Sokolov < stephan@degirum.ai >" );
}
/// \brief Initializes the GstDgAccelerator element and sets the BaseTransform properties for in-place mode.
///
/// This function sets up the GstDgAccelerator instance and configures the underlying BaseTransform
/// properties to work in in-place mode, allowing the element to process data without additional memory copying.
///
/// \param[in] dgaccelerator The pointer to the GstDgAccelerator instance that will be initialized.
///
static void gst_dgaccelerator_init( GstDgAccelerator *dgaccelerator )
{
	GstBaseTransform *btrans = GST_BASE_TRANSFORM( dgaccelerator );
	gst_base_transform_set_in_place( GST_BASE_TRANSFORM( btrans ), TRUE );
	gst_base_transform_set_passthrough( GST_BASE_TRANSFORM( btrans ), TRUE );

	// Initialize all property variables to default values
	dgaccelerator->unique_id = DEFAULT_UNIQUE_ID;
	dgaccelerator->processing_width = DEFAULT_PROCESSING_WIDTH;
	dgaccelerator->processing_height = DEFAULT_PROCESSING_HEIGHT;
	dgaccelerator->gpu_id = DEFAULT_GPU_ID;
	dgaccelerator->model_name = const_cast< char * >( DEFAULT_MODEL_NAME );
	dgaccelerator->server_ip = const_cast< char * >( DEFAULT_SERVER_IP );
	dgaccelerator->cloud_token = const_cast< char * >( DEFAULT_CLOUD_TOKEN );
	dgaccelerator->box_color = DEFAULT_BOX_COLOR;
	dgaccelerator->drop_frames = DEFAULT_DROP_FRAMES;

	// This quark is required to identify NvDsMeta when iterating through
	// the buffer metadatas
	if( !_dsmeta_quark )
		_dsmeta_quark = g_quark_from_static_string( NVDS_META_STRING );
}
///
/// \brief Sets the value of the specified property for the GstDgAccelerator object
///
/// This function is called when a property is set for the GstDgAccelerator object. It takes in the
/// property ID, value, and property specification, and sets the corresponding property for the object.
///
/// \param[in] object Pointer to the GObject instance of GstDgAccelerator
/// \param[in] prop_id The ID of the property to set
/// \param[in] value The new value to set for the property
/// \param[in] pspec The GParamSpec of the property
///
static void gst_dgaccelerator_set_property( GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec )
{
	GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR( object );
	switch( prop_id )
	{
	case PROP_UNIQUE_ID:
		dgaccelerator->unique_id = g_value_get_uint( value );
		break;
	case PROP_PROCESSING_WIDTH:
		dgaccelerator->processing_width = g_value_get_int( value );
		break;
	case PROP_PROCESSING_HEIGHT:
		dgaccelerator->processing_height = g_value_get_int( value );
		break;
	case PROP_GPU_DEVICE_ID:
		dgaccelerator->gpu_id = g_value_get_uint( value );
		break;
	case PROP_BOX_COLOR:
		dgaccelerator->box_color = (GstDgAcceleratorBoxColor)g_value_get_enum( value );
		break;
	case PROP_MODEL_NAME:
		// Don't allow >128 characters!
		if( strlen( g_value_get_string( value ) ) + 1 > 128 )
		{
			std::cout << "\n\nModel Name is too long! Setting the model to the default value!\n\n";
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
		}
		dgaccelerator->model_name = new char[ strlen( g_value_get_string( value ) ) + 1 ];
		strcpy( dgaccelerator->model_name, g_value_get_string( value ) );
		break;
	case PROP_SERVER_IP:
		// Don't allow >128 characters!
		if( strlen( g_value_get_string( value ) ) + 1 > 128 )
		{
			std::cout << "\n\nServer IP is too long! Setting the server ip to the default value!\n\n";
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
		}
		dgaccelerator->server_ip = new char[ strlen( g_value_get_string( value ) ) + 1 ];
		strcpy( dgaccelerator->server_ip, g_value_get_string( value ) );
		break;
	case PROP_CLOUD_TOKEN:
		// Don't allow > 128 characters!
		if( strlen( g_value_get_string( value ) ) + 1 > 128 )
		{
			std::cout << "\n\nCloud Token is too long! Setting the cloud token to the default value!\n\n";
			G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
			break;
		}
		dgaccelerator->cloud_token = new char[ strlen( g_value_get_string( value ) ) + 1 ];
		strcpy( dgaccelerator->cloud_token, g_value_get_string( value ) );
		break;
	case PROP_DROP_FRAMES:
		dgaccelerator->drop_frames = g_value_get_boolean( value );
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
		break;
	}
}

///
/// \brief Retrieves the value of the specified property for the GstDgAccelerator object
///
/// This function is called when a property value is requested for the GstDgAccelerator object. It takes
/// in the property ID and property specification, and returns the value of the corresponding property
/// through the GValue parameter.
///
/// \param[in] object Pointer to the GObject instance of GstDgAccelerator
/// \param[in] prop_id The ID of the property to retrieve
/// \param[out] value The retrieved value of the property
/// \param[in] pspec The GParamSpec of the property
///
static void gst_dgaccelerator_get_property( GObject *object, guint prop_id, GValue *value, GParamSpec *pspec )
{
	GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR( object );

	switch( prop_id )
	{
	case PROP_UNIQUE_ID:
		g_value_set_uint( value, dgaccelerator->unique_id );
		break;
	case PROP_PROCESSING_WIDTH:
		g_value_set_int( value, dgaccelerator->processing_width );
		break;
	case PROP_PROCESSING_HEIGHT:
		g_value_set_int( value, dgaccelerator->processing_height );
		break;
	case PROP_GPU_DEVICE_ID:
		g_value_set_uint( value, dgaccelerator->gpu_id );
		break;
	case PROP_BOX_COLOR:
		g_value_set_enum( value, dgaccelerator->box_color );
		break;

	case PROP_MODEL_NAME:
		g_value_set_string( value, dgaccelerator->model_name );
		break;
	case PROP_SERVER_IP:
		g_value_set_string( value, dgaccelerator->server_ip );
		break;
	case PROP_CLOUD_TOKEN:
		g_value_set_string( value, dgaccelerator->cloud_token );
		break;
	case PROP_DROP_FRAMES:
		g_value_set_boolean( value, dgaccelerator->drop_frames );
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID( object, prop_id, pspec );
		break;
	}
}

/// \brief Initializes all the parameters and CUDA stream for the GstDgAccelerator.
///
/// This function is called as a result of the BaseTransform class changing states in the pipeline.
/// It ensures that the GstDgAccelerator is properly set up and ready for processing.
///
/// \param[in] btrans The pointer to the GstBaseTransform instance that will be initialized.
///
static gboolean gst_dgaccelerator_start( GstBaseTransform *btrans )
{
	GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR( btrans );
	// NvBufSurface params for buffer creation
	NvBufSurfaceCreateParams create_params;
	// Creates the init params for our context
	DgAcceleratorInitParams init_params =
		{ dgaccelerator->processing_width, dgaccelerator->processing_height, "", "", 0, "", dgaccelerator->drop_frames };
	snprintf( init_params.model_name, 128, "%s", dgaccelerator->model_name );    // Sets the model name
	snprintf( init_params.server_ip, 128, "%s", dgaccelerator->server_ip );      // Sets the server ip
	snprintf( init_params.cloud_token, 128, "%s", dgaccelerator->cloud_token );  // Sets the cloud token

	guint batch_size = 1;
	int val = -1;
	GstQuery *queryparams = gst_nvquery_batch_size_new();

	CHECK_CUDA_STATUS( cudaSetDevice( dgaccelerator->gpu_id ), "Unable to set cuda device" );
	// Checks if GPU is integrated graphics
	cudaDeviceGetAttribute( &val, cudaDevAttrIntegrated, dgaccelerator->gpu_id );
	dgaccelerator->is_integrated = val;
	// queries the batch size of the input buffers from the upstream element and
	// sets the batch size for the plugin accordingly
	dgaccelerator->batch_size = 1;
	if( gst_pad_peer_query( GST_BASE_TRANSFORM_SINK_PAD( btrans ), queryparams ) ||
		gst_pad_peer_query( GST_BASE_TRANSFORM_SRC_PAD( btrans ), queryparams ) )
	{
		if( gst_nvquery_batch_size_parse( queryparams, &batch_size ) )
		{
			dgaccelerator->batch_size = batch_size;
		}
	}
	init_params.numInputStreams = batch_size;  // Sets the number of input streams
	gst_query_unref( queryparams );

	// Initialize our context with the init params structure
	dgaccelerator->dgacceleratorlib_ctx = DgAcceleratorCtxInit( &init_params );

	CHECK_CUDA_STATUS( cudaStreamCreate( &dgaccelerator->cuda_stream ), "Could not create cuda stream" );

	// destroy intermediate buffer if needed, prior to using it
	if( dgaccelerator->inter_buf )
		NvBufSurfaceDestroy( dgaccelerator->inter_buf );
	dgaccelerator->inter_buf = NULL;

	// handle box color for drawing
	switch( dgaccelerator->box_color )
	{
	case DGACCELERATOR_BOX_COLOR_RED:  // Red
		dgaccelerator->color = ( NvOSD_ColorParams ){ 1, 0, 0, 1 };
		break;
	case DGACCELERATOR_BOX_COLOR_GREEN:  // Green
		dgaccelerator->color = ( NvOSD_ColorParams ){ 0, 1, 0, 1 };
		break;
	case DGACCELERATOR_BOX_COLOR_BLUE:  // Blue
		dgaccelerator->color = ( NvOSD_ColorParams ){ 0, 0, 1, 1 };
		break;
	case DGACCELERATOR_BOX_COLOR_CYAN:  // Cyan
		dgaccelerator->color = ( NvOSD_ColorParams ){ 0, 1, 1, 1 };
		break;
	case DGACCELERATOR_BOX_COLOR_PINK:  // Pink
		dgaccelerator->color = ( NvOSD_ColorParams ){ 1, 0.06, 0.94, 1 };
		break;
	case DGACCELERATOR_BOX_COLOR_YELLOW:  // Yellow
		dgaccelerator->color = ( NvOSD_ColorParams ){ 1, 1, 0, 1 };
		break;
	case DGACCELERATOR_BOX_COLOR_BLACK:  // Black
		dgaccelerator->color = ( NvOSD_ColorParams ){ 0, 0, 0, 1 };
		break;
	default:  // Default to red
		dgaccelerator->color = ( NvOSD_ColorParams ){ 1, 0, 0, 1 };
		break;
	}
	// NvBufSurface params for NV12/RGBA to BGR conversion
	create_params.gpuId = dgaccelerator->gpu_id;
	create_params.width = dgaccelerator->processing_width;
	create_params.height = dgaccelerator->processing_height;
	create_params.size = 0;
	create_params.colorFormat = NVBUF_COLOR_FORMAT_RGBA;
	create_params.layout = NVBUF_LAYOUT_PITCH;

	if( dgaccelerator->is_integrated )
	{
		create_params.memType = NVBUF_MEM_DEFAULT;
	}
	else
	{
		create_params.memType = NVBUF_MEM_CUDA_PINNED;
	}

	if( NvBufSurfaceCreate( &dgaccelerator->inter_buf, 1, &create_params ) != 0 )
	{
		GST_ERROR( "Error: Could not allocate internal buffer for dgaccelerator" );
		goto error;
	}
	// Create host memory for storing converted/scaled interleaved RGB data
	CHECK_CUDA_STATUS(
		cudaMallocHost( &dgaccelerator->host_rgb_buf, dgaccelerator->processing_width * dgaccelerator->processing_height * 3 ),
		"Could not allocate cuda host buffer" );
	// CV Mat containing interleaved RGB data. This call does not allocate memory.
	// It uses host_rgb_buf as data.
	dgaccelerator->cvmat = new cv::Mat(
		dgaccelerator->processing_height,
		dgaccelerator->processing_width,
		CV_8UC3,
		dgaccelerator->host_rgb_buf,
		dgaccelerator->processing_width * 3 );
	if( !dgaccelerator->cvmat )
		goto error;

	return TRUE;
error:
	if( dgaccelerator->host_rgb_buf )
	{
		cudaFreeHost( dgaccelerator->host_rgb_buf );
		dgaccelerator->host_rgb_buf = NULL;
	}
	if( dgaccelerator->cuda_stream )
	{
		cudaStreamDestroy( dgaccelerator->cuda_stream );
		dgaccelerator->cuda_stream = NULL;
	}
	if( dgaccelerator->dgacceleratorlib_ctx )
		DgAcceleratorCtxDeinit( dgaccelerator->dgacceleratorlib_ctx );

	return FALSE;
}

///
/// \brief Stops the GstDgAccelerator element and frees memory
///
/// This function is called when the pipeline transitions to the NULL state, which triggers the
/// stop function of the GstBaseTransform class. The function frees all the memory allocated for
/// the GstDgAccelerator element and deinitializes the library.
///
/// \param[in] btrans Pointer to the GstBaseTransform instance
/// \return Returns TRUE if the element was stopped successfully, FALSE otherwise
///
static gboolean gst_dgaccelerator_stop( GstBaseTransform *btrans )
{
	GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR( btrans );

	if( dgaccelerator->inter_buf )
		NvBufSurfaceDestroy( dgaccelerator->inter_buf );
	dgaccelerator->inter_buf = NULL;

	if( dgaccelerator->cuda_stream )
		cudaStreamDestroy( dgaccelerator->cuda_stream );
	dgaccelerator->cuda_stream = NULL;

	delete dgaccelerator->cvmat;
	dgaccelerator->cvmat = NULL;

	if( dgaccelerator->host_rgb_buf )
	{
		cudaFreeHost( dgaccelerator->host_rgb_buf );
		dgaccelerator->host_rgb_buf = NULL;
	}

	// Deinitialize our library
	DgAcceleratorCtxDeinit( dgaccelerator->dgacceleratorlib_ctx );
	dgaccelerator->dgacceleratorlib_ctx = NULL;

	return TRUE;
}

///
/// \brief Callback function for when source/sink pad capabilities have been negotiated
///
/// This function is called when the source/sink pad capabilities have been negotiated, i.e., when
/// the pads' capabilities are compatible and can be set. It takes in the GstCaps instances for
/// the input and output pads and returns a boolean indicating whether the negotiation was successful.
///
/// \param[in] btrans Pointer to the GstBaseTransform instance
/// \param[in] incaps Pointer to the GstCaps instance for the input pad
/// \param[in] outcaps Pointer to the GstCaps instance for the output pad
/// \return Returns TRUE if the pad capabilities were successfully negotiated, FALSE otherwise
///
static gboolean gst_dgaccelerator_set_caps( GstBaseTransform *btrans, GstCaps *incaps, GstCaps *outcaps )
{
	GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR( btrans );
	// Save the input video information, since this will be required later.
	gst_video_info_from_caps( &dgaccelerator->video_info, incaps );

	return TRUE;

error:
	return FALSE;
}

///
/// \brief Scales the entire frame or crops and scales objects while maintaining aspect ratio
///
/// This function scales the entire frame or crops and scales objects to the processing resolution while
/// maintaining aspect ratio. It removes the padding required by hardware and converts the data from RGBA to RGB
/// using OpenCV, unless the algorithm can work with padded data and/or can work with RGBA. The input NvBufSurface
/// object is modified in-place to contain the output buffer. The function uses NVIDIA's NvBufSurfTransform API
/// to perform scaling, format conversion, and cropping, as well as OpenCV's cvtColor function to convert the RGBA
/// format to BGR format. The input buffer is cropped according to the provided crop_rect_params rectangle, and
/// the aspect ratio is maintained while scaling to a destination resolution specified by the processing_width
/// and processing_height parameters of the dgaccelerator structure.
///
/// \param[in] dgaccelerator Pointer to the GstDgAccelerator instance
/// \param[in] input_buf Pointer to the input NvBufSurface object
/// \param[in] idx Index of the surface
/// \param[in] crop_rect_params Pointer to the NvOSD_RectParams struct representing the crop rectangle parameters
/// \param[out] ratio The aspect ratio of the input buffer
/// \param[in] input_width Width of the input buffer
/// \param[in] input_height Height of the input buffer
/// \return Returns a GstFlowReturn value indicating the status of the function
///
static GstFlowReturn get_converted_mat(
	GstDgAccelerator *dgaccelerator,
	NvBufSurface *input_buf,
	gint idx,
	NvOSD_RectParams *crop_rect_params,
	gdouble &ratio,
	gint input_width,
	gint input_height )
{
	NvBufSurfTransform_Error err;
	NvBufSurfTransformConfigParams transform_config_params;
	NvBufSurfTransformParams transform_params;
	NvBufSurfTransformRect src_rect;
	NvBufSurfTransformRect dst_rect;
	NvBufSurface ip_surf;
	cv::Mat in_mat;
	ip_surf = *input_buf;

	ip_surf.numFilled = ip_surf.batchSize = 1;
	ip_surf.surfaceList = &( input_buf->surfaceList[ idx ] );

	gint src_left = GST_ROUND_UP_2( (unsigned int)crop_rect_params->left );
	gint src_top = GST_ROUND_UP_2( (unsigned int)crop_rect_params->top );
	gint src_width = GST_ROUND_DOWN_2( (unsigned int)crop_rect_params->width );
	gint src_height = GST_ROUND_DOWN_2( (unsigned int)crop_rect_params->height );

	// Maintain aspect ratio
	double hdest = dgaccelerator->processing_width * src_height / (double)src_width;
	double wdest = dgaccelerator->processing_height * src_width / (double)src_height;
	guint dest_width, dest_height;

	if( hdest <= dgaccelerator->processing_height )
	{
		dest_width = dgaccelerator->processing_width;
		dest_height = hdest;
	}
	else
	{
		dest_width = wdest;
		dest_height = dgaccelerator->processing_height;
	}

	// Configure transform session parameters for the transformation
	transform_config_params.compute_mode = NvBufSurfTransformCompute_Default;
	transform_config_params.gpu_id = dgaccelerator->gpu_id;
	transform_config_params.cuda_stream = dgaccelerator->cuda_stream;

	// Set the transform session parameters for the conversions executed in this
	// thread.
	err = NvBufSurfTransformSetSessionParams( &transform_config_params );
	if( err != NvBufSurfTransformError_Success )
	{
		GST_ELEMENT_ERROR( dgaccelerator, STREAM, FAILED, ( "NvBufSurfTransformSetSessionParams failed with error %d", err ), ( NULL ) );
		goto error;
	}

	// Calculate scaling ratio while maintaining aspect ratio
	ratio = MIN( 1.0 * dest_width / src_width, 1.0 * dest_height / src_height );

	if( ( crop_rect_params->width == 0 ) || ( crop_rect_params->height == 0 ) )
	{
		GST_ELEMENT_ERROR( dgaccelerator, STREAM, FAILED, ( "%s:crop_rect_params dimensions are zero", __func__ ), ( NULL ) );
		goto error;
	}

#ifdef __aarch64__
	if( ratio <= 1.0 / 16 || ratio >= 16.0 )
	{
		// Currently cannot scale by ratio > 16 or < 1/16 for Jetson
		goto error;
	}
#endif
	// Set the transform ROIs for source and destination
	src_rect = { (guint)src_top, (guint)src_left, (guint)src_width, (guint)src_height };
	dst_rect = { 0, 0, (guint)dest_width, (guint)dest_height };

	// Set the transform parameters
	transform_params.src_rect = &src_rect;
	transform_params.dst_rect = &dst_rect;
	transform_params.transform_flag = NVBUFSURF_TRANSFORM_FILTER | NVBUFSURF_TRANSFORM_CROP_SRC | NVBUFSURF_TRANSFORM_CROP_DST;
	transform_params.transform_filter = NvBufSurfTransformInter_Default;

	// Memset the memory
	NvBufSurfaceMemSet( dgaccelerator->inter_buf, 0, 0, 0 );

	// Transformation scaling+format conversion if any.
	err = NvBufSurfTransform( &ip_surf, dgaccelerator->inter_buf, &transform_params );
	if( err != NvBufSurfTransformError_Success )
	{
		GST_ELEMENT_ERROR( dgaccelerator, STREAM, FAILED, ( "NvBufSurfTransform failed with error %d while converting buffer", err ), ( NULL ) );
		goto error;
	}
	// Map the buffer so that it can be accessed by CPU
	if( NvBufSurfaceMap( dgaccelerator->inter_buf, 0, 0, NVBUF_MAP_READ ) != 0 )
	{
		goto error;
	}
	if( dgaccelerator->inter_buf->memType == NVBUF_MEM_SURFACE_ARRAY )
	{
		// Cache the mapped data for CPU access
		NvBufSurfaceSyncForCpu( dgaccelerator->inter_buf, 0, 0 );
	}

	// Use OpenCV to remove padding and convert RGBA to BGR.
	in_mat = cv::Mat(
		dgaccelerator->processing_height,
		dgaccelerator->processing_width,
		CV_8UC4,
		dgaccelerator->inter_buf->surfaceList[ 0 ].mappedAddr.addr[ 0 ],
		dgaccelerator->inter_buf->surfaceList[ 0 ].pitch );

#if( CV_MAJOR_VERSION >= 4 )
	cv::cvtColor( in_mat, *dgaccelerator->cvmat, cv::COLOR_RGBA2BGR );
#else
	cv::cvtColor( in_mat, *dgaccelerator->cvmat, CV_RGBA2BGR );
#endif

	if( NvBufSurfaceUnMap( dgaccelerator->inter_buf, 0, 0 ) )
	{
		goto error;
	}

	if( dgaccelerator->is_integrated )
	{
#ifdef __aarch64__
		// To use the converted buffer in CUDA, create an EGLImage and then use
		// CUDA-EGL interop APIs
		if( USE_EGLIMAGE )
		{
			if( NvBufSurfaceMapEglImage( dgaccelerator->inter_buf, 0 ) != 0 )
			{
				goto error;
			}

			// dgaccelerator->inter_buf->surfaceList[0].mappedAddr.eglImage
			// Use interop APIs cuGraphicsEGLRegisterImage and
			// cuGraphicsResourceGetMappedEglFrame to access the buffer in CUDA

			// Destroy the EGLImage
			NvBufSurfaceUnMapEglImage( dgaccelerator->inter_buf, 0 );
		}
#endif
	}
	return GST_FLOW_OK;

error:
	return GST_FLOW_ERROR;
}

///
/// \brief Main processing function for the GstDgAccelerator element
///
/// This function is called when an input buffer is received by the GstDgAccelerator element. It performs
/// in-place transformation of the buffer using the static library. The function uses the input buffer
/// to update the GstBuffer instance in-place.
///
/// \param[in] btrans Pointer to the GstBaseTransform instance
/// \param[in] inbuf Pointer to the input GstBuffer
/// \return Returns a GstFlowReturn value indicating the status of the function
///
static GstFlowReturn gst_dgaccelerator_transform_ip( GstBaseTransform *btrans, GstBuffer *inbuf )
{
	// initializes NVIDIA buffer and metadata variables.
	GstDgAccelerator *dgaccelerator = GST_DGACCELERATOR( btrans );
	GstMapInfo in_map_info;
	GstFlowReturn flow_ret = GST_FLOW_ERROR;
	gdouble scale_ratio = 1.0;

	// Initialize an empty frame of objects
	DgAcceleratorOutput *output;

	NvBufSurface *surface = NULL;
	NvDsBatchMeta *batch_meta = NULL;
	NvDsFrameMeta *frame_meta = NULL;
	NvDsMetaList *l_frame = NULL;
	guint i = 0;  // frame number in the batch

	dgaccelerator->frame_num++;
	CHECK_CUDA_STATUS( cudaSetDevice( dgaccelerator->gpu_id ), "Unable to set cuda device" );

	// maps the input buffer to get the input NvBufSurface.
	memset( &in_map_info, 0, sizeof( in_map_info ) );
	if( !gst_buffer_map( inbuf, &in_map_info, GST_MAP_READ ) )
	{
		g_print( "Error: Failed to map gst buffer\n" );
		goto error;
	}
	nvds_set_input_system_timestamp( inbuf, GST_ELEMENT_NAME( dgaccelerator ) );
	surface = (NvBufSurface *)in_map_info.data;

	// checks the NvBufSurface to ensure it is valid and that the memory is
	// allocated on the correct GPU
	if( CHECK_NVDS_MEMORY_AND_GPUID( dgaccelerator, surface ) )
		goto error;

	// retrieves the NvDsBatchMeta from the input buffer and processes each
	// NvDsFrameMeta in the batch
	batch_meta = gst_buffer_get_nvds_batch_meta( inbuf );
	if( batch_meta == nullptr )
	{
		GST_ELEMENT_ERROR( dgaccelerator, STREAM, FAILED, ( "NvDsBatchMeta not found for input buffer." ), ( NULL ) );
		return GST_FLOW_ERROR;
	}

	// sets the scaling parameters for the frame and scales and converts the
	// frame using the get_converted_mat function
	for( l_frame = batch_meta->frame_meta_list; l_frame != NULL; l_frame = l_frame->next )
	{
		frame_meta = (NvDsFrameMeta *)( l_frame->data );
		NvOSD_RectParams rect_params;

		// Scale the entire frame to processing resolution
		rect_params.left = 0;
		rect_params.top = 0;
		rect_params.width = dgaccelerator->video_info.width;
		rect_params.height = dgaccelerator->video_info.height;

		// Scale and convert the frame
		if( get_converted_mat(
				dgaccelerator,
				surface,
				i,
				&rect_params,
				scale_ratio,
				dgaccelerator->video_info.width,
				dgaccelerator->video_info.height ) != GST_FLOW_OK )
		{
			goto error;
		}
		// processes the frame using the DgAcceleratorProcess function and attaches
		// the metadata for the full frame
		// Output is a DgAcceleratorOutput object!
		output = DgAcceleratorProcess( dgaccelerator->dgacceleratorlib_ctx, dgaccelerator->cvmat->data );

		// Add a check here to return GST_FLOW_ERROR with the correct error?
		// if( error_found )
		// {
		// 	GST_ELEMENT_ERROR( dgaccelerator, STREAM, FAILED, ( "error message" ), ( NULL ) );
		// 	flow_ret = GST_FLOW_ERROR;
		//	goto error;
		// }

		// Attach the metadata for the full frame
		attach_metadata_full_frame( dgaccelerator, frame_meta, scale_ratio, output, i );
		i++;
	}

	flow_ret = GST_FLOW_OK;

error:
	nvds_set_output_system_timestamp( inbuf, GST_ELEMENT_NAME( dgaccelerator ) );
	gst_buffer_unmap( inbuf, &in_map_info );
	return flow_ret;
}

///
/// \brief Attaches metadata for the processed video frame using NvDsBatch Meta
///
/// This function attaches metadata for the processed video frame using NvDsBatch Meta. It takes in the
/// GstDgAccelerator instance, NvDsFrameMeta instance for the video frame, scale ratio, DgAcceleratorOutput
/// instance for the output, and batch ID. The function updates the NvDsBatchMeta with the metadata for the
/// processed video frame.
///
/// \param[in] dgaccelerator Pointer to the GstDgAccelerator instance
/// \param[in] frame_meta Pointer to the NvDsFrameMeta instance for the video frame
/// \param[in] scale_ratio The scale ratio used for processing the frame
/// \param[in] output Pointer to the DgAcceleratorOutput instance for the output
/// \param[in] batch_id The frame number in the batch, unused
///
static void attach_metadata_full_frame(
	GstDgAccelerator *dgaccelerator,
	NvDsFrameMeta *frame_meta,
	gdouble scale_ratio,
	DgAcceleratorOutput *output,
	guint batch_id )
{
	NvDsBatchMeta *batch_meta = frame_meta->base_meta.batch_meta;
	NvDsObjectMeta *object_meta = NULL;
	static gchar font_name[] = "Serif";
	// Set width / height to be the source frame width / height
	int frame_width = frame_meta->source_frame_width;
	int frame_height = frame_meta->source_frame_height;
	// Object Detection loop in DgAcceleratorOutput
	for( gint i = 0; i < output->numObjects; i++ )
	{
		DgAcceleratorObject *obj = &output->object[ i ];
		object_meta = nvds_acquire_obj_meta_from_pool( batch_meta );
		NvOSD_RectParams &rect_params = object_meta->rect_params;
		NvOSD_TextParams &text_params = object_meta->text_params;

		// Assign bounding box coordinates
		rect_params.left = obj->left;
		rect_params.top = obj->top;
		rect_params.width = obj->width;
		rect_params.height = obj->height;

		// Background color for rectangle, default off
		rect_params.has_bg_color = 0;
		rect_params.bg_color = ( NvOSD_ColorParams ){ 1, 1, 0, 0.4 };
		// Set box border width
		rect_params.border_width = 3;
		// Set box color
		rect_params.border_color = dgaccelerator->color;

		// Scale the bounding boxes proportionally based on how the object/frame was
		// scaled during input
		rect_params.left /= scale_ratio;
		rect_params.top /= scale_ratio;
		rect_params.width /= scale_ratio;
		rect_params.height /= scale_ratio;

		object_meta->object_id = UNTRACKED_OBJECT_ID;
		g_strlcpy( object_meta->obj_label, obj->label, MAX_LABEL_SIZE );
		// display_text requires heap allocated memory
		text_params.display_text = g_strdup( obj->label );
		// Display text above the left top corner of the object
		text_params.x_offset = rect_params.left;
		text_params.y_offset = rect_params.top - 10;
		// Set black background for the text
		text_params.set_bg_clr = 1;
		text_params.text_bg_clr = ( NvOSD_ColorParams ){ 0, 0, 0, 1 };
		// Font face, size and color
		text_params.font_params.font_name = font_name;
		text_params.font_params.font_size = 11;
		text_params.font_params.font_color = ( NvOSD_ColorParams ){ 1, 1, 1, 1 };

		nvds_add_obj_meta_to_frame( frame_meta, object_meta, NULL );
	}
	// Pose Estimation in DgAcceleratorOutput
	for( gint j = 0; j < output->numPoses; j++ )
	{
		DgAcceleratorPose *pose = &output->pose[ j ];
		NvDsDisplayMeta *dmeta = nvds_acquire_display_meta_from_pool( batch_meta );
		nvds_add_display_meta_to_frame( frame_meta, dmeta );

		for( const auto &landmark : pose->landmarks )
		{
			int x = static_cast< int >( landmark.point.first );
			int y = static_cast< int >( landmark.point.second );
			// scale back
			x /= scale_ratio;
			y /= scale_ratio;
			if( dmeta->num_circles == MAX_ELEMENTS_IN_DISPLAY_META )
			{
				dmeta = nvds_acquire_display_meta_from_pool( batch_meta );
				nvds_add_display_meta_to_frame( frame_meta, dmeta );
			}
			// Add circle at each landmark
			NvOSD_CircleParams &cparams = dmeta->circle_params[ dmeta->num_circles ];
			cparams.xc = x;
			cparams.yc = y;
			cparams.radius = 8;
			cparams.circle_color = NvOSD_ColorParams{ 0, 255, 0, 1 };
			cparams.has_bg_color = 1;
			cparams.bg_color = NvOSD_ColorParams{ 200, 0, 40, 1 };
			dmeta->num_circles++;

			// Add lines
			for( int connection_index : landmark.connection )
			{
				if( connection_index >= 0 && connection_index < pose->landmarks.size() )
				{
					auto &connected_landmark = pose->landmarks[ connection_index ];
					int x1 = static_cast< int >( connected_landmark.point.first );
					int y1 = static_cast< int >( connected_landmark.point.second );
					// scale back
					x1 /= scale_ratio;
					y1 /= scale_ratio;
					if( dmeta->num_lines == MAX_ELEMENTS_IN_DISPLAY_META )
					{
						dmeta = nvds_acquire_display_meta_from_pool( batch_meta );
						nvds_add_display_meta_to_frame( frame_meta, dmeta );
					}
					NvOSD_LineParams &lparams = dmeta->line_params[ dmeta->num_lines ];
					lparams.x1 = x;
					lparams.x2 = x1;
					lparams.y1 = y;
					lparams.y2 = y1;
					lparams.line_width = 3;
					lparams.line_color = NvOSD_ColorParams{ 255, 0, 0, 1 };
					dmeta->num_lines++;
				}
			}
		}
		// nvds_add_display_meta_to_frame(frame_meta, dmeta);
	}
	// Classification loop in DgAcceleratorOutput
	for( int i = 0; i < output->k; i++ )
	{
		DgAcceleratorClassObject *class_obj = &output->classifiedObject[ i ];
		object_meta = nvds_acquire_obj_meta_from_pool( batch_meta );
		NvOSD_TextParams &text_params = object_meta->text_params;

		// Display the label and score as text above the frame
		text_params.display_text = g_strdup_printf( "%s: %.2f", class_obj->label, class_obj->score );
		text_params.x_offset = 10;
		text_params.y_offset = 30 + i * 20;  // Adjust the y-offset for each label
		text_params.font_params.font_name = font_name;
		text_params.font_params.font_size = 11;
		text_params.font_params.font_color = ( NvOSD_ColorParams ){ 1, 1, 1, 1 };

		// Add a dark background behind the text
		text_params.set_bg_clr = 1;
		text_params.text_bg_clr = ( NvOSD_ColorParams ){ 0, 0, 0, 1 };  // Set background color to black

		nvds_add_obj_meta_to_frame( frame_meta, object_meta, NULL );
	}

	// Segmentation loop in DgAcceleratorOutput
	if( !output->segMap.class_map.empty() )
	{
		// Resize the segmentation map to original frame dimensions
		// Convert class_map to cv::Mat
		cv::Mat classMapMat( output->segMap.mask_height, output->segMap.mask_width, CV_32S, output->segMap.class_map.data() );
		// Create a new cv::Mat for the resized map
		cv::Mat resizedClassMapMat;
		// Resize the class map
		cv::resize( classMapMat, resizedClassMapMat, cv::Size( frame_width, frame_height ), 0, 0, cv::INTER_NEAREST );
		
		// attach the segmentation metadata to the frame
		attachSegmentationMetadata( frame_meta, dgaccelerator->frame_num, frame_width, frame_height, (const int *)resizedClassMapMat.data );
	}
	frame_meta->bInferDone = TRUE;
}
//////////// SEGMENTATION META FUNCTIONS //////////////////

///
/// \brief Releases the memory associated with the given segmentation metadata.
///
/// This function releases the memory associated with the given segmentation metadata. It first checks if the metadata
/// is valid and if the class_map and class_probabilities_map pointers are not null. If they are not null, it frees the
/// memory associated with them. Finally, it deletes the metadata object and sets the user_meta_data
/// pointer to null.
///
/// \param[in] data A gpointer to the user meta data to be released.
/// \param[in] user_data Unused, but required
///
static void releaseSegmentationMeta( gpointer data, gpointer user_data )
{
	if( data == nullptr )
		return;

	NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
	assert( user_meta != nullptr );
	assert( user_meta->base_meta.meta_type == NVDSINFER_SEGMENTATION_META );

	NvDsInferSegmentationMeta *segm_meta = (NvDsInferSegmentationMeta *)user_meta->user_meta_data;
	if( segm_meta != nullptr )
	{
		if( segm_meta->class_map != nullptr )
		{
			delete[] segm_meta->class_map;  // Use delete[] to deallocate memory allocated with new[]
			segm_meta->class_map = nullptr;
		}

		if( segm_meta->class_probabilities_map != nullptr )
		{
			delete[] segm_meta->class_probabilities_map;
			segm_meta->class_probabilities_map = nullptr;
		}
		delete segm_meta;
		user_meta->user_meta_data = nullptr;
	}
}
///
/// \brief Creates a deep copy of the given segmentation metadata.
///
/// This function creates a deep copy of the given segmentation metadata. It first checks if the metadata is valid and if
/// the class_map pointer is not null. If it is not null and the width, height, and classes fields are positive, it creates
/// a new metadata object and sets its fields to the same values as the source metadata. It then allocates memory for the
/// class_map using g_memdup() function and copies the data from the source class_map to the new class_map. Finally, it
/// returns the new metadata object.
///
/// \param[in] data A gpointer to the user meta data to be copied.
/// \param[in] user_data Unused, but required
/// \return A gpointer to the newly created copy of the user meta data.
///
static gpointer copySegmentationMeta( gpointer data, gpointer user_data )
{
	NvDsUserMeta *user_meta = (NvDsUserMeta *)data;
	assert( user_meta != nullptr );
	assert( user_meta->base_meta.meta_type == NVDSINFER_SEGMENTATION_META );

	NvDsInferSegmentationMeta *segm_meta = (NvDsInferSegmentationMeta *)user_meta->user_meta_data;
	assert( segm_meta != nullptr );

	NvDsInferSegmentationMeta *ret = new NvDsInferSegmentationMeta();
	std::memset( ret, 0, sizeof *ret );

	// copy the data to meta
	ret->unique_id = segm_meta->unique_id;
	ret->classes = segm_meta->classes;
	ret->width = segm_meta->width;
	ret->height = segm_meta->height;

	if( segm_meta->class_map != nullptr )
	{
		const size_t class_map_cnt = segm_meta->width * segm_meta->height;
		ret->class_map = new int[ class_map_cnt ];
		std::memcpy( ret->class_map, segm_meta->class_map, class_map_cnt * sizeof( int ) );
	}

	if( segm_meta->class_probabilities_map != nullptr )
	{
		const size_t prob_map_cnt = segm_meta->classes * segm_meta->width * segm_meta->height;
		ret->class_probabilities_map = new float[ prob_map_cnt ];
		std::memcpy( ret->class_probabilities_map, segm_meta->class_probabilities_map, prob_map_cnt * sizeof( float ) );
	}

	return ret;
}

///
/// \brief Attaches the given segmentation metadata to the given frame metadata.
///
/// This function attaches the given segmentation metadata to the given frame metadata. It first checks if the given frame
/// metadata and batch metadata are valid. It then acquires the metadata lock using nvds_acquire_meta_lock() function. It
/// acquires a user meta from the pool using nvds_acquire_user_meta_from_pool() function. It then creates a new segmentation
/// metadata object and sets its fields to the values from the given segOutput. It sets the user meta data pointer to point
/// to the new metadata object, and sets the base_meta fields accordingly. It then adds the user meta to the frame using
/// nvds_add_user_meta_to_frame() function. Finally, it releases the metadata lock using nvds_release_meta_lock() function.
///
/// \param[in] frameMeta A pointer to the frame metadata to attach the segmentation metadata to.
///
void attachSegmentationMetadata( NvDsFrameMeta *frameMeta, guint64 frame_num, int width, int height, const int *class_map )
{
	assert( frameMeta );
	NvDsBatchMeta *batchMeta = frameMeta->base_meta.batch_meta;

	assert( batchMeta );
	nvds_acquire_meta_lock( batchMeta );

	NvDsUserMeta *user_meta = nvds_acquire_user_meta_from_pool( batchMeta );
	NvDsInferSegmentationMeta *segm_meta = new NvDsInferSegmentationMeta();
	segm_meta->unique_id = frame_num;
	segm_meta->classes = UINT_MAX;
	segm_meta->width = width;
	segm_meta->height = height;
	segm_meta->class_map = new int[ width * height ];
	std::memcpy( segm_meta->class_map, class_map, width * height * sizeof( int ) );
	segm_meta->class_probabilities_map = nullptr;
	segm_meta->priv_data = nullptr;

	user_meta->user_meta_data = segm_meta;

	user_meta->base_meta.meta_type = (NvDsMetaType)NVDSINFER_SEGMENTATION_META;
	user_meta->base_meta.release_func = releaseSegmentationMeta;
	user_meta->base_meta.copy_func = copySegmentationMeta;

	// add the meta to frame
	assert( frameMeta );
	nvds_add_user_meta_to_frame( frameMeta, user_meta );

	nvds_release_meta_lock( batchMeta );
}

///
/// \brief Initializes the GstDgAccelerator plugin
///
/// This function registers all the elements and features provided by the GstDgAccelerator plugin.
///
/// \param[in] plugin Pointer to the GstPlugin instance
/// \return Returns TRUE if the plugin was initialized successfully, FALSE otherwise
///
static gboolean dgaccelerator_plugin_init( GstPlugin *plugin )
{
	GST_DEBUG_CATEGORY_INIT( gst_dgaccelerator_debug, "dgaccelerator", 0, "dgaccelerator plugin" );

	return gst_element_register( plugin, "dgaccelerator", GST_RANK_PRIMARY, GST_TYPE_DGACCELERATOR );
}

GST_PLUGIN_DEFINE(
	GST_VERSION_MAJOR,
	GST_VERSION_MINOR,
	nvdsgst_dgaccelerator,
	DESCRIPTION,
	dgaccelerator_plugin_init,
	VERSION,
	LICENSE,
	BINARY_PACKAGE,
	URL )
