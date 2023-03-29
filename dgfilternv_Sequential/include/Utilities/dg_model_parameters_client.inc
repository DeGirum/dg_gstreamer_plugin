//////////////////////////////////////////////////////////////////////
/// \file  dg_model_parameters_client.inc
/// \brief List of Json model parameters, visible to client side.
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declaration of Json model parameters, visible to client side.
/// This file is included into dg_model_parameters.h for client-side builds.
/// 


///
/// List of all configuration parameters
/// Format:
///	_( name,						section,				C-type,						default,		req'd,	runtime,visible, fallback )
///
#define DG_MODEL_PARAMS_LIST_CLIENT \
	_( ConfigVersion,				SECT_TOP,				int,						0,				true,	false,	false,	None	) \
	_( MeasureTime,					SECT_TOP,				bool,						false,			false,	true,	true,	None	) \
	_( CloudToken,					SECT_TOP,				std::string,				"",				false,	true,	false,	None	) \
	\
	_( InputImgFmt,					SECT_PRE_PROCESS,		std::string,				"JPEG",			false,	true,	false,	None	) \
	_( InputImgRawDataType,			SECT_PRE_PROCESS,		std::string,				"DG_UINT8",		false,	true,	false,	None	) \
	_( InputRawDataType,			SECT_PRE_PROCESS,		std::string,				"DG_UINT8",		false,	true,	true,	InputImgRawDataType	) \
	_( InputImgRotation,			SECT_PRE_PROCESS,		int,						0,				false,	true,	false,	None	) \
	_( InputColorSpace,				SECT_PRE_PROCESS,		std::string,				"RGB",			false,	true,	true,	None	) \
	_( InputResizeMethod,			SECT_PRE_PROCESS,		std::string,				"bilinear",		false,	false,	true,	None	) \
	_( InputPadMethod,				SECT_PRE_PROCESS,		std::string,				"letterbox",	false,	false,	true,	None	) \
	_( InputCropPercentage,			SECT_PRE_PROCESS,		double,						1.0,			false,	false,	true,	None	) \
	_( ImageBackend,				SECT_PRE_PROCESS,		std::string,				"auto",			false,	false,	true,	None	) \
	\
	_( OutputPostprocessType,		SECT_POST_PROCESS,		std::string,				"None",			false,	true,	true,	None	) \
	_( OutputConfThreshold,			SECT_POST_PROCESS,		double,						0.1,			false,	true,	true,	None	) \
	_( OutputNMSThreshold,			SECT_POST_PROCESS,		double,						0.6,			false,	true,	true,	None	) \
	_( OutputTopK,					SECT_POST_PROCESS,		size_t,						0,				false,	true,	true,	None	) \
	_( MaxDetections,				SECT_POST_PROCESS,		int,						20,				false,	true,	true,	None	) \
	_( MaxDetectionsPerClass,		SECT_POST_PROCESS,		int,						100,			false,	true,	true,	None	) \
	_( MaxClassesPerDetection,		SECT_POST_PROCESS,		int,						30,				false,	true,	true,	None	) \
	_( UseRegularNMS,				SECT_POST_PROCESS,		bool,						true,			false,	true,	true,	None	)
