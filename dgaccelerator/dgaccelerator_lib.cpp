//////////////////////////////////////////////////////////////////////
///  \file  dgaccelerator_lib.cpp
///  \brief DgAccelerator context for handling inference using Degirum models
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

#include <stdio.h>
#include <stdlib.h>
#include <memory>

// OpenCV
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

// Degirum
#include "client/dg_client.h"
#include "dg_file_utilities.h"
#include "dg_model_api.h"
#include "dgaccelerator_lib.h"
#include "gstdgaccelerator.h"
#include "json.hpp"

/// \brief long double json
using json_ld = nlohmann::basic_json< std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, long double >;

int NUM_INPUT_STREAMS;  //!< Number of input streams
int RING_BUFFER_SIZE;   //!< Size of circular queue of output objects
int FRAME_DIFF_LIMIT;   //!< Maximum number of frames waiting to be processed
#define DEFAULT_EAGER_BATCH_SIZE          8                                          //!< Default eager batch size
#define DEFAULT_INPUT_RAW_DATA_TYPE       "DG_UINT8"                                 //!< Default input raw data type
#define DEFAULT_OUTPUT_POSTPROCESS_TYPE   "None"                                     //!< Default output postprocess type
#define DEFAULT_OUTPUT_CONF_THRESHOLD     0.1                                        //!< Default output confidence threshold
#define DEFAULT_OUTPUT_NMS_THRESHOLD      0.6                                        //!< Default output NMS threshold
#define DEFAULT_OUTPUT_TOP_K              0                                          //!< Default output top K
#define DEFAULT_MAX_DETECTIONS            20                                         //!< Default maximum detections
#define DEFAULT_MAX_DETECTIONS_PER_CLASS  100                                        //!< Default maximum detections per class
#define DEFAULT_MAX_CLASSES_PER_DETECTION 30                                         //!< Default maximum classes per detection
#define DEFAULT_USE_REGULAR_NMS           true                                       //!< Default use regular NMS

// parseOutput function declaration
void parseOutput( const json &response, const unsigned int &index, std::vector< DgAcceleratorOutput * > out, DgAcceleratorCtx *ctx );

/// \brief Enum to hold the types of models
enum ModelType
{
	SEGMENTATION,     //!< Segmentation model type
	OBJ_DETECTION,    //!< Object detection model type
	POSE_ESTIMATION,  //!< Pose estimation model type
	CLASSIFICATION,   //!< Classification model type
	ERROR             //!< Error model type
};

/// \brief ModelType from json function
ModelType determineModelType( const json &response )
{
	if( !response.is_array() )
		return ERROR;
	if( response[ 0 ].contains( "data" ) && response[ 0 ].contains( "size" ) && response[ 0 ].contains( "shape" ) )
		return SEGMENTATION;
	if( response[ 0 ].contains( "landmarks" ) && response[ 0 ].contains( "score" ) )
		return POSE_ESTIMATION;
	if( response[ 0 ].contains( "bbox" ) && response[ 0 ].contains( "label" ) )
		return OBJ_DETECTION;
	return CLASSIFICATION;
}

/// \brief Context for the element, holds parameters for the model and a smart pointer to the model
struct DgAcceleratorCtx
{
	bool drop_frames;                           //!< Toggle for dropping frames
	gint processing_width;                      //!< Processing width of the model
	gint processing_height;                     //!< Processing height of the model
	std::unique_ptr< DG::AIModelAsync > model;  //!< Smart pointer to the model
	size_t diff = 0;                            //!< Counter for the number of frames waiting for callback at any given moment
	size_t framesProcessed = 0;                 //!< Frame count for FPS calculation.
	unsigned int curIndex;                      //!< Circular buffer index implementation
	std::chrono::time_point< std::chrono::high_resolution_clock > start_time;  //!< Clock for counting total duration
	std::vector< DgAcceleratorOutput * > out;  //!< Vector of pointers to output structs for circular buffer implementation
	// Error handling
	bool failed = false;     //!< Flag indicating if an error occurred
	std::string failReason;  //!< Reason for failure
};

///
/// \brief Initializes the DgAccelerator model with the given parameters and sets the callback function
///
/// This function initializes the DgAccelerator model with the given parameters using GstDgAccelerator struct.
/// It also sets the callback function for asynchronous operation of inference. The function returns a pointer to the
/// DgAcceleratorCtx instance.
///
/// \param[in] dgaccelerator Pointer to the GstDgAccelerator element for model initialization
/// \return Returns a pointer to the DgAcceleratorCtx instance
///
DgAcceleratorCtx *DgAcceleratorCtxInit( GstDgAccelerator *dgaccelerator )
{
	DgAcceleratorCtx *ctx = (DgAcceleratorCtx *)calloc( 1, sizeof( DgAcceleratorCtx ) );
	ctx->drop_frames = dgaccelerator->drop_frames;
	ctx->processing_width = dgaccelerator->processing_width;
	ctx->processing_height = dgaccelerator->processing_height;
	// Initialize number of input streams
	NUM_INPUT_STREAMS = dgaccelerator->batch_size;
	// Set the ring buffer size
	RING_BUFFER_SIZE = 2 * NUM_INPUT_STREAMS;  // 2 * the number of input streams
	// Set the ceiling for frame skipping
	FRAME_DIFF_LIMIT = std::max( 3, RING_BUFFER_SIZE - 1 );

	// Initialize the vector of output objects
	ctx->out.resize( RING_BUFFER_SIZE );
	for( auto &elem : ctx->out )
	{
		elem = (DgAcceleratorOutput *)calloc( 1, sizeof( DgAcceleratorOutput ) );
	}
	// Initialize curIndex
	ctx->curIndex = 0;

	const std::string serverIP = dgaccelerator->server_ip;
	std::string modelNameStr = dgaccelerator->model_name;
	std::cout << "\n\nINITIALIZING MODEL with IP ";
	std::cout << serverIP << " and name ";
	std::cout << dgaccelerator->model_name << "\n";


	DG::ModelParamsWriter mparams;  // Model Parameters writer to pass to the model

	// Sets the model parameters for each parameter set in model_params
	// set the eager batch size property
	if (dgaccelerator->model_params.eager_batch_size != DEFAULT_EAGER_BATCH_SIZE)
		mparams.EagerBatchSize_set(dgaccelerator->model_params.eager_batch_size);

	// set the input raw data type property
	if (strcmp(dgaccelerator->model_params.input_raw_data_type, DEFAULT_INPUT_RAW_DATA_TYPE) != 0)
		mparams.InputRawDataType_set(dgaccelerator->model_params.input_raw_data_type);

	// set the output postprocess type property
	if (strcmp(dgaccelerator->model_params.output_postprocess_type, DEFAULT_OUTPUT_POSTPROCESS_TYPE) != 0)
		mparams.OutputPostprocessType_set(dgaccelerator->model_params.output_postprocess_type);

	// set the output confidence threshold property
	if (dgaccelerator->model_params.output_conf_threshold != DEFAULT_OUTPUT_CONF_THRESHOLD)
		mparams.OutputConfThreshold_set(dgaccelerator->model_params.output_conf_threshold);

	// set the output NMS threshold property
	if (dgaccelerator->model_params.output_nms_threshold != DEFAULT_OUTPUT_NMS_THRESHOLD)
		mparams.OutputNMSThreshold_set(dgaccelerator->model_params.output_nms_threshold);

	// set the output top K property
	if (dgaccelerator->model_params.output_top_k != DEFAULT_OUTPUT_TOP_K)
		mparams.OutputTopK_set(dgaccelerator->model_params.output_top_k);

	// set the max detections property
	if (dgaccelerator->model_params.max_detections != DEFAULT_MAX_DETECTIONS)
		mparams.MaxDetections_set(dgaccelerator->model_params.max_detections);

	// set the max detections per class property
	if (dgaccelerator->model_params.max_detections_per_class != DEFAULT_MAX_DETECTIONS_PER_CLASS)
		mparams.MaxDetectionsPerClass_set(dgaccelerator->model_params.max_detections_per_class);

	// set the max classes per detection property
	if (dgaccelerator->model_params.max_classes_per_detection != DEFAULT_MAX_CLASSES_PER_DETECTION)
		mparams.MaxClassesPerDetection_set(dgaccelerator->model_params.max_classes_per_detection);

	// set the use regular NMS property
	if (dgaccelerator->model_params.use_regular_nms != DEFAULT_USE_REGULAR_NMS)
		mparams.UseRegularNMS_set(dgaccelerator->model_params.use_regular_nms);



	if( modelNameStr.find( '/' ) == std::string::npos )  // Check if requesting a local model
	{                                                    // Validate model name:
		std::vector< DG::ModelInfo > modelList;
		DG::modelzooListGet( serverIP, modelList );
		auto model_id = DG::modelFind( serverIP, { modelNameStr } );
		if( model_id.name.empty() )
		{
			std::cout << "Model '" + modelNameStr + "' is not found in model zoo";
			std::cout << "\nAvailable models:\n\n";
			for( auto m : modelList )
				std::cout << m.name << ", WxH: " << m.W << "x" << m.H << "\n";
			throw std::runtime_error( "Model '" + modelNameStr + "' is not found in model zoo" );
		}
		// Validate model width/height:
		if( dgaccelerator->processing_height != model_id.H )
		{
			throw std::runtime_error( "Property processing-height does not match model." );
			return nullptr;
		}
		if( dgaccelerator->processing_width != model_id.W )
		{
			throw std::runtime_error( "Property processing-width does not match model." );
			return nullptr;
		}
	}
	else  // Cloud model requested, set the token in model params
	{     // Can't validate cloud model name or cloud token. Happens in PLAYING state
		// Instead we at least can check if cloud token is missing
		if( strlen( dgaccelerator->cloud_token ) == 0 )
		{
			throw std::runtime_error( "No cloud token provided for the chosen cloud model." );
			return nullptr;
		}
		else
		{
			mparams.CloudToken_set( dgaccelerator->cloud_token );
		}
		// Validation of cloud model existence and width/height match happens in PLAYING state.
	}
	// Callback function for parsing the model inference data for a frame
	auto callback = [ ctx ]( const json &response, const std::string &fr ) {
		unsigned int index = std::stoi( fr );  // Index of the Output struct to fill

		// Deallocate the output struct prior to working on it:
		// Deallocate memory for Pose Estimation
		for( int i = 0; i < ctx->out[ index ]->numPoses; i++ )
		{
			ctx->out[ index ]->pose[ i ].landmarks.clear();  // Deallocate memory for vector of landmarks
		}
		// Deallocate memory for Segmentation
		ctx->out[ index ]->segMap.class_map.clear();  // Deallocate memory for vector of class_map
		// Reset values to 0
		ctx->out[ index ]->numObjects = 0;
		ctx->out[ index ]->numPoses = 0;
		ctx->out[ index ]->k = 0;
		ctx->out[ index ]->segMap.mask_width = 0;
		ctx->out[ index ]->segMap.mask_height = 0;

		// Check for errors during inference
		std::string possible_error = DG::errorCheck( response );
		if( !possible_error.empty() )
		{
			ctx->failed = true;
			ctx->failReason = possible_error;
			goto fail;
		}
		// Parse the json output, fill output structure using processed output
		parseOutput( response, index, ctx->out, ctx );
	fail:
		ctx->framesProcessed++;
		ctx->diff--;  // Decrement # of frames waiting to be processed
	};

	// Initialize the model with the parameters. Internal frame queue size set to 48
	ctx->model = std::make_unique< DG::AIModelAsync >( serverIP, modelNameStr, callback, mparams, 48u );
	// runtime error will happen if invalid modelname or server ip is set.

	std::cout << "\nMODEL SUCCESSFULLY INITIALIZED\n\n";

	// Start the clock for counting total duration
	ctx->start_time = std::chrono::high_resolution_clock::now();

	return ctx;
}
///
/// \brief Parses the output of the DgAccelerator model and fills in a DgAcceleratorOutput instance
///
/// This function parses the output of the DgAccelerator model, which is expected to be a JSON array containing
/// information about inference results. It populates a DgAcceleratorOutput instance with the
/// inference results. The function is called once for each frame.
///
/// \param[in] response The JSON response from the model
/// \param[in] index The index of the output instance to populate
/// \param[in] out A vector of pointers to DgAcceleratorOutput instances to populate
/// \param[in] ctx A pointer to the DgAcceleratorCtx instance
///
/// \return void
///
void parseOutput( const json &response, const unsigned int &index, std::vector< DgAcceleratorOutput * > out, DgAcceleratorCtx *ctx )
{
	if( response.empty() )
		return;  // empty frame: no inference results

	// Check for which model type we are using, based on the json.
	ModelType type = determineModelType( response );
	if( type == POSE_ESTIMATION )
	{
		int numPoses = 0;
		for( const nlohmann::json &pose : response )
		{
			if( !pose.contains( "landmarks" ) || !pose.contains( "score" ) )
				continue;

			if( numPoses >= MAX_OBJ_PER_FRAME )
				break;

			// Iterate over all landmarks in the JSON
			for( const nlohmann::json &landmark : pose[ "landmarks" ] )
			{
				DgAcceleratorPose::Landmark lm;
				lm.landmark_class = landmark[ "category_id" ].get< int >();
				strncpy( lm.label, landmark[ "label" ].get< std::string >().c_str(), DG_MAX_LABEL_SIZE - 1 );
				lm.label[ DG_MAX_LABEL_SIZE - 1 ] = '\0';
				lm.connection = landmark[ "connect" ].get< std::vector< int > >();
				std::vector< double > point = landmark[ "landmark" ].get< std::vector< double > >();
				lm.point = { point[ 0 ], point[ 1 ] };
				ctx->out[ index ]->pose[ numPoses ].landmarks.push_back( lm );
			}
			numPoses++;
		}
		ctx->out[ index ]->numPoses = numPoses;
	}
	else if( type == OBJ_DETECTION )
	{
		// Iterate over all of the detected objects
		for( int i = 0; i < response.size(); i++ )
		{
			if( ctx->out[ index ]->numObjects >= MAX_OBJ_PER_FRAME )
				break;
			ctx->out[ index ]->numObjects++;
			json_ld newresp = response[ i ];  // Output from model is a json array, so convert to single element
			std::vector< long double > bbox = newresp[ "bbox" ].get< std::vector< long double > >();
			std::string label = newresp[ "label" ];
			int category_id = newresp[ "category_id" ].get< int >();
			long double score = newresp[ "score" ].get< long double >();
			ctx->out[ index ]->object[ i ] = ( DgAcceleratorObject ){
				std::roundf( bbox[ 0 ] ),                                               // left
				std::roundf( bbox[ 1 ] ),                                               // top
				std::roundf( bbox[ 2 ] - bbox[ 0 ] ),                                   // width
				std::roundf( bbox[ 3 ] - bbox[ 1 ] ),                                   // height
				""                                                                      // label, must be of type char[]
			};
			snprintf( ctx->out[ index ]->object[ i ].label, 64, "%s", label.c_str() );  // Sets the label
		}
	}
	else if( type == CLASSIFICATION )
	{
		for( const nlohmann::json &object : response )
		{
			if( ctx->out[ index ]->k >= MAX_OBJ_PER_FRAME )
				break;
			if( !object.contains( "label" ) )
				continue;
			std::string label = object[ "label" ];
			double score = object[ "score" ].get< double >();
			ctx->out[ index ]->classifiedObject[ ctx->out[ index ]->k ] = ( DgAcceleratorClassObject ){
				score,
				""                                                                                                   // label, must be of type char[]
			};
			snprintf( ctx->out[ index ]->classifiedObject[ ctx->out[ index ]->k ].label, 64, "%s", label.c_str() );  // Sets the label
			ctx->out[ index ]->k++;
		}
	}
	else if( type == SEGMENTATION )
	{
		// If metadata does not contain needed result, skip
		if( !response[ 0 ].contains( "data" ) || !response[ 0 ].contains( "size" ) || !response[ 0 ].contains( "shape" ) )
			return;

		// Obtain mask height/width from the json
		size_t mask_width = response[ 0 ][ "shape" ].get< std::vector< int > >()[ 1 ];
		size_t mask_height = response[ 0 ][ "shape" ].get< std::vector< int > >()[ 2 ];
		// Now parse the json into an int array mask
		const auto &byte_vector = response[ 0 ][ "data" ].get_binary();

		ctx->out[ index ]->segMap.mask_width = mask_width;
		ctx->out[ index ]->segMap.mask_height = mask_height;
		ctx->out[ index ]->segMap.class_map.resize( mask_width * mask_height );
		std::copy( byte_vector.begin(), byte_vector.end(), ctx->out[ index ]->segMap.class_map.begin() );
	}
	else if( type == ERROR || strcmp( response.type_name(), "object" ) == 0 )
	{  // Model gave a bad result not caught by errorcheck
		ctx->failed = true;
		ctx->failReason = response.dump();
	}
}

///
/// \brief Main process function for the DgAccelerator model
///
/// This function is the main processing function for the DgAccelerator model. It converts the input data to a cv::Mat
/// and passes the JPEG information to the model. The function is called for each frame and outputs objects in a
/// DgAcceleratorOutput instance.
///
/// \param[in] ctx Pointer to the DgAcceleratorCtx instance
/// \param[in] data Pointer to the input data as a OpenCV mat
/// \return Returns a pointer to the DgAcceleratorOutput instance
///
DgAcceleratorOutput *DgAcceleratorProcess( DgAcceleratorCtx *ctx, unsigned char *data )
{
	ctx->diff++;  // Increment # of frames waiting to be processed
	// Immediately need to add to curIndex so that the circular buffer can keep going
	// Wrap around RING_BUFFER_SIZE for circular buffer implementation
	ctx->curIndex %= RING_BUFFER_SIZE;
	int curFrameIndex = ctx->curIndex++;

	// If an error happens during inference (such as runtime model parameter validation)
	if( ctx->failed )
	{
		throw std::runtime_error( ctx->failReason );
	}

	// Frame skip implementation:
	if( ctx->drop_frames )
	{
		if( ctx->diff > FRAME_DIFF_LIMIT )  // if FRAME_DIFF_LIMIT frames behind
			goto skip;
	}

	if( data != NULL )  // Data is a pointer to a cv::Mat.
	{
		// Extract the mat
		cv::Mat frameMat( ctx->processing_height, ctx->processing_width, CV_8UC3, data );
		// encode this mat into a jpeg buffer vector.
		std::vector< int > param = { cv::IMWRITE_JPEG_QUALITY, 85 };
		std::vector< unsigned char > ubuff = {};
		// Compress the image and store it in the memory buffer that is resized to fit the result.
		cv::imencode( ".jpeg", frameMat, ubuff, param );
		// Pass to the model.
		std::vector< std::vector< char > > frameVect{ std::vector< char >( ubuff.begin(), ubuff.end() ) };
		// This passes the data buffer and the current frame output object index to work on
		ctx->model->predict( frameVect, std::to_string( curFrameIndex ) );  // Call the predict function
		frameMat.release();
	}
	return ctx->out[ curFrameIndex ];

skip:
	// Reach here if the model can't keep up with all the incoming frames
	std::cout << "Skipping frame due to diff of " << ctx->diff << "\n";
	std::cout << "If this happens too often, lower the incoming framerate of streams and/or the number of streams!\n";
	ctx->diff--;
	// Return an empty frame instead
	return (DgAcceleratorOutput *)calloc( 1, sizeof( DgAcceleratorOutput ) );
}

///
/// \brief Deinitializes the DgAccelerator model
///
/// This function deinitializes the DgAccelerator model by processing all outstanding frames and resetting the model.
/// It also frees the output objects.
///
/// \param[in] ctx Pointer to the DgAcceleratorCtx instance
///
void DgAcceleratorCtxDeinit( DgAcceleratorCtx *ctx )
{
	std::cout << "\nDeinitializing model, processing " << ctx->diff << " outstanding frames...\n\n\n";
	// Process all outstanding frames:
	ctx->model->waitCompletion();

	// Calculate FPS
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( end_time - ctx->start_time );
	std::cout << "Frames processed / duration (FPS) :" << 1000 * ( (long double)ctx->framesProcessed / duration.count() ) << "\n";

	ctx->framesProcessed = 0;
	ctx->diff = 0;

	// Reset our model
	ctx->model.reset();
	free( ctx );
	// Free output objects
	for( auto &elem : ctx->out )
	{
		delete elem;
		elem = nullptr;
	}
}