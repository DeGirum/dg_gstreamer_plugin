/**
 * Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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
 */

#include "dgaccelerator_lib.h"
#include <stdio.h>
#include <stdlib.h>

#include <memory>
#include "dg_model_api.h"
#include "dg_file_utilities.h"
#include "dg_tracing_facility.h"
#include "dg_client.h"
#include "json.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

DG_TRC_GROUP_DEF( DgAcceleratorLib )

using json_ld = nlohmann::basic_json< std::map, std::vector, std::string, bool, std::int64_t, std::uint64_t, long double >;

int NUM_INPUT_STREAMS;                     // Number of input streams
int RING_BUFFER_SIZE;                      // Size of circular queue of output objects
int FRAME_DIFF_LIMIT;                      // Maximum number of frames waiting to be processed
std::vector< DgAcceleratorOutput * > out;  // Vector of pointers to output structs, for circular buffer implementation
unsigned int curIndex;                     // circular buffer index implementation

size_t diff = 0;             // Counter for # of frames waiting for callback at any given moment
size_t framesProcessed = 0;  // Frame count for FPS calculation, careful with uint overflow...

bool failed = false;  // Model error message handling
std::string failReason;

// Clock for counting total duration
std::chrono::time_point< std::chrono::high_resolution_clock > start_time;

// Context for the plugin, holds initParams for the model
// and a smart pointer to the model
struct DgAcceleratorCtx
{
	DgAcceleratorInitParams initParams;
	std::unique_ptr< DG::AIModelAsync > model;
};

// Initializes the model with the given initParams.
// Sets the callback function for asynchronous operation of inference.
DgAcceleratorCtx *DgAcceleratorCtxInit( DgAcceleratorInitParams *initParams )
{
	DgAcceleratorCtx *ctx = (DgAcceleratorCtx *)calloc( 1, sizeof( DgAcceleratorCtx ) );
	ctx->initParams = *initParams;
	// Initialize number of input streams
	NUM_INPUT_STREAMS = initParams->numInputStreams;
	// Set the ring buffer size
	RING_BUFFER_SIZE = 2 * NUM_INPUT_STREAMS;  // 2x the number of input streams works best..
	// Set the ceiling for frame skipping
	FRAME_DIFF_LIMIT = RING_BUFFER_SIZE - 1;

	// Initialize the vector of output objects.
	out.resize( RING_BUFFER_SIZE );
	for( auto &elem : out )
	{
		elem = (DgAcceleratorOutput *)calloc( 1, sizeof( DgAcceleratorOutput ) );
	}
	// Initialize curIndex
	curIndex = 0;

	const std::string serverIP = ctx->initParams.server_ip;
	std::string modelNameStr = ctx->initParams.model_name;
	std::cout << "\n\nINITIALIZING MODEL with IP ";
	std::cout << serverIP << " and name ";
	std::cout << ctx->initParams.model_name << "\n";

	DG::ModelParamsWriter mparams;  // Model Parameters writer to pass to the model

	// Validate model name here:
	if( modelNameStr.find( '/' ) == std::string::npos )  // Check if requesting a local model
	{
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
	}
	else  // Cloud model requested, set the token in model params
	{
		mparams.CloudToken_set( initParams->cloud_token );
	}

	// Callback function for parsing the model inference data for a frame
	auto callback = [ ctx ]( const json &response, const std::string &fr ) {
		DG_TRC_BLOCK( DgAcceleratorLib, callback, DGTrace::lvlBasic );

		unsigned int index = std::stoi( fr );  // Index of the Output struct to fill
		// Reset the output struct:
		std::memset( out[ index ], 0, sizeof( DgAcceleratorObject ) );

		// Parse the json output, fill output structure using processed output
		json_ld resp = response;
		if( strcmp( resp.type_name(), "array" ) == 0 && response.dump() != "[]" )
		{
			// Iterate over all of the detected objects
			for( int i = 0; i < resp.size(); i++ )
			{
				out[ index ]->numObjects++;
				json_ld newresp = resp[ i ];  // Output from model is a json array, so convert to single element
				std::vector< long double > bbox = newresp[ "bbox" ].get< std::vector< long double > >();
				std::string label = newresp[ "label" ];
				int category_id = newresp[ "category_id" ].get< int >();
				long double score = newresp[ "score" ].get< long double >();
				// std::cout << label << " " << category_id << " " << score;
				// std::cout << " " << bbox[0]<< " "<< bbox[1]<< " "<< bbox[2]<< " "<< bbox[3] << "\n";
				out[ index ]->object[ i ] = ( DgAcceleratorObject ){
					std::roundf( bbox[ 0 ] ),              // left
					std::roundf( bbox[ 1 ] ),              // top
					std::roundf( bbox[ 2 ] - bbox[ 0 ] ),  // width
					std::roundf( bbox[ 3 ] - bbox[ 1 ] ),  // height
					""                                     // label, must be of type char[]
				};
				snprintf( out[ index ]->object[ i ].label, 64, "%s", label.c_str() );  // Sets the label
			}
		}
		else if( strcmp( resp.type_name(), "object" ) == 0 )
		{  // Model gave a bad result
			failed = true;
			failReason = response.dump();
		}
		// Now, all of the detected objects in the frame are inside the out struct
		// std::cout << "Finished work on object with frame index: " << index << "\n";

		framesProcessed++;
		diff--;  // Decrement # of frames waiting to be processed
	};

	// Initialize the model with the parameters
	ctx->model.reset( new DG::AIModelAsync( serverIP, modelNameStr, callback, mparams, 48u ) );

	std::cout << "\nMODEL SUCCESSFULLY INITIALIZED\n\n";

	// Start the clock for counting total duration
	start_time = std::chrono::high_resolution_clock::now();

	return ctx;
}

// Main process function. Converts input to a cv::Mat and passes jpeg info to the model.
// Gets called for each frame, outputs objects in a DgAcceleratorOutput
DgAcceleratorOutput *DgAcceleratorProcess( DgAcceleratorCtx *ctx, unsigned char *data )
{
	DG_TRC_BLOCK( DgAcceleratorLib, DgAcceleratorProcess, DGTrace::lvlBasic );

	diff++;  // Increment # of frames waiting to be processed

	// Immediately need to add to curIndex so that the circular buffer can keep going
	// Wrap around RING_BUFFER_SIZE for circular buffer implementation
	curIndex %= RING_BUFFER_SIZE;
	int curFrameIndex = curIndex++;

	if( failed )
		throw std::runtime_error( "Model gave bad result: " + failReason );

	// Frame skip implementation:
	if( ctx->initParams.drop_frames )
	{
		if( diff > FRAME_DIFF_LIMIT )  // if FRAME_DIFF_LIMIT frames behind
			goto skip;
	}

	if( data != NULL )  // Data is a pointer to a cv::Mat.
	{
		// Extract the mat
		cv::Mat frameMat( ctx->initParams.processingHeight, ctx->initParams.processingWidth, CV_8UC3, data );
		// (Usually is square) We can now pass it to the AI Model.
		// encode this mat into a jpeg buffer vector.
		std::vector< int > param = { cv::IMWRITE_JPEG_QUALITY, 85 };
		std::vector< unsigned char > ubuff = {};
		// The function imencode compresses the image and stores it in the memory buffer that is resized to fit the result.
		cv::imencode( ".jpeg", frameMat, ubuff, param );
		// Pass to the model.
		std::vector< std::vector< char > > frameVect{ std::vector< char >( ubuff.begin(), ubuff.end() ) };
		ctx->model->predict( frameVect, std::to_string( curFrameIndex ) );  // Call the predict function
		// This passes the data buffer and the current frame output object index to work on
		frameMat.release();
	}
	// std::cout << "Returning object with current frame index: " << curFrameIndex << "\n";
	return out[ curFrameIndex ];

skip:
	// Reach here if the model can't keep up with all the incoming frames
	DG_TRC_POINT( DgAcceleratorLib, ProcessSkip, DGTrace::lvlBasic );
	std::cout << "Skipping frame due to diff of " << diff << "\n";
	std::cout << "If this happens too often, lower the incoming framerate of streams and/or the number of streams!\n";
	diff--;
	// Return an empty frame instead
	return (DgAcceleratorOutput *)calloc( 1, sizeof( DgAcceleratorOutput ) );
}

// Deinitialize function, called when element stops processing
void DgAcceleratorCtxDeinit( DgAcceleratorCtx *ctx )
{
	std::cout << "\nDeinitializing model, processing " << diff << " outstanding frames...\n\n\n";
	// Process all outstanding frames:
	ctx->model->waitCompletion();

	// Calculate FPS
	auto end_time = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast< std::chrono::milliseconds >( end_time - start_time );
	std::cout << "Frames processed / duration (FPS) :" << 1000 * ( (long double)framesProcessed / duration.count() );

	// Reset our model
	ctx->model.reset();
	free( ctx );
	// Free output objects
	for( auto &elem : out )
	{
		delete elem;
		elem = nullptr;
	}
}