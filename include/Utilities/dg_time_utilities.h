//////////////////////////////////////////////////////////////////////
/// \file  dg_time_utilities.h
/// \brief DG time handling utility functions
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains implementation of various helper functions
/// for time handling
/// 

#ifndef DG_TIME_UTILITIES_H_
#define DG_TIME_UTILITIES_H_

#include <chrono>
#include <time.h>
#include <string>
#include <float.h>

namespace DG
{
	class TimeHelper
	{
	public:

		/// Return time since epoch in nanoseconds
		static uint64_t epochTimeGet_ns()
		{
			return (uint64_t )std::chrono::duration_cast< std::chrono::nanoseconds >(
				std::chrono::high_resolution_clock::now().time_since_epoch() ).count();
		}

		/// Time since epoch in seconds
		using epoch_time_t = double;

		/// Precise sleep using spinning
		/// \param[in] sleep_us - sleep duration in usec
		static void spinSleep( double sleep_us )
		{
			const auto spin_start = std::chrono::high_resolution_clock::now();
			while( std::chrono::duration_cast< std::chrono::nanoseconds >( 
				std::chrono::high_resolution_clock::now() - spin_start ).count() < sleep_us * 1e3 )
				;
		}

		/// Return time since epoch in seconds
		static epoch_time_t epochTimeGet_s()
		{
			return 1e-9 * epochTimeGet_ns();
		}

		/// Alias for backward compatibility
		#define get_epoch_time_s DG::TimeHelper::epochTimeGet_s

		/// Get current time as ctime-formatted string
		static std::string curStringTime()
		{
			const time_t cur_time = time( nullptr );
			char cur_time_str[ 64 ];
			#ifdef _WIN32
				ctime_s( cur_time_str, sizeof( cur_time_str ), &cur_time );
			#else
				ctime_r( &cur_time, cur_time_str );
			#endif
			return cur_time_str;
		}

		/// Class to measure elapsed time
		class Duration
		{
		public:
			/// Constructor. Records start time.
			Duration(): m_start( std::chrono::high_resolution_clock::now() ){}

			/// Latch and return elapsed time since object construction
			/// \return elapsed time in ms
			double elapsed_ms()
			{
				if( m_latched_duration_ms < 0 )
					m_latched_duration_ms = delta< std::milli >();

				return m_latched_duration_ms;
			}

			/// return elapsed time since object construction
			/// \tparam T - desired time units
			/// \return elapsed time in specified units
			template<typename T> double delta() const
			{
				return std::chrono::duration< double, T >( std::chrono::high_resolution_clock::now() - m_start ).count();
			}

		private:
			std::chrono::high_resolution_clock::time_point m_start;		//!< start time
			double m_latched_duration_ms = -1;							//!< duration (latched)
		};
	};


	/// Wait until given function returns true or until timeout
	/// \param[in] f - function to poll
	/// \param[in] timeout_ms - polling timeout; -1 - infinite wait, 0 - do not wait
	/// \return false if f() always returned false during timeout_ms
	template< typename FUNC > bool pollingWaitFor( FUNC f, double timeout_ms )
	{
		auto start_time = std::chrono::high_resolution_clock::now();
		if( timeout_ms < 0 )
			timeout_ms = DBL_MAX;
		for(;;)
		{
			const double time_delta_ms = 1e-3 * (double )(std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::high_resolution_clock::now() - start_time ).count() );
			const bool is_timeout = time_delta_ms > timeout_ms;
			const auto res = f();
			// important! check condition one more time even if timeout is already detected
			// this makes the code debugger-friendly: regardless of how long you stay inside f(), code will work correctly
			if( res )
				return true;
			if( is_timeout )
				return false;
		}
	}


	/// Tracker of utilization of a resource
	template< typename my_clock >
	class UtilizationTrackerBase
	{
	public:
		using my_duration = typename my_clock::duration;		//!< clock duration type alias
		using my_time_point = typename my_clock::time_point;	//!< clock time point type alias

		/// Constructor
		/// \param[in] idle_time - idle duration threshold: utilization counters will reset when the period of inactivity exceeds this duration
		UtilizationTrackerBase( my_duration idle_time = std::chrono::seconds( 1 ) ):
			m_idle_time_threshold( idle_time ),
			m_nesting_level( 0 ),
			m_active_duration( std::chrono::seconds( 0 ) ),
			m_not_active_duration( std::chrono::seconds( 0 ) )
		{
			m_start = m_stop = my_clock::now();
		}

		/// Call this method when start using a resource to mark the starting point of resource usage (can be nested)
		void start()
		{
			if( m_nesting_level == 0 )
			{
				m_start = my_clock::now();

				// reset accumulators after long idle time
				auto idle_time = m_start - m_stop;
				if( idle_time > m_idle_time_threshold )
				{
					m_active_duration = m_not_active_duration = std::chrono::seconds( 0 );
					m_stop = m_start;
				}
				else
					m_not_active_duration += idle_time;
			}
			m_nesting_level++;
		}

		/// Call this method when stop using a resource to mark the ending point of resource usage (can be nested)
		void stop()
		{
			if( m_nesting_level == 1 )
			{
				m_stop = my_clock::now();
				m_active_duration += m_stop - m_start;
			}
			if( m_nesting_level > 0 )
				m_nesting_level--;
		}

		/// Compute and return current utilization of a resource
		/// \return current resource utilization in percent 0..100
		double currentUtilization() const
		{
			const double total_duration = ( m_active_duration + m_not_active_duration ).count();
			return total_duration > 0 ? 100. * m_active_duration.count() / total_duration : 0;			
		}

	private:
		my_time_point m_start;					//!< timestamp of the last outermost start() call
		my_time_point m_stop;					//!< timestamp of the last outermost stop() call
		int m_nesting_level = 0;				//!< start() nesting level
		my_duration m_idle_time_threshold;		//!< idle duration threshold: utilization accumulators will reset when the period of inactivity exceeds this duration
		my_duration m_active_duration;			//!< accumulated duration of resource usage times
		my_duration m_not_active_duration;		//!< accumulated duration of resource spare times

	};

	/// Tracker of utilization of a resource based on high-resolution clock
	using UtilizationTracker = UtilizationTrackerBase< std::chrono::high_resolution_clock >;

}  // namespace DG

#endif // DG_TIME_UTILITIES_H_
