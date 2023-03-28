//////////////////////////////////////////////////////////////////////
/// \file  dg_raii_helpers.h
/// \brief DG Core RAII helper classes and functions
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declaration of various helper classes
/// and helper functions, which perform automatic execution
/// of template actions when going out of scope (RAII paradigm)
/// Useful in automatic resource management,
/// while std::unique_resource is still not available
/// 

#ifndef CORE_DG_RAII_HELPERS_H_
#define CORE_DG_RAII_HELPERS_H_

#include <utility>

namespace DG
{
	/// Class which calls specified action on destruction
	template< typename Exit > struct RAII_Helper
	{
		/// Constructor with initializer and finalizer functions
		/// \param init - initializer function
		/// \param exit - finalizer function
		template< typename Init >
		RAII_Helper( Init &&init, Exit &&exit ): m_exit( std::forward< Exit >( exit ) ), m_canceled( false )
		{
			init();
		}

		/// Constructor with onlyfinalizer function
		/// \param exit - finalizer function
		RAII_Helper( Exit &&exit ) : m_exit( exit ), m_canceled( false ){}

		/// Destructor
		~RAII_Helper()
		{
			if ( !m_canceled )
				m_exit();
		}

		/// Cancel call to finalizer function on destruction
		void cancel(){ m_canceled = true; }

	private:
		Exit m_exit; //!< finalizer function 
		bool m_canceled; //!< cancel flag: when set no finalizer function is called
	};

	/// Return RAII_Helper object initialized to call specified finalizer function on destruction
	/// \return RAII_Helper object
	template< class Exit > RAII_Helper< Exit > RAII_Cleanup( Exit &&exit )
	{
		return RAII_Helper< Exit >( std::forward< Exit >( exit ) );
	}

	/// Return RAII_Helper object initialized to call specified initializer and finalizer functions
	/// \return RAII_Helper object
	template< class Init, class Exit > RAII_Helper< Exit > RAII_Cleanup( Init &&init, Exit &&exit )
	{
		return RAII_Helper< Exit >( std::forward< Init >( init ), std::forward< Exit >( exit ) );
	}


	/// Trivial wrapper over C array providing iterator capability
	template <typename T>
	struct ArrayWrapper
	{
		const T *m_a;			//!< raw array pointer
		size_t m_sz;			//!< its size in elements

		/// Constructor
		ArrayWrapper( const T *a, size_t sz ) noexcept: m_a( a ), m_sz ( sz ){}

		using value_type = T;				//!< value type
		using iterator = T*;				//!< iterator type
		using const_iterator = const T*;	//!< const iterator type

		/// begin() iterator
		const_iterator begin() const noexcept { return m_a; }

		/// end() iterator
		const_iterator end() const noexcept { return m_a + m_sz; }

		/// Get array size in elements
		size_t size() const { return m_sz; }

		/// Access to array data
		T* data() noexcept { return m_a; }

		/// R/O access to array data
		const T* data() const noexcept { return m_a; }
	};

}  // namespace DG

#endif // CORE_DG_RAII_HELPERS_H_
