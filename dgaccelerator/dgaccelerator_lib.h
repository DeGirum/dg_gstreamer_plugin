//////////////////////////////////////////////////////////////////////
///  \file  dgaccelerator_lib.h
///  \brief DgAccelerator context class header file
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

#ifndef __DGACCELERATOR_LIB__
#define __DGACCELERATOR_LIB__

#include <string>

constexpr int DG_MAX_LABEL_SIZE = 128;  //!< Max string size to allocate
constexpr int MAX_OBJ_PER_FRAME = 35;   //!< Max objects to draw per frame

class DgAcceleratorCtx;

/// \brief Init parameters structure as input, required for instantiating dgaccelerator_lib
struct DgAcceleratorInitParams
{
	int processingWidth;                    //!< Width of the model
	int processingHeight;                   //!< Height of the model
	char model_name[ DG_MAX_LABEL_SIZE ];   //!< Model name
	char server_ip[ DG_MAX_LABEL_SIZE ];    //!< Server IP
	int numInputStreams;                    //!< Number of input streams
	char cloud_token[ DG_MAX_LABEL_SIZE ];  //!< Cloud token
	bool drop_frames;                       //!< Drop frames toggle
};

/// \brief Result from Object Detection Model
struct DgAcceleratorObject
{
	float left;                       //!< x coordinate of the bounding box
	float top;                        //!< y coordinate of the bounding box
	float width;                      //!< Width of the bounding box
	float height;                     //!< Height of the bounding box
	char label[ DG_MAX_LABEL_SIZE ];  //!< Label assigned to the detected object
};

/// \brief Result from Pose Estimation Model
struct DgAcceleratorPose
{
	/// \brief A Landmark. Inference produces a set of these, together they define the pose
	struct Landmark
	{
		std::pair< double, double > point;  //!< Coordinate of the landmark. [0] = x, [1] = y
		std::vector< int > connection;      //!< Indices of landmarks this one connects to
		char label[ DG_MAX_LABEL_SIZE ];    //!< Label of this landmark
		int landmark_class;                 //!< Index of this landmark
	};
	std::vector< Landmark > landmarks;      //!< Vector of landmarks defining the pose
};

/// \brief Result from Classification Model
struct DgAcceleratorClassObject
{
	double score;                     //!< Probability of classified object
	char label[ DG_MAX_LABEL_SIZE ];  //!< Label for the object
};

/// \brief Result from Segmentation Model
struct DgAcceleratorSegmentation
{
	std::vector< int > class_map;  //!< Array for 2D pixel class map. Pixel (x,y) is at class_map[y*width+x]
	size_t mask_width;             //!< Width of the segmentation mask
	size_t mask_height;            //!< Height of the segmentation mask
};

/// \brief Output data for 1 frame returned after processing
struct DgAcceleratorOutput
{
	// Object Detection models:
	int numObjects;                                   //!< Number of detected objects
	DgAcceleratorObject object[ MAX_OBJ_PER_FRAME ];  //!< Object array. Allocates room for MAX_OBJ_PER_FRAME objects
	// Pose Estimation models:
	int numPoses;                                 //!< Number of detected poses
	DgAcceleratorPose pose[ MAX_OBJ_PER_FRAME ];  //!< Poses array. Allocates room for MAX_OBJ_PER_FRAME poses.
	// Classification models:
	int k;                                                           //!< Number of classified objects
	DgAcceleratorClassObject classifiedObject[ MAX_OBJ_PER_FRAME ];  //!< Classified object array. Allocates room for MAX_OBJ_PER_FRAME objects.
	// Segmentation Models:
	DgAcceleratorSegmentation segMap;  //!< Segmentation map for a frame
};

// Initialize library
DgAcceleratorCtx *DgAcceleratorCtxInit( DgAcceleratorInitParams *init_params );

// Process output
DgAcceleratorOutput *DgAcceleratorProcess( DgAcceleratorCtx *ctx, unsigned char *data );

// Deinitialize our library context
void DgAcceleratorCtxDeinit( DgAcceleratorCtx *ctx );

#endif
