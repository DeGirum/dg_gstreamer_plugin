//////////////////////////////////////////////////////////////////////
/// \file  dg_tensor_structs.h
/// \brief DG tensor container classes
///
/// This file contains declaration of tensor container classes
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


#ifndef DG_TENSOR_STRUCTS_H_
#define DG_TENSOR_STRUCTS_H_

#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <typeinfo>
#include "Utilities/DGErrorHandling.h"
#include "Utilities/TypeList.h"
#include "Utilities/dg_math_utilities.h"
#include "Utilities/dg_raii_helpers.h"
#include "Utilities/nameof.hpp"

namespace DG
{
	/// Basic tensor container class.
	/// Based on "linear buffer with dimension array" approach.
	/// Dynamically-typed.
	/// Supports both internally or externally allocated buffers.
	class BasicTensor
	{
	public:

		//
		// Data types
		//

		/// Tensor shape vector type
		using shape_t = std::vector< size_t >;

		/// Quantization parameters (how to convert from integer/quantized data back to floating point)
		struct quant_params_t
		{
			/// Single quantization parameter: scale and zero offset
			/// Scaling formula is real_value = m_scale * (int_value - m_zero)
			struct scale_t
			{
				double m_scale;			//!< scale factor
				int64_t m_zero;			//!< zero offset
			};

			/// Default constructor
			quant_params_t(): m_quant_axis( -1 ), m_quant_params( { { 1.0, 0 } } ){}

			/// Constructor for global quantization
			/// \param[in] global_qparam - global quantization parameters
			quant_params_t( const scale_t &global_qparam ): m_quant_axis( -1 ), m_quant_params( { global_qparam } ){}

			/// Constructor for per-axis quantization
			/// \param[in] axis - quantization axis
			/// \param[in] qparams - vector of per-axis quantization parameters
			quant_params_t( int axis, const std::vector< scale_t > &qparams ): m_quant_axis( axis ), m_quant_params( qparams ){}

			/// Alternative constructor for per-axis quantization
			/// \param[in] axis - quantization axis
			/// \param[in] scales - vector of per-axis scales
			/// \param[in] zeros - vector of per-axis zero offsets
			template < typename SCALE_T, typename ZERO_T >
			quant_params_t( int axis, const std::vector< SCALE_T > &scales, const std::vector< ZERO_T > &zeros ):
				m_quant_axis( axis )
			{
				size_t q_size = std::min( scales.size(), zeros.size() );
				if( axis < 0 )
					q_size = 1;
				m_quant_params.resize( q_size );
				for( size_t qi = 0; qi < q_size; qi++ )
				{
					m_quant_params[ qi ].m_scale = scales[ qi ];
					m_quant_params[ qi ].m_zero = zeros[ qi ];
				}
			}

			/// Get quantization axis (-1 means global quantization)
			int quant_axis() const { return m_quant_axis; }

			/// Get quantization parameters
			const std::vector< scale_t >& quant_params() const { return m_quant_params; }

			/// Get array of scales
			template < typename T >
			std::vector< T > quant_scales() const
			{
				std::vector< T > ret( m_quant_params.size() );
				std::transform( m_quant_params.begin(), m_quant_params.end(), ret.begin(),
					[]( const scale_t &s ){ return (T )s.m_scale; } );
				return ret;
			}

			/// Get array of zero offsets
			template < typename T >
			std::vector< T > quant_zeros() const
			{
				std::vector< T > ret( m_quant_params.size() );
				std::transform( m_quant_params.begin(), m_quant_params.end(), ret.begin(),
					[]( const scale_t &s ){ return (T )s.m_zero; } );
				return ret;
			}

			/// Check, if this object and given object have equal structure
			/// \param[in] rhs - object to compare with
			bool isEqualStruct( const quant_params_t &rhs ) const
			{
				return m_quant_axis == rhs.m_quant_axis &&
					m_quant_params.size() == rhs.m_quant_params.size();
			}

			/// Check, if this object and given object have equal data contents
			/// \param[in] rhs - object to compare with
			/// \param[in] maxRelDiff - maximum relative difference for floating point comparison
			bool isEqualData( const quant_params_t &rhs, double maxRelDiff ) const
			{
				if( !isEqualStruct( rhs ) )
					return false;

				for( size_t qi = 0; qi < m_quant_params.size(); qi++ )
				{
					if( m_quant_params[ qi ].m_zero != rhs.m_quant_params[ qi ].m_zero ||
						!floatCompare( m_quant_params[ qi ].m_scale, rhs.m_quant_params[ qi ].m_scale, maxRelDiff ) )
						return false;
				}
				return true;
			}

		private:
			int m_quant_axis;			//!< quantization axis, or -1 in case of global quantization

			/// array of quantization parameters, one per tensor slice along quantization axis;
			/// in case of global quantization, this array contains single element
			std::vector< scale_t > m_quant_params;
		};


		//
		// Construction
		//

		/// Default constructor
		BasicTensor() noexcept { null(); }

		/// Deleted copy constructor
		BasicTensor( const BasicTensor & ) = delete;

		/// Deleted copy assignment operator
		BasicTensor &operator=( const BasicTensor & ) = delete;

		/// Move constructor
		BasicTensor( BasicTensor&& move ) noexcept
		{
			m_id = move.m_id;
			m_name = std::move( move.m_name );
			m_linear_buffer = move.m_linear_buffer;
			m_linear_size = move.m_linear_size;
			m_el_size = move.m_el_size;
			m_external = move.m_external;
			m_shape = std::move( move.m_shape );
			m_type = move.m_type;
			m_quant_params = std::move( move.m_quant_params );
			move.null();
		}

		/// Move assignment operator
		BasicTensor& operator = ( BasicTensor&& move )
		{
			dealloc();
			m_id = move.m_id;
			m_name = std::move( move.m_name );
			m_linear_buffer = move.m_linear_buffer;
			m_linear_size = move.m_linear_size;
			m_el_size = move.m_el_size;
			m_external = move.m_external;
			m_shape = std::move( move.m_shape );
			m_type = move.m_type;
			m_quant_params = std::move( move.m_quant_params );
			move.null();
			return *this;
		}

		/// Destructor
		~BasicTensor(){ dealloc(); }


		//
		// Allocation
		//

		/// Constructor: allocate or assign tensor memory according to dimension vector (static typing version)
		/// \param[in] id - tensor ID
		/// \param[in] name - tensor name
		/// \param[in] shape - tensor shape vector, which defines dimensions of new tensor
		/// \param[in] quant_params - tensor quantization parameters
		/// \param[in] ext_lin_buffer - optional external linear buffer pointer;
		///   if nullptr, buffer will be allocated internally
		template <typename T>
		BasicTensor( int32_t id, const std::string &name, const shape_t &shape,
			const quant_params_t &quant_params = {},
			T *ext_lin_buffer = nullptr )
		{
			null();
			alloc< T >( id, name, shape, quant_params, ext_lin_buffer );
		}


		/// Constructor: allocate or assign tensor memory according to dimension vector (dynamic typing version)
		/// \param[in] id - tensor ID
		/// \param[in] name - tensor name
		/// \param[in] shape - tensor shape vector, which defines dimensions of new tensor
		/// \param[in] data_type - tensor element data type id
		/// \param[in] quant_params - tensor quantization parameters
		/// \param[in] ext_lin_buffer - optional external linear buffer pointer;
		///   if nullptr, buffer will be allocated internally
		BasicTensor( int32_t id, const std::string &name, const shape_t &shape, DGType data_type,
			const quant_params_t &quant_params = {},
			void *ext_lin_buffer = nullptr )
		{
			null();
			alloc( id, name, shape, data_type, quant_params, ext_lin_buffer );
		}


		/// Constructor: allocate tensor memory and copy data from given linear container.
		/// CONTAINER type should have value_type, data(), and size() methods.
		/// If shape is empty, 1-D tensor the size of the linear container will be allocated.
		/// The size of the data copied is the minimum of the linear container size and the shape dimensions product.
		///
		/// \param[in] source - linear container (such as string or vector) with tensor data
		/// \param[in] do_copy - when true, copy data from container into tensor-owned buffer, otherwise assign pointer to container data
		/// \param[in] id - tensor ID
		/// \param[in] name - tensor name
		/// \param[in] shape - tensor shape vector, which defines dimensions of new tensor
		/// \param[in] quant_params - tensor quantization parameters
		template < class CONTAINER >
		explicit BasicTensor( const CONTAINER &source, bool do_copy = true, int32_t id = 0, const std::string &name = "", const shape_t &shape = {},
			const quant_params_t &quant_params = {} )
		{
			null();
			alloc< CONTAINER >( source, do_copy, id, name, shape, quant_params );
		}


		/// Allocate or assign tensor memory according to dimension vector (static typing version)
		/// \param[in] id - tensor ID
		/// \param[in] name - tensor name
		/// \param[in] shape - tensor shape vector, which defines dimensions of new tensor
		/// \param[in] quant_params - tensor quantization parameters
		/// \param[in] ext_lin_buffer - optional external linear buffer pointer;
		///   if nullptr, buffer will be allocated internally
		template <typename T>
		void alloc( int32_t id, const std::string &name, const shape_t &shape,
			const quant_params_t &quant_params = {},
			T *ext_lin_buffer = nullptr )
		{
			dealloc();

			m_id = id;
			m_name = name;
			m_shape = shape;
			m_quant_params = quant_params;
			m_linear_size = std::accumulate( m_shape.begin(), m_shape.end(), (size_t )1, std::multiplies< size_t >() );
			m_el_size = sizeof( T );

			m_type = &typeid( T );
			if( ext_lin_buffer != nullptr )
			{
				m_linear_buffer = ext_lin_buffer;
				m_external = true;
			}
			else
			{
				m_linear_buffer = new T[ m_linear_size ];
				if( std::is_arithmetic_v< T > )
					std::memset( m_linear_buffer, 0, m_linear_size * m_el_size );
				m_external = false;
			}
		}


		/// Allocate or assign tensor memory according to dimension vector (dynamic typing version)
		/// \param[in] id - tensor ID
		/// \param[in] name - tensor name
		/// \param[in] shape - tensor shape vector, which defines dimensions of new tensor
		/// \param[in] data_type - tensor element data type id
		/// \param[in] quant_params - tensor quantization parameters
		/// \param[in] ext_lin_buffer - optional external linear buffer pointer;
		///   if nullptr, buffer will be allocated internally
		void alloc( int32_t id, const std::string &name, const shape_t &shape, DGType data_type,
			const quant_params_t &quant_params = {},
			void *ext_lin_buffer = nullptr )
		{
			switch( data_type )
			{
				#define _( type_id, ctype, width ) case type_id: alloc< ctype >( id, name, shape, quant_params, (ctype* )ext_lin_buffer ); break;
				DG_TYPE_LIST
				#undef _
			default:
				break;
			}
		}


		/// Allocate tensor memory and copy data from given linear container.
		/// CONTAINER type should have value_type, data(), and size() methods.
		/// If shape is empty, 1-D tensor the size of the linear container will be allocated.
		/// The size of the data copied is the minimum of the linear container size and the shape dimensions product.
		///
		/// \param[in] source - linear container (such as string or vector) with tensor data
		/// \param[in] do_copy - when true, copy data from container into tensor-owned buffer, otherwise assign pointer to container data
		/// \param[in] id - tensor ID
		/// \param[in] name - tensor name
		/// \param[in] shape - tensor shape vector, which defines dimensions of new tensor
		/// \param[in] quant_params - tensor quantization parameters
		template < class CONTAINER >
		auto alloc( const CONTAINER &source, bool do_copy = true, int32_t id = 0, const std::string &name = "",
			const shape_t &shape = {}, const quant_params_t &quant_params = {} )

			-> decltype( source.data(), source.size(), void() )
			// this fancy syntax is needed to allow this template only for CONTAINER types which have data() and size() methods

		{
			typename CONTAINER::value_type *ext_lin_buffer = do_copy ? nullptr : const_cast< typename CONTAINER::value_type* >( source.data() );

			if( shape.empty() )
				// shape is not defined: allocate 1-D tensor the size of the container
				alloc< typename CONTAINER::value_type >( id, name, { source.size() }, quant_params, ext_lin_buffer );
			else
				// shape IS defined: allocate tensor with given shape
				alloc< typename CONTAINER::value_type >( id, name, shape, quant_params, ext_lin_buffer );

			if( do_copy )
			{
				// limit data copy to the sizes of the tensor and the container
				const size_t copy_size = std::min( source.size(), linearSizeGet() );
				std::memcpy( untypedData(), source.data(), copy_size * elementSizeGet() );
			}
		}


		/// Create and return exact copy of itself
		BasicTensor clone() const
		{
			return do_clone( false );
		}


		/// Create and return deep copy of itself.
		/// External buffer data will be duplicated.
		BasicTensor copy() const
		{
			return do_clone( true );
		}


		/// Convert tensor to new element type with casting and copying data
		/// \tparam output data type T
		/// \return new owning tensor of type T
		template< typename T > BasicTensor convert() const
		{
			DG::BasicTensor ret( id(), name(), shape(), DGTypeOf< T >::value, quantParams() );
			switch( dataTypeGet() )
			{
			#define _( type_id, ctype, width )                                          \
				case type_id:                                                           \
					std::copy_n( data< ctype >(), linearSizeGet(), ret.data< T >() );   \
					break;

				DG_TYPE_LIST
			#undef _
				case DG_UNDEFINED:
					return ret;
			}
			return ret;
		}

		/// Convert tensor to new element type with casting and copying data
		/// \param[in] to_type - new element type
		/// \return new owning tensor
		BasicTensor convert( DGType to_type ) const
		{
			switch( to_type )
			{
			#define _( type_id, ctype, width ) \
				case type_id:                  \
					return convert< ctype >();

				DG_TYPE_LIST
			#undef _
				case DG_UNDEFINED:
					return {};
			}
			return {};
		}

		/// Deallocate tensor data and clear tensor
		void dealloc()
		{
			if( m_linear_buffer != nullptr && !m_external )
				delete [] (char *)m_linear_buffer;
			m_name.clear();
			m_shape.clear();
			m_quant_params = quant_params_t();
			null();
		}


		/// Reshape tensor to given shape
		/// When new shape is bigger than current, shape is appended with unity values
		/// When new shape is smaller than current, shape is reduced,
		/// and last shape dimension is multiplied by product of reduced dimensions
		void reshapeTo( size_t dim )
		{
			if( dim > m_shape.size() )
				m_shape.resize( dim, 1 );
			else if( dim < m_shape.size() && dim > 0 )
			{
				m_shape[ dim - 1 ] *= std::accumulate( m_shape.begin() + dim, m_shape.end(), (size_t )1, std::multiplies< size_t >() );
				m_shape.resize( dim );
			}
		}

		/// Reshape tensor to 4-D NHWC shape
		void reshapeToNHWC()
		{
			const size_t old_dim = m_shape.size();
			const size_t new_dim = 4;
			if( old_dim < new_dim )
			{
				m_shape.resize( new_dim, 1 );
				if( old_dim == 2 )
					std::swap( m_shape[ 3 ], m_shape[ 1 ] );
				if( old_dim == 3 )
					std::swap( m_shape[ 2 ], m_shape[ 3 ] );
			}
			else if( old_dim > new_dim )
			{
				m_shape[ new_dim - 1 ] *= std::accumulate( m_shape.begin() + new_dim, m_shape.end(), (size_t )1, std::multiplies< size_t >() );
				m_shape.resize( new_dim );
			}
		}

		/// Quantize tensor according to current quantization settings from type T_IN to type T_OUT
		/// New linear buffer will be internally allocated to receive quantized data.
		template < typename T_IN, typename T_OUT >
		void quantize()
		{
			// TODO
		}

		/// Dequantize tensor according to current quantization settings from type T_IN to type T_OUT
		/// New linear buffer will be internally allocated to receive dequantized data.
		template < typename T_IN, typename T_OUT >
		void dequantize()
		{
			if( m_linear_size == 0 )
				return;

			const T_IN *in = data< T_IN >();
			if( in == nullptr )
				DG_ERROR(
					DG_FORMAT( "Dequantize: tensor data type " << numpyTypeGet() << " does not match requested " << NAMEOF_SHORT_TYPE( T_IN ) ),
					ErrBadParameter );
			T_OUT *out = new T_OUT[ m_linear_size ];

			const auto &qarr = m_quant_params.quant_params();
			auto q_axis = m_quant_params.quant_axis();
			size_t denom = 1;	// linear index is divided by this value then...
			size_t dim = 1;		// ... the modulo by this value is calculated to obtain the index in quantization parameters array
			if( q_axis >= 0 )
			{
				if( q_axis >= m_shape.size() )
					DG_ERROR(
						DG_FORMAT( "Dequantize: tensor quantization axis " << q_axis << " is out of range 0.." << m_shape.size() ),
						ErrBadParameter );
				dim = m_shape[ q_axis ];
				if( q_axis < m_shape.size() - 1 )
					denom = std::accumulate( m_shape.begin() + q_axis + 1, m_shape.end(), 1, std::multiplies< size_t >() );
			}

			for( size_t li = 0; li < m_linear_size; li++ )
			{
				const size_t qi = ( li / denom ) % dim;
				const auto &qp = qarr[ qi ];
				out[ li ] = (T_OUT)( ( (T_OUT)in[ li ] - qp.m_zero ) * qp.m_scale );
			}

			if( !m_external )
				delete[]( char * ) m_linear_buffer;
			m_external = false;
			m_linear_buffer = out;
			m_el_size = sizeof( T_OUT );
			m_type = &typeid( T_OUT );
		}


		//
		// Accessors
		//

		///  Get id
		int32_t id() const { return m_id; }

		/// Get name
		const std::string& name() const { return m_name; }

		/// Get shape vector
		const shape_t& shape() const { return m_shape; }

		/// Get quantization parameters
		const quant_params_t& quantParams() const { return m_quant_params; }

		/// Return non-const typed pointer to underlying linear buffer.
		/// Returns nullptr if template type does not match actual runtime tensor element type
		template <typename T> T* data()
		{
			if( std::is_array_v< T > ||
				std::is_pointer_v< T > ||
				std::is_reference_v< T > ||
				std::is_const_v< T > ||
				typeid( T ) != *m_type )
			{
				return nullptr;
			}
			return (T* )m_linear_buffer;
		}

		/// Return const typed pointer to underlying linear buffer.
		/// Returns nullptr if template type does not match actual runtime tensor element type
		template <typename T> const T* data() const
		{
			if( std::is_array_v< T > ||
				std::is_pointer_v< T > ||
				std::is_reference_v< T > ||
				std::is_const_v< T > ||
				typeid( T ) != *m_type )
			{
				return nullptr;
			}
			return (const T* )m_linear_buffer;
		}

		/// Return raw data buffer
		template < typename T = void >
		T* untypedData() const { return (T* )m_linear_buffer; }

		/// Return raw data buffer
		template < typename T = void >
		T* untypedData() { return (T* )m_linear_buffer; }

		/// Return type info
		const std::type_info& typeInfo() const { return *m_type; }

		/// Get DG data type of tensor element
		DGType dataTypeGet() const
		{
			#define _( type_id, ctype, width ) if( *m_type == typeid( ctype ) ) return type_id;
			DG_TYPE_LIST
			#undef _
			return DG_UNDEFINED;
		}

		/// Return numpy-compatible type string from given DG type string
		static std::string numpyTypeGet( const std::string &dg_type )
		{
			return dg_type.substr( 0, dg_type.find( "_t" ) );
		}

		/// Return numpy-compatible type string
		std::string numpyTypeGet() const
		{
			return numpyTypeGet( DG::Type2CTypeString( dataTypeGet() ) );
		}

		/// Get tensor linear size in elements of current type
		size_t linearSizeGet() const { return m_linear_size; }

		/// Get tensor linear size in bytes
		size_t linearSizeGet_bytes() const { return m_linear_size * m_el_size; }

		/// Get tensor element size in bytes
		size_t elementSizeGet() const { return m_el_size; }

		/// Is external buffer?
		bool isExternal() const { return m_external; }

		/// Check if tensor is null
		bool isNull() const
		{
			return m_id == -1 &&
				m_name.empty() &&
				m_linear_buffer == nullptr &&
				m_linear_size == 0 &&
				m_el_size == 0 &&
				m_external == false &&
				m_type == nullptr &&
				m_shape.empty() &&
				(m_quant_params.isEqualData( quant_params_t(), 0 ) || m_quant_params.quant_params().size() == 0);
		}

		/// Check if tensor is empty
		bool empty() const
		{
			return isNull();
		}


		//
		// Comparison
		//

		/// Check, if this object and given object have equal structure
		/// \param[in] rhs - object to compare with
		bool isEqualStruct( const BasicTensor &rhs ) const
		{
			return m_shape == rhs.m_shape &&
				m_linear_size == rhs.m_linear_size &&
				*m_type == *rhs.m_type &&
				m_external == rhs.m_external &&
				m_quant_params.isEqualStruct( rhs.m_quant_params );
		}

		/// Check, if this object and given object have equal shape, element size and number of elements
		/// \param[in] rhs - object to compare with
		bool isEqualDataShape( const BasicTensor &rhs ) const
		{
			return m_shape == rhs.m_shape && m_linear_size == rhs.m_linear_size && *m_type == *rhs.m_type;
		}

		/// Check, if this object and given object have equal data contents
		/// \param[in] rhs - object to compare with
		/// \param[in] maxRelDiff - maximum relative difference for floating point comparison (ignored for integer data)
		FloatCompareResult< double > isEqualData( const BasicTensor &rhs, double maxRelDiff ) const
		{
			double max_abs_diff = 0;

			if( m_shape != rhs.m_shape || m_linear_size != rhs.m_linear_size || *m_type != *rhs.m_type )
				return { false, max_abs_diff };

			#define _( type_id, ctype, width ) \
				if( *m_type == typeid( ctype ) ) \
				{ \
					auto res = floatCompare( ArrayWrapper( (const ctype* )m_linear_buffer, m_linear_size ), \
						ArrayWrapper( (const ctype* )rhs.m_linear_buffer, rhs.m_linear_size ), (const ctype )maxRelDiff ); \
					return { (bool )res, (double )(const ctype )res }; \
				}
			DG_TYPE_LIST
			#undef _
			return { false, max_abs_diff };
		}

		/// Check, if this object and given vector have equal data contents
		/// \param[in] rhs - vector to compare with
		/// \param[in] maxRelDiff - maximum relative difference for floating point comparison (ignored for integer data)
		template <typename T >
		FloatCompareResult< double > isEqualData( const std::vector< T > &rhs, double maxRelDiff ) const
		{
			double max_abs_diff = 0;

			if( typeid( T ) != typeInfo() || rhs.size() != linearSizeGet() )
				return { false, max_abs_diff };

			#define _( type_id, ctype, width ) \
				if( *m_type == typeid( ctype ) ) \
				{ \
					auto res = floatCompare( ArrayWrapper( (const ctype* )m_linear_buffer, m_linear_size ), \
						rhs, (const ctype )maxRelDiff ); \
					return { (bool )res, (double )(const ctype )res }; \
				}
			DG_TYPE_LIST
			#undef _
			return { false, max_abs_diff };
		}

	private:

		//
		// Data members
		//

		int32_t m_id;						//!< tensor ID
		std::string m_name;					//!< tensor name
		shape_t m_shape;					//!< shape vector (last element corresponds to fastest changing index)
		quant_params_t m_quant_params;		//!< tensor quantization parameters

		void *m_linear_buffer;				//!< linear buffer
		bool m_external;					//!< linear buffer has external ownership
		size_t m_linear_size;				//!< size of linear buffer in elements of the current type
		size_t m_el_size;					//!< size in bytes of the element of the current type
		const std::type_info *m_type;		//!< tensor current data type

		/// Nullify all POD members
		void null()
		{
			m_id = -1;
			m_linear_buffer = nullptr;
			m_linear_size = 0;
			m_el_size = 0;
			m_external = false;
			m_type = nullptr;
		}

		/// Create and return exact copy of itself
		/// \param[in] copy_ext_data - when true, data from external buffer will be duplicated into cloned copy
		/// otherwise just the pointer will be copied
		BasicTensor do_clone( bool copy_ext_data ) const
		{
			BasicTensor ret;

			ret.m_id = m_id;
			ret.m_name = m_name;
			ret.m_shape = m_shape;
			ret.m_quant_params = m_quant_params;

			ret.m_external = m_external;
			ret.m_linear_size = m_linear_size;
			ret.m_el_size = m_el_size;
			ret.m_type = m_type;

			if( m_external && !copy_ext_data )
				ret.m_linear_buffer = m_linear_buffer;
			else
			{
				const size_t buf_sz = m_linear_size * m_el_size;
				ret.m_linear_buffer = new char[ buf_sz ];
				ret.m_external = false;
				std::memcpy( ret.m_linear_buffer, m_linear_buffer, buf_sz );
			}
			return ret;
		}
	};

	/// Print tensor to stream
	inline std::ostream &operator<<( std::ostream &os, const BasicTensor &t )
	{
		// [ id = 0, name = 'xxx', shape = { 1 2 3 4 }, type = uint8, lin_size = 1234 ]
		os << "[ id = " << t.id();
		if( !t.name().empty() )
			os << ", name = '" << t.name() << "'";
		os << ", shape = { ";
		for( auto d : t.shape() )
			os << d << " ";
		os << "}, type = " << t.numpyTypeGet() << ", bytes = " << t.linearSizeGet_bytes() << " ]";
		return os;
	}


	/// Collection of tensors
	using BasicTensorVector = std::vector< BasicTensor >;

}  // namespace DG

#endif	// DG_TENSOR_STRUCTS_H_
