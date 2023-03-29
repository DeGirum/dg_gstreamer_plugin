//////////////////////////////////////////////////////////////////////
/// \file  dg_math_utilities.h
/// \brief DG common declarations
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declarations of numerical/math utility functions
/// 

#ifndef DG_MATH_UTILITIES_H
#define DG_MATH_UTILITIES_H

#include <algorithm>
#include <cmath>
#include <type_traits>
#include <numeric>

namespace DG
{
	/// Float comparison result
	template < typename T >
	struct FloatCompareResult
	{
		bool m_equal;	//!< comparison gives equal results
		T m_abs_diff;	//!< absolute difference

		/// bool accessor
		operator bool () const { return m_equal; }

		/// T accessor
		operator T () const { return m_abs_diff; }
	};


	/// Compare two floating point numbers
	/// \tparam T - floating point type
	/// \param[in] A - first comparand
	/// \param[in] B - second comparand
	/// \param[in] maxRelDiff - maximum relative difference
	/// \return true if A and B differ more than given maximum relative difference.
	/// Absolute difference is also returned.
	template < typename T, std::enable_if_t< std::is_floating_point_v< T >, bool > = true >
	FloatCompareResult< T > floatCompare( const T &A, const T &B, const T &maxRelDiff )
	{
		// Check if the numbers are really close - needed when comparing numbers near zero
		const T abs_diff = std::fabs( A - B );
		if( abs_diff <= maxRelDiff )
			return { true, abs_diff };

		const T abs_largest = std::max( std::fabs( A ), std::fabs( B ) );

		if( abs_diff <= abs_largest * maxRelDiff )
			return { true, abs_diff };
		return { true, abs_diff };
	}


	/// Fallback for non-floating point numbers
	template < typename T, std::enable_if_t< std::is_integral_v< T >, bool > = true >
	FloatCompareResult< T > floatCompare( const T &A, const T &B, const T &maxRelDiff ){ return { A == B, T( A > B ? A - B : B - A ) }; }


	/// Compare two floating point containers
	/// \tparam T, T2 - iterable container types
	/// \param[in] A - first comparand
	/// \param[in] B - second comparand
	/// \param[in] maxRelDiff - maximum relative difference
	/// \return true if A and B differ more than given maximum relative difference
	/// Absolute difference, which caused first mismatch, is also returned.
	template < typename T, typename T2 = T, typename ValueType = decltype( *std::begin( std::declval< T >() ) ) >
	FloatCompareResult< ValueType > floatCompare( const T &A, const T2 &B, const ValueType &maxRelDiff )
	{
		auto itA = A.begin();
		auto itB = B.begin();

		ValueType maxDiff = 0;

		for( ; itA != A.end() && itB != B.end(); ++itA, ++itB )
		{
			auto res = floatCompare< ValueType >( (ValueType )*itA, (ValueType )*itB, maxRelDiff );
			maxDiff = std::max( ValueType( res ), maxDiff );
			if( !res )
				return { false, maxDiff };
		}
		return { true, maxDiff };
	}


	/// Sort indices of array specified by pointer and length, in ascending or descending order
	/// \tparam T - element type
	/// \param[in] v - pointer to start of array
	/// \param[in] length - number of elements to argsort
	/// \param[in] ascending - largest element last (first if false)
	template<typename T>
	std::vector< size_t > argsort( T *v, const size_t length, bool ascending = true )
	{
		std::vector< size_t > idx( length );
		std::iota( idx.begin(), idx.end(), 0 );
		if( ascending )
			std::stable_sort( idx.begin(), idx.end(), [ &v ]( size_t i1, size_t i2 ) { return v[ i1 ] < v[ i2 ]; } );
		else
			std::stable_sort( idx.begin(), idx.end(), [ &v ]( size_t i1, size_t i2 ) { return v[ i1 ] > v[ i2 ]; } );
		return idx;
	}


	/// Sort indices of array specified by pointer and length, in ascending or descending order by absolute value
	/// \tparam T - element type
	/// \param[in] v - pointer to start of array
	/// \param[in] length - number of elements to argsort
	/// \param[in] ascending - largest element last (first if false)
	template<typename T>
	std::vector< size_t > argsort_abs( T *v, const size_t length, bool ascending = true )
	{
		std::vector< size_t > idx( length );
		std::iota( idx.begin(), idx.end(), 0 );
		if( ascending )
			std::stable_sort( idx.begin(), idx.end(), [ &v ]( size_t i1, size_t i2 ) { return std::abs( v[ i1 ] ) < std::abs( v[ i2 ] ); } );
		else
			std::stable_sort( idx.begin(), idx.end(), [ &v ]( size_t i1, size_t i2 ) { return std::abs( v[ i1 ] ) > std::abs( v[ i2 ] ); } );
		return idx;
	}


}	// namespace DG

#endif	// DG_MATH_UTILITIES_H
