//////////////////////////////////////////////////////////////////////
///  \file  dgaccelerator_lib.h
///  \brief DgAccelerator context class header file
///
///  Copyright (c) 2017-2020, NVIDIA CORPORATION.  All rights reserved.
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
///  This software contains source code provided by NVIDIA Corporation.

#ifndef __DGACCELERATOR_LIB__
#define __DGACCELERATOR_LIB__

#define MAX_LABEL_SIZE    128
#define MAX_OBJ_PER_FRAME 35
#ifdef __cplusplus
extern "C"
{
#endif

	typedef struct DgAcceleratorCtx DgAcceleratorCtx;

	// Init parameters structure as input, required for instantiating dgaccelerator_lib
	typedef struct
	{
		// Width at which frame/object will be scaled
		int processingWidth;
		// height at which frame/object will be scaled
		int processingHeight;
		// model name
		char model_name[ MAX_LABEL_SIZE ];
		// server ip
		char server_ip[ MAX_LABEL_SIZE ];
		// Number of input streams
		int numInputStreams;
		// cloud token
		char cloud_token[ MAX_LABEL_SIZE ];
		// drop frames toggle
		bool drop_frames;
	} DgAcceleratorInitParams;

	// Detected/Labelled object structure, stores bounding box info along with label
	typedef struct
	{
		float left;
		float top;
		float width;
		float height;
		char label[ MAX_LABEL_SIZE ];
	} DgAcceleratorObject;

	// Output data returned after processing
	typedef struct
	{
		int numObjects;
		DgAcceleratorObject object[ MAX_OBJ_PER_FRAME ];  // Allocates room for MAX_OBJ_PER_FRAME objects
	} DgAcceleratorOutput;

	// Initialize library
	DgAcceleratorCtx *DgAcceleratorCtxInit( DgAcceleratorInitParams *init_params );

	// Process output
	DgAcceleratorOutput *DgAcceleratorProcess( DgAcceleratorCtx *ctx, unsigned char *data );

	// Deinitialize library context
	void DgAcceleratorCtxDeinit( DgAcceleratorCtx *ctx );

#ifdef __cplusplus
}
#endif

#endif
