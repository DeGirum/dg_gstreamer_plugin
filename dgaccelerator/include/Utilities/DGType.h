//////////////////////////////////////////////////////////////////////
/// \file  DGType.h
/// \brief DG data types support
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains scalar data type related declarations: type ID enum,
/// type width enum, information and conversion functions
/// 


//
// NOTE: this file should be compilable under BOTH C and C++
//

#ifndef DG_TYPE_H
#define DG_TYPE_H

#include <string.h>


/// Type bit width enumerator
typedef enum DGTypeBitWidth
{
	DG_8BIT, DG_16BIT, DG_32BIT, DG_64BIT, DG_UNDEFINED_BITS
}
	DGTypeBitWidth;

typedef float float32_t;	//!< alias for float
typedef double float64_t;	//!< alias for double

/// Supported types list
#define DG_TYPE_LIST \
	_( DG_FLT,			float32_t,	DG_32BIT ) \
	_( DG_UINT8,		uint8_t,	DG_8BIT ) \
	_( DG_INT8,			int8_t,		DG_8BIT ) \
	_( DG_UINT16,		uint16_t,	DG_16BIT ) \
	_( DG_INT16,		int16_t,	DG_16BIT ) \
	_( DG_INT32,		int32_t,	DG_32BIT ) \
	_( DG_INT64,		int64_t,	DG_64BIT ) \
	_( DG_DBL,			float64_t,	DG_64BIT ) \
	_( DG_UINT32,		uint32_t,	DG_32BIT ) \
	_( DG_UINT64,		uint64_t,	DG_64BIT) \
	

/// Type enumerator
typedef enum DGType
{
	#define _( id, type, width ) id,
	DG_TYPE_LIST
	#undef _
	DG_UNDEFINED
}
	DGType;

#ifdef __cplusplus
namespace DG
{
	namespace DGTypeStrings {
	#define _(X, ctype, bitw) static constexpr const char* s##X = #X;
		DG_TYPE_LIST
	#undef _
	}
#endif

	/// Return bit width of given type ID
	inline DGTypeBitWidth ConvertType2BitWidth( DGType type )
	{
		switch( type )
		{
		#define _( id, type, width ) case id: return width;
		DG_TYPE_LIST
		#undef _
		default: return DG_UNDEFINED_BITS;
		}
	}

	/// Derive computation type from log2 of byte width
	/// \param[in] log2_data_width - log2 of byte width of the type
	/// \return type ID of type, which satisfies given width
	inline DGType DeriveComputeType( size_t log2_data_width )
	{   
		switch( log2_data_width )
		{
		case 0:     return DG_UINT8;
		case 1:     return DG_UINT16;
		case 2:     return DG_FLT;
		default:    return DG_UNDEFINED;
		}
	}

	/// Derive accumulation type for given computation type
	/// \param[in] comp_type - computation type ID
	/// \return type ID of accumulation type, which satisfies given computation type
	inline DGType DeriveAccumType( DGType comp_type )
	{   
		switch( comp_type )
		{
			case DG_FLT:        return DG_FLT;
			case DG_UINT8:      return DG_INT32;
			default:            break;
		}
		return DG_UNDEFINED;
	}

	/// Return size in bytes for give bit width ID
	inline int SizeOfBW( DGTypeBitWidth type )
	{   
		switch( type )
		{
			case DG_8BIT:       return 1;
			case DG_16BIT:      return 2;
			case DG_32BIT:      return 4;
			case DG_64BIT:      return 8;
			default:            return -1;
		}		
	}

	/// Return size in bytes for type ID
	inline int SizeOf( DGType type )
	{   
		return SizeOfBW( ConvertType2BitWidth( type ) );
	}

	/// Derive type ID from type string representation
	inline DGType Sting2DGType( const char *sType )
	{
		#define _( id, type, width ) if( strcmp( sType, #id ) == 0 ) return id;
		DG_TYPE_LIST
		#undef _
		return DG_UNDEFINED;
	}

	/// Convert type ID to string representation
	inline static const char* Type2String( DGType type )
	{   
		switch( type )
		{
			#define _( id, type, width ) case id: return #id;
			DG_TYPE_LIST
			#undef _
			default:	return "DG_UNDEFINED";
		}
	}

	/// Convert type ID to string representation of underlying C type
	inline const char* Type2CTypeString( DGType type )
	{
		switch( type )
		{
			#define _( id, type, width ) case id: return #type;
			DG_TYPE_LIST
			#undef _
			default:	return "void";
		}
	}



#ifdef __cplusplus
}	// namespace DG
#endif

#endif	// DG_TYPE_H