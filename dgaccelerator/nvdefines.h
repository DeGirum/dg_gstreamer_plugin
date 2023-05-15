//////////////////////////////////////////////////////////////////////
///  \file  nvdefines.h
///  \brief helper function definitions, provided by NVIDIA
///
///  Copyright (c) 2017-2021, NVIDIA CORPORATION.  All rights reserved.
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
///


#define CHECK_NVDS_MEMORY_AND_GPUID( object, surface )                                                                                           \
	( {                                                                                                                                          \
		int _errtype = 0;                                                                                                                        \
		do                                                                                                                                       \
		{                                                                                                                                        \
			if( ( surface->memType == NVBUF_MEM_DEFAULT || surface->memType == NVBUF_MEM_CUDA_DEVICE ) && ( surface->gpuId != object->gpu_id ) ) \
			{                                                                                                                                    \
				GST_ELEMENT_ERROR(                                                                                                               \
					object,                                                                                                                      \
					RESOURCE,                                                                                                                    \
					FAILED,                                                                                                                      \
					( "Input surface gpu-id doesnt match with configured gpu-id for element,"                                                    \
					  " please allocate input using unified memory, or use same gpu-ids" ),                                                      \
					( "surface-gpu-id=%d,%s-gpu-id=%d", surface->gpuId, GST_ELEMENT_NAME( object ), object->gpu_id ) );                          \
				_errtype = 1;                                                                                                                    \
			}                                                                                                                                    \
		} while( 0 );                                                                                                                            \
		_errtype;                                                                                                                                \
	} ) //!< Check memory and gpu prior to using CUDA - Nvidia 
#define CHECK_NPP_STATUS( npp_status, error_str )                                                               \
	do                                                                                                          \
	{                                                                                                           \
		if( ( npp_status ) != NPP_SUCCESS )                                                                     \
		{                                                                                                       \
			g_print( "Error: %s in %s at line %d: NPP Error %d\n", error_str, __FILE__, __LINE__, npp_status ); \
			goto error;                                                                                         \
		}                                                                                                       \
	} while( 0 ) //!< Check NPP Status - Nvidia 

#define CHECK_CUDA_STATUS( cuda_status, error_str )                                                                         \
	do                                                                                                                      \
	{                                                                                                                       \
		if( ( cuda_status ) != cudaSuccess )                                                                                \
		{                                                                                                                   \
			g_print( "Error: %s in %s at line %d (%s)\n", error_str, __FILE__, __LINE__, cudaGetErrorName( cuda_status ) ); \
			goto error;                                                                                                     \
		}                                                                                                                   \
	} while( 0 ) //!< Check CUDA - Nvidia 

	