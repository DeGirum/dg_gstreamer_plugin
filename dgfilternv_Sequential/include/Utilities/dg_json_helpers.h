//////////////////////////////////////////////////////////////////////
/// \file  dg_json_helpers.h
/// \brief DG Core JSON helper classes and functions
///
/// This file contains declaration of various helper classes
/// and helper functions enhancing Json handling
/// 

// Copyright DeGirum Corporation 2021
// All rights reserved

//
// ****  ATTENTION!!! ****
//
// This file is used to build client documentation using Doxygen.
// Please clearly describe all public entities declared in this file,
// using full and grammatically correct sentences, avoiding acronyms,
// not omitting articles.
//


#ifndef CORE_DG_JSON_HELPERS_H_
#define CORE_DG_JSON_HELPERS_H_

#include "Utilities/DGErrorHandling.h"
#include "Utilities/dg_tensor_structs.h"
#include "Utilities/dg_raii_helpers.h"
#include "Utilities/json.hpp"
#include <functional>
#include <algorithm>


namespace DG
{
	/// Json library
	using json = nlohmann::json;


	/// Check if given key exists in Json array
	/// \param[in] json_params - Json array
	/// \param[in] section - top section name, which contains array of records
	/// if empty, key is checked on topmost level
	/// \param[in] index - index in array of records
	/// \param[in] key - key of key-value pair
	/// \return true if such key exists in array
	static inline bool jsonKeyExist( const json &json_params, const std::string &section, int index, const std::string &key )
	{
		const bool exists = section.empty() ? json_params.contains( key ) :
			( json_params.contains( section )
				&& json_params[ section ].is_array()
				&& index < json_params[ section ].size()
				&& json_params[ section ][ index ].contains( key ) );
		return exists;
	}


	/// Get value from Json array, which may not be present there
	/// \param[in] json_params - Json array
	/// \param[in] section - top section name, which contains array of records
	/// if empty, key is taken from topmost level
	/// \param[in] index - index in array of records
	/// \param[in] key - key of key-value pair
	/// \param[in] default_value - value to return, if no such key is found
	/// \return parameter value
	template< typename T > T jsonGetOptionalValue( const json &json_params,
		const std::string &section, int index, const std::string &key, const T& default_value )
	{
		if( !jsonKeyExist( json_params, section, index, key ) )
			return default_value;
		return section.empty() ? json_params[ key ].get< T >() : json_params[ section ][ index ][ key ].get< T >();
	}

	/// Set value to Json array; if it is not present, skip assignment
	/// \param[in] json_params - Json array
	/// \param[in] section - top section name, which contains array of records
	/// if empty, key is set on topmost level
	/// \param[in] index - index in array of records
	/// \param[in] key - key of key-value pair
	/// \param[in] value - value to set
	template< typename T > void jsonSetOptionalValue( json &json_params,
		const std::string &section, int index, const std::string &key, const T& value )
	{
		if( jsonKeyExist( json_params, section, index, key ) )
		{
			if( section.empty() )
				json_params[ key ] = value;
			else
				json_params[ section ][ index ][ key ] = value;
		}
	}


	/// Get value from Json array. Raise error, if not there
	/// \param[in] json_params - Json array
	/// \param[in] section - top section name, which contains array of records;
	/// if empty, key is taken from topmost level
	/// \param[in] index - index in array of records
	/// \param[in] key - key of key-value pair
	/// \return parameter value
	template< typename T > T jsonGetMandatoryValue( const json &json_params,
		const std::string &section, int index, const std::string &key )
	{
		if( !jsonKeyExist( json_params, section, index, key ) )
			DG_ERROR(
				"Incorrect Json configuration: parameter '" + key + "'" +
				(section.empty() ? "" : (" in section '" + section + "[ " + std::to_string( index ) + " ]'")) + " is missing",
				ErrBadParameter );
		return section.empty() ? json_params[ key ].get< T >() : json_params[ section ][ index ][ key ].get< T >();
	}


	/// Json helper class: contains assorted static methods to operate with Json configurations
	class JsonHelper
	{
	public:

		/// Parse given Json string with proper exception handling
		/// \param[in] json_cfg - Json string to parse
		/// \param[in] file - file where parsing happens
		/// \param[in] line - line in file where parsing happens
		/// \param[in] func - function where parsing happens
		/// \return Json array
		static json parse( const std::string &json_cfg, const char *file, const char *line, const char *func )
		{
			try
			{
				return json::parse( json_cfg );
			}
			catch( std::exception &e )
			{
				DG::ErrorHandling::errorAdd( file, line, func, DG::ErrorType::RUNTIME_ERROR, DG::ErrParseError, e.what() );
			}
			return {};
		}

		/// Parse given Json string with swallowing exceptions
		/// \param[in] json_cfg - Json string to parse
		/// \return Json array or empty array, if parsing failed
		static json parse_ignore_errors( const std::string json_cfg )
		{
			try
			{
				return json::parse( json_cfg );
			}
			catch( ... )
			{
			}
			return {};
		}


		/// Parse given Json string with proper exception handling
		#define DG_JSON_PARSE( json_cfg )		DG::JsonHelper::parse( json_cfg, __FILE__, TOSTRING(__LINE__), FUNCTION_NAME )	

		/// Container type for serialized Json array (it is std::vector< unit8_t >)
		using serial_container_t = json::binary_t::container_type;


		/// Serialize given Json array into byte vector using conversion to msgpack
		/// \param[in] j - Json array to serialize
		/// \return byte vector with Json array contents converted to msgpack representation
		static serial_container_t jsonSerialize( const json &j )
		{
			return json::to_msgpack( j );
		}


		/// Deserialize given byte vector with msgpack data into Json array
		/// \param[in] v - byte vector with msgpack data to deserialize
		/// \return Json array
		static json jsonDeserialize( const serial_container_t &v )
		{
			return json::from_msgpack( v );
		}


		/// Convert basic tensor to Json array
		/// \param[in] t - basic tensor 
		/// \return Json array
		static json tensorSerialize( const BasicTensor &t )
		{
			json scales = {};
			json zeroes = {};
			auto qparams = t.quantParams().quant_params();
			for( auto &q : qparams )
			{
				scales.push_back( q.m_scale );
				zeroes.push_back( q.m_zero );
			}

			// serialize linear buffer data
			serial_container_t byte_vector( t.linearSizeGet_bytes() );
			memcpy( byte_vector.data(), t.untypedData(), byte_vector.size() );

			return json(
			{
				{ "id",		t.id() },
				{ "name",	t.name() },
				{ "shape",	t.shape() },
				{ "quantization",
					{
						{ "axis", t.quantParams().quant_axis() },
						{ "scale", scales },
						{ "zero", zeroes },
					}
				},
				{ "type", Type2String( t.dataTypeGet() ) },
				{ "size", t.linearSizeGet() },
				{ "data", json::binary( std::move( byte_vector ) ) }
			} );
		}


		/// Convert Json array to basic tensor
		/// \param[in] j - Json array
		/// \return basic tensor
		static BasicTensor tensorDeserialize( const json &j )
		{
			BasicTensor t;

			try
			{
				const int32_t id = j[ "id" ].get< int32_t >();
				const std::string name = j[ "name" ].get< std::string >();
				const BasicTensor::shape_t shape = j[ "shape" ].get< BasicTensor::shape_t >();
				const auto q = j[ "quantization" ];
				const int axis = q[ "axis" ].get< int >();
				const std::vector< double > scales = q[ "scale" ].get< std::vector< double > >();
				const std::vector< int64_t > zeros = q[ "zero" ].get< std::vector< int64_t > >();
				const DGType type = Sting2DGType( j[ "type" ].get< std::string >().c_str() );
				
				size_t q_size = std::min( scales.size(), zeros.size() );
				std::vector< BasicTensor::quant_params_t::scale_t > qparams( q_size );
				for( size_t qi = 0; qi < q_size; qi++ )
				{
					qparams[ qi ].m_scale = scales[ qi ];
					qparams[ qi ].m_zero = zeros[ qi ];
				}

				const auto &byte_vector = j[ "data" ].get_binary();
				t.alloc( id, name, shape, type, BasicTensor::quant_params_t( axis, qparams ) );
				memcpy( t.untypedData(), byte_vector.data(), std::min( byte_vector.size(), t.linearSizeGet_bytes() ) );
			}
			catch( std::exception &e )
			{
				DG_ERROR( e.what(), ErrParseError );
			}
			return t;
		}

		/// Create and return Json error packet with given message
		static json errorCreate( const std::string &message )
		{
			return json( { { "success", false }, { "msg", message } } );
		}

		/// Check server Json response for errors and throw exception, if any
		/// \param[in] response - Json response from server/core
		/// \param[in] source - description of the server command initiator
		/// \param[in] do_throw - if true, throws exception in case of error
		/// \return original error string, if error is detected, empty string otherwise
		static std::string errorCheck( const json &response, const std::string &source, bool do_throw = true )
		{
			if( response.contains( "success" ) )
			{
				if( !response[ "success" ].get< bool >() )
				{
					std::string msg;
					if( response.contains( "msg" ) )
						msg = response[ "msg" ].get< std::string >();
					else
						msg = "unspecified error";

					if( do_throw )
					{
						if( source.empty() )
							DG_ERROR( msg, ErrOperationFailed );
						else
							DG_ERROR( source + ": " + msg, ErrOperationFailed );
					}
					return msg;  // error detected
				}
			}
			return "";  // no error
		}

	};
}  // namespace DG

#endif // CORE_DG_JSON_HELPERS_H_
