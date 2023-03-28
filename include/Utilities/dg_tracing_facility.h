//////////////////////////////////////////////////////////////////////
/// \file  dg_tracing_facility.h
/// \brief DG tracing facility
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declarations of tracing facility functionality.
/// Tracing facility is intended to be used for frequent events, when
/// the speed of tracing is very important, but it is OK to lose the trace 
/// information in case of app crash.
/// For high-reliability (but slow) logging, use logging facility (Utilities/DGLog.h)
/// 

#ifndef DG_TRACING_FACILITY_H
#define DG_TRACING_FACILITY_H

#include "dg_com_decl.h"

///
/// To start using tracing facility all you need to do, is to include this header file in your .cpp file
/// 
/// The following macros are used to do tracing:
///
/// The set of "fast" macros (they do not do printf() so the overhead in release build is 100-200ns):
/// 
/// 1. DG_TRC_START/DG_TRC_STOP - use them to trace starting and ending points of some activity in your code.
/// Tracing facility will measure and report the duration between start and stop points for you.
/// 
/// 2. DG_TRC_POINT - use it just to trace some point in your code
/// 
/// The set of "convenient" macros (they allow specifying messages in printf-like format so the
/// overhead in release build is 400-700ns):
/// 
/// 3. DG_TRC_START_MSG/DG_TRC_STOP_MSG - to trace starting and ending points of some activity in your code
///
/// 4. DG_TRC_POINT_MSG - just to trace some point in your code
///
/// Common parameters:
/// 
/// - group: trace group name as defined in DG_TRC_GROUP_DEF() macro (more on trace groups below).
///   Every trace belongs to some trace group, and the trace group defines, will this trace be active or not.
/// 
/// - name: trace point unique name. The name should not contain spaces, commas, quotes, backslashes,
///   or otherwise disrupt macro parameters parsing. If you use it in START/STOP macros, this name
///   should be the same in matching pair.
/// 
/// - level: tracing level (DGTrace::lvlXXX). Trace point will be traced only when the trace point
///   tracing level is not higher than the tracing level currently set for the corresponding trace group.
///
/// - message: message to add to the trace. Must be string literal for non _MSG macros.
///   In case of _MSG macros, it is printf-style format string followed by printf-arguments list.
///
/// If you prefer using RAII-style helper to trace entry-exit points from blocks, you can use DG_TRC_BLOCK macro.
/// Usage:
///   DG_TRC_BLOCK( group, name, level, message, ... )
/// All arguments are the same as for _MSG macros.
/// It will issue start trace point on construction and stop trace point on destruction.
/// Note: even if the tracing is disabled, the object will be created and destroyed.
///
/// All tracing information is saved into "dg_trace.txt" file (see DG_TRC_TRACE_FILE) in the current process directory.
/// If such file existed before, it is renamed to .bak
/// 
/// All tracing macros refer to trace groups. A trace group is defined by DG_TRC_GROUP_DEF() macro.
/// You define a single trace group for a set of logically related trace points. By using trace group
/// mechanism you can select the tracing level for all trace points referring to that group.
/// How it works:
/// Each trace point has certain tracing level as specified in "level" macro argument. If this tracing level is 
/// higher than the current tracing level of the corresponding  trace group, tracing of this trace point will be skipped.
/// (the overhead of skipped trace is just few instructions).
/// 
/// The trace group tracing level is specified in the tracing configuration file: "dg_trace.ini" (see DG_TRC_CONFIG_FILE).
/// Tracing configuration file is searched in the current process directory and parsed during static objects initialization.
/// 
/// Configuration file consists of zero or more lines like that:
/// group = trace level
/// Where "group" is the group name as defined in DG_TRC_GROUP_DEF() macro,
/// and "trace level" is either Basic, Detailed, or Full (directly corresponds to DGTrace::lvlXXX constants)
/// All lines are case insensitive. All whitespace separators are ignored.
/// If a line starts with '#' or ';' or '//' it is considered as a comment and ignored.
/// 


/// Macro defines the trace group variable name by prefixing it with some unique prefix
#define DG_TRC_GROUP_VAR( name ) __dg_trace_##name

/// Define trace group with given name. The name should be valid C++ identifier name.
/// Trace group is the entity which is then references by some set of logically-connected 
/// trace points. The tracing level can be set for each trace group in the configuration file.
/// A trace point will be traced only when the tracing level of that trace point is not higher than
/// the tracing level currently set for the corresponding trace group.
#define DG_TRC_GROUP_DEF( name ) \
	inline DGTrace::TraceLevel_t DG_TRC_GROUP_VAR( name ) = \
		DGTrace::g_TraceGroupsRegistry.registerTraceGroup( &DG_TRC_GROUP_VAR( name ), #name );

/// RAII-style macro to trace entry-exit points from a block
#define DG_TRC_BLOCK( group, name, level, ... ) \
	DGTrace::Tracer DG_CONCAT( __dg_trace_, __LINE__ )( &DGTrace::g_TracingFacility, \
		&DG_TRC_GROUP_VAR( group ), #group "::" #name, level, ##__VA_ARGS__ )

/// Helper macros used in DG_TRC_xxx macros to reduce copy-paste
/// Trace with static message
#define DG_TRC_DO( group, name, level, type, ... ) \
	{ if( ( level ) <= DG_TRC_GROUP_VAR( group ) ) \
		DGTrace::g_TracingFacility.traceDo( type, #group "::" #name, level, ##__VA_ARGS__ ); }
/// Trace with printf-like message
#define DG_TRC_PRINTF_DO( group, name, level, type, msg, ... ) \
	{ if( ( level ) <= DG_TRC_GROUP_VAR( group ) ) \
		DGTrace::g_TracingFacility.tracePrintfDo( type, #group "::" #name, level, msg, ##__VA_ARGS__ ); }

/// Trace starting point of some activity
#define DG_TRC_START( group, name, level, ... ) \
	DG_TRC_DO( group, name, level, DGTrace::TracingFacility::TraceType::Start, ##__VA_ARGS__ )
/// Trace ending point of some activity
#define DG_TRC_STOP( group, name, level, ... ) \
	DG_TRC_DO( group, name, level, DGTrace::TracingFacility::TraceType::Stop, ##__VA_ARGS__ )
/// Trace some point in code
#define DG_TRC_POINT( group, name, level, ... ) \
	DG_TRC_DO( group, name, level, DGTrace::TracingFacility::TraceType::Point, ##__VA_ARGS__ )

/// Trace starting point of some activity with printf-style message
#define DG_TRC_START_MSG( group, name, level, message, ... ) \
	DG_TRC_PRINTF_DO( group, name, level, DGTrace::TracingFacility::TraceType::Start, message, ##__VA_ARGS__ )
/// Trace ending point of some activity with printf-style message
#define DG_TRC_STOP_MSG( group, name, level, message, ... ) \
	DG_TRC_PRINTF_DO( group, name, level, DGTrace::TracingFacility::TraceType::Stop, message, ##__VA_ARGS__ )
/// Trace some point in code with printf-style message
#define DG_TRC_POINT_MSG( group, name, level, message, ... ) \
	DG_TRC_PRINTF_DO( group, name, level, DGTrace::TracingFacility::TraceType::Point, message, ##__VA_ARGS__ )


/// Trace critical information unconditionally with printf-like message
#define DG_TRC_CRITICAL( name, msg, ... ) \
	DGTrace::g_TracingFacility.tracePrintfDo( DGTrace::TracingFacility::TraceType::Point, name, DGTrace::lvlNone, msg, ##__VA_ARGS__ )

/// Flush all buffered trace contents to file
#define DG_TRC_FLUSH()	DGTrace::g_TracingFacility.flush()

//////////////////////////////////////////////////////////////////////
// Internal implementation. Do not use directly!

#include <cstdio>
#include <chrono>
#include <ctime>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <tuple>
#include <type_traits>
#include <algorithm>
#include <filesystem>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "dg_time_utilities.h"
#ifndef _WIN32
	#define _stricmp strcasecmp
#endif

/// trace file name
#define DG_TRC_TRACE_FILE		"dg_trace.txt"
/// trace configuration file name
#define DG_TRC_CONFIG_FILE		"dg_trace.ini"


namespace DGTrace
{
	/// Tracing level type. Less important/more detailed information should have higher tracing level.
	using TraceLevel_t = unsigned;

	/// Some predefined tracing levels
	enum : TraceLevel_t
	{
		lvlNone = 0,				//!< tracing is disabled
		lvlBasic = 1,				//!< only basic trace points are traced
		lvlDetailed = 2,			//!< detailed trace points are traced
		lvlFull = 3					//!< all trace points are traced		
	};

	/// Registry of trace groups
	/// Important: this structure and all its members MUST be POD with no constructors!
	/// Since it is accessed during initialization of global variables this is needed 
	/// to avoid problems with non-deterministic initialization sequence of static objects.
	struct TraceGroupsRegistry
	{
		/// Trace group descriptor
		struct TraceGroupDesc
		{
			TraceLevel_t *m_groupAddress;	//!< address of trace group variable
			const char *m_groupName;		//!< trace group symbolic name
		};

		/// Register trace group. Used in DG_TRACE_GROUP_DEF() macro.
		/// Called during global variables initialization
		/// \param[in] groupAddress - address of trace group variable
		/// \param[in] groupName - trace group symbolic name: used in configuration file to reference trace groups.
		/// \returns group trace level as it is read from configuration file or lvlNone if it is not in configuration file
		TraceLevel_t registerTraceGroup( TraceLevel_t *groupAddress, const char *groupName )
		{
			if( m_groupsCount < MAX_GROUPS )
			{
				m_groupsRegistry[ m_groupsCount ].m_groupAddress = groupAddress;
				m_groupsRegistry[ m_groupsCount ].m_groupName = groupName;
				applyConfig( m_groupsCount );
				m_groupsCount++;
			}
			return *groupAddress;
		}

		/// Return the list of registered trace groups: pointer to the beginning and size
		std::pair< const TraceGroupDesc*, size_t > traceGroupsGet() const
		{
			return { m_groupsRegistry, m_groupsCount };
		}

		/// Apply tracing levels to registered trace groups
		/// \param[in] config - trace configuration list of pairs: trace group name and its new tracing level;
		/// trace group name should be one of returned by traceGroupsGet();
		/// pass empty list to disable all groups
		void traceGroupsApply( const std::vector< std::pair< std::string, TraceLevel_t > > &config )
		{
			for( size_t gi = 0; gi < m_groupsCount; gi++ )
			{
				auto c = std::find_if( config.begin(), config.end(), [&]( const auto &v ) {
					return _stricmp( m_groupsRegistry[ gi ].m_groupName, v.first.c_str() ) == 0;
				 } );

				// group not in config -> disable it, otherwise set trace level from config
				*( m_groupsRegistry[ gi ].m_groupAddress ) = c == config.end() ? lvlNone : c->second;
			}
		}


		/// Print trace header into trace stream
		void printHeader( std::ostream &out_stream )
		{
			out_stream << "----------------------------------------\n";
			out_stream << "Started: " << DG::TimeHelper::curStringTime() << '\n';

			out_stream << "Enabled trace groups:\n";
			bool nothing_enabled = true;
			for( size_t ci = 0; ci < m_groupsCount; ci++ )
				if( *(m_groupsRegistry[ ci ].m_groupAddress) != lvlNone )
				{
					nothing_enabled = false;
					out_stream << "  " << std::setw( 32 ) << std::left << m_groupsRegistry[ ci ].m_groupName
						<< " = " << (
						*(m_groupsRegistry[ ci ].m_groupAddress) == lvlBasic ? "Basic" :
						*(m_groupsRegistry[ ci ].m_groupAddress) == lvlDetailed ? "Detailed" :
						*(m_groupsRegistry[ ci ].m_groupAddress) == lvlFull? "Full" : "None") << '\n';
				}
			if( nothing_enabled )
				out_stream << "  <none>\n\n";
			else
				out_stream << "\n";

			if( m_TraceStatisticsEnable )
				out_stream << "Trace statistics enabled\n";

			if( m_TraceImmediateFlush )
				out_stream << "Immediate flush enabled (NOTE: this option degrades performance)\n";

			out_stream << "\n\nLine format:\n";
			out_stream << "[<Timestamp, us>:<delta, us] <thread ID> [<level>] <type> <name>: <message> <-- <duration, usec>\n";
			out_stream << "* in first position means timing of this trace point is distorted by forced file flush\n\n";
		}

		/// Get path to DG temporary directory
		inline static std::string getTempPath();

		// tracing facility configuration parameters 
		bool m_TraceStatisticsEnable;		//!< enable collection and reporting of trace statistics
		bool m_TraceImmediateFlush;			//!< flush trace immediately, do not buffer

	private:

		enum { MAX_GROUPS = 1000 };			//!< max. # of trace groups
		enum { MAX_GR_NAME = 64 };			//!< max. trace group name length

		size_t m_groupsCount;				//!< # of registered trace groups
		TraceGroupDesc
			m_groupsRegistry[ MAX_GROUPS ];	//!< array of all registered trace groups

		/// Trace group configuration record
		struct TraceGroupConfig
		{
			TraceLevel_t m_groupLevel;		//!< group tracing level
			char m_groupName[ MAX_GR_NAME ];//!< group symbolic name
		};

		TraceGroupConfig
			m_groupsConfig[ MAX_GROUPS ];	//!< array of all loaded group config. records
		size_t m_configsCount;				//!< # of config. records


		/// Load configuration from configuration file
		void loadConfig()
		{
			// format: <group>=<trace level>
			// case insensitive
			// trace level: basic, detailed, full
			std::ifstream fcfg( getTempPath() + DG_TRC_CONFIG_FILE );
			if( !fcfg.good() )
			{
				m_configsCount = -1;	// meaning no config. file is present
				return;
			}

			// trim whitespaces from both ends of string
			auto trim_spaces = []( const std::string &s ) -> std::string
			{
				const char ws[] = " \t";
				const size_t bi = s.find_first_not_of( ws );
				const size_t ei = s.find_last_not_of( ws );				
				return bi != std::string::npos ? s.substr( bi, ei - bi + 1 ) : "";
			};

			// parse internal tracing facility parameters
			auto traceParamsParse = [ & ]( const std::string &group_str, const std::string &value_str ) -> bool
			{
				if( group_str == "__TraceStatisticsEnable" )
				{
					m_TraceStatisticsEnable = value_str == "yes" || value_str == "true" || value_str == "1";
					return true;
				}
				else if( group_str == "__TraceImmediateFlush" )
				{
					m_TraceImmediateFlush = value_str == "yes" || value_str == "true" || value_str == "1";
					return true;
				}
				// ... add new parameters here
				return false;
			};

			// parse config. file line-by-line
			while( !fcfg.eof() )
			{
				std::string line;

				std::getline( fcfg, line );
				if( fcfg.fail() )
					break;

				if( line.length() < 2 )
					continue;	// too short string
				if( line[ 0 ] == '#' || line[ 0 ] == ';' || (line[ 0 ] == '/' && line[ 1 ] == '/') )
					continue;	// skip all possible comments
				size_t sep = line.find_first_of( '=' );
				if( sep  == std::string::npos )
					continue;	// no '=' separator

				const std::string group_str = trim_spaces( line.substr( 0, sep ) );
				std::string level_str = trim_spaces( line.substr( sep + 1 ) );
				for( auto &c : level_str )
					c = tolower( c );

				// parse internal tracing facility parameters
				if( traceParamsParse( group_str, level_str ) )
					continue;

			#ifdef _MSC_VER
				strncpy_s( m_groupsConfig[ m_configsCount ].m_groupName, MAX_GR_NAME, group_str.c_str(), MAX_GR_NAME );
			#else
				strncpy( m_groupsConfig[ m_configsCount ].m_groupName, group_str.c_str(), MAX_GR_NAME );
			#endif

				m_groupsConfig[ m_configsCount ].m_groupName[ MAX_GR_NAME - 1 ] = '\0';

				m_groupsConfig[ m_configsCount ].m_groupLevel =
					level_str == "basic" ? lvlBasic :
					level_str == "detailed" ? lvlDetailed :
					level_str == "full" ? lvlFull :
					lvlNone;

				m_configsCount++;
			}

			if( m_configsCount == 0 )
				m_configsCount = -1;
		}

		/// Apply configuration to given group entry
		void applyConfig( size_t idx )
		{
			if( m_configsCount == 0 )	// == 0 means config was never loaded
				loadConfig();
			if( m_configsCount == -1 )	// == -1 means config was loaded unsuccessfully
				return;

			for( size_t ci = 0; ci < m_configsCount; ci++ )
			{
				if( _stricmp( m_groupsRegistry[ idx ].m_groupName, m_groupsConfig[ ci ].m_groupName ) == 0 )
				{
					// assign tracing level to group variable from configuration
					*(m_groupsRegistry[ idx ].m_groupAddress) = m_groupsConfig[ ci ].m_groupLevel;
					break;
				}
			}
		}
	};

	/// Define global variable to store trace groups registry with weak linkage.
	/// Weak linkage is needed so this variable will be linked only once when this file is
	/// included in multiple .cpp files
	inline TraceGroupsRegistry g_TraceGroupsRegistry;

	/// Tracing facility class: implements all functionality of tracing facility 
	class TracingFacility
	{
	public:

		/// Trace point types
		enum class TraceType : int
		{
			Invalid = 0,	//!< trace invalid/not filled record (must be zero!)
			Start,			//!< trace start record
			Stop,			//!< trace stop record
			Point			//!< trace point record
		};


		/// Constructor
		/// It reads configuration file and sets tracing level of all registered trace groups.
		/// It allocates tracing buffers.
		/// \param[in] traceBufCnt - trace buffer size in records
		/// \param[in] stringPoolSize - string pool buffer size in bytes
		/// \param[in] out_stream - optional output stream to print trace into (used in unit tests)
		TracingFacility( size_t traceBufCnt = 10000, size_t stringPoolSize = 100000, std::ostream *out_stream = nullptr ):
			m_traceBuf( traceBufCnt ),
			m_stringPool( stringPoolSize ),
			m_isOwnStream( false ),
			m_outStream( out_stream ),
			m_poison( false ),
			m_do_flush( false ),
			m_do_restart( false )
		{
			if( out_stream == nullptr )
			{
				m_outStream = &m_outFileStream;
				m_isOwnStream = true;
			}
		}

		/// Destructor
		~TracingFacility()
		{
			if( m_traceBuf.m_BufWP > m_traceBuf.m_BufRP )
				flush();

			// send termination request to worker thread and wait for completion
			if( m_thread.joinable() )
			{
				std::unique_lock< std::mutex > lk( m_thread_mutex );
				m_poison = true;
				m_thread_cv.notify_one();
				lk.unlock();
				m_thread.join();
			}
		}


		/// Add trace to trace buffer (fast variant, without printf)
		/// \param[in] type - trace type (start/stop/point)
		/// \param[in] name - trace name
		/// \param[in] level - tracing level
		/// \param[in] message - static message (not in string pool)
		/// \param[in] flags - flags inherited from caller (used when called from tracePrintfDo)
		inline void traceDo( TraceType type, const char *name, TraceLevel_t level,
			const char *message = nullptr, unsigned flags = 0 )
		{
			// reserve position in the buffer: atomically increment write pointer
			size_t free_pos = m_traceBuf.m_BufWP.fetch_add( 1 );
			bool timing_is_distorted = false;

			// wait until reserved position is freed
			while( free_pos - m_traceBuf.m_BufRP >= m_traceBuf.m_BufSize - 1 )
			{
				ensureThreadRuns();

				// signal worker thread to print the buffer and yield
				m_thread_cv.notify_one();
				std::this_thread::yield();
				timing_is_distorted = true;
			}

			free_pos %= m_traceBuf.m_BufSize;	// wrap it over

			auto &rec = m_traceBuf.m_Buf[ free_pos ];
			rec.m_traceName = name;
			rec.m_level = level;
			rec.m_timeStamp = clock::now();
			rec.m_Flags = flags | (timing_is_distorted ? TraceRec::TimingDistorted : 0);
			rec.m_threadID = std::this_thread::get_id();
			rec.m_message = message;
			rec.m_type = type;	// should be assigned last; this marks that the record becomes valid

			// flush critical traces and if flush is enabled globally
			if( level == lvlNone || g_TraceGroupsRegistry.m_TraceImmediateFlush )
				flush();
		}

		/// Add trace to trace buffer with printf-like message (slow variant)
		/// \param[in] type - trace type (start/stop/point)
		/// \param[in] name - trace name
		/// \param[in] level - tracing level
		/// \param[in] message - printf-like message format string
		inline void tracePrintfDo( TraceType type, const char *name, TraceLevel_t level, const char *message, ... )
		{
			va_list args;
			va_start( args, message );
			traceVPrintfDo( type, name, level, message, args );
			va_end( args );
		}


		/// Add trace to trace buffer with vprintf-like vararg list (slow variant)
		/// \param[in] type - trace type (start/stop/point)
		/// \param[in] name - trace name
		/// \param[in] level - tracing level
		/// \param[in] message - printf-like message format string
		/// \param[in] args - vprintf-compatible vararg list
		void traceVPrintfDo( TraceType type, const char *name, TraceLevel_t level, const char *message, va_list args )
		{
			char msg_buf[ DG_LOG_TRACE_BUF_SIZE ];
			msg_buf[ sizeof msg_buf - 1 ] = '\0';

			const size_t msg_len = std::min(	// message length including trailing zero
				(size_t )vsnprintf( msg_buf, sizeof msg_buf, message, args ) + 1,
				sizeof msg_buf );

			if( msg_len > 0 )
			{
				m_stringPool.lock();	// lock string pool to have strings in pool in the same order as records in trace buffer

										// reserve space in the pool: atomically increment write pointer
				size_t free_pos = m_stringPool.m_BufWP.fetch_add( msg_len );
				unsigned flags = TraceRec::InStringPool;

				// wait until reserved space is freed
				while( free_pos - m_stringPool.m_BufRP >= m_stringPool.m_BufSize - msg_len )
				{
					ensureThreadRuns();

					// signal worker thread to print the buffer and yield
					m_thread_cv.notify_one();
					std::this_thread::yield();
					flags |= TraceRec::TimingDistorted;
				}

				const char *msg_ptr = m_stringPool.put( msg_buf, msg_len, free_pos );
				traceDo( type, name, level, msg_ptr, flags );

				m_stringPool.unlock();
			}
			else
				traceDo( type, name, level, nullptr );
		}


		/// Flush trace to file
		/// \param[in] do_wait - wait for flush completion
		void flush( bool do_wait = false )
		{
			ensureThreadRuns();
			m_do_flush = true;
			{
				std::unique_lock< std::mutex > lk( m_thread_mutex );
				m_thread_cv.notify_one();  // notify under mutex to guarantee not loosing event
			}
			while( do_wait && m_do_flush )
				std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}


		/// Restart trace file, backing up current trace file.
		/// \param[in] do_wait - wait for restart completion
		void restart( bool do_wait = false )
		{
			ensureThreadRuns();
			m_do_restart = true;
			{
				std::unique_lock< std::mutex > lk( m_thread_mutex );
				m_thread_cv.notify_one();  // notify under mutex to guarantee not loosing event
			}
			while( do_wait && m_do_restart )
				std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
		}

		/// Read and return current trace file contents
		/// \param[in] offset - start read offset in file
		/// \param[in] size - size of data to read, -1 to read whole file
		inline std::string readTraceFile( size_t offset = 0, size_t size = -1 );

	private:

		/// Chrono clock type to be used by facility
		using clock = std::chrono::high_resolution_clock;

		/// One trace record
		struct TraceRec
		{
			volatile TraceType m_type;			//!< trace type (start/stop/point)
			const char* m_traceName;			//!< trace name
			TraceLevel_t m_level;				//!< tracing level
			clock::time_point m_timeStamp;		//!< trace time stamp
			std::thread::id m_threadID;			//!< thread ID, which performed tracing
			const char* m_message;				//!< additional trace message (can be nullptr)

												/// various bit flags
			enum Flags : unsigned 
			{
				TimingDistorted = 0x01,			//!< timing of this record may be distorted due to waiting for buffer flush
				InStringPool = 0x02,			//!< message is in string pool
			};
			unsigned m_Flags;					//!< various bit flags

			/// Are two trace points match?
			bool match( const TraceRec &rhs )
			{
				// we consider trace points are matching,
				// if trace names are the same and they are reported from the same thread
				return m_threadID == rhs.m_threadID
					&& strcmp( m_traceName, rhs.m_traceName ) == 0;
			}
		};

		// semantics of read and write pointers:
		// WP points to first free record
		// RP points to first used record
		// so [RP...WP-1] is used portion of the buffer, [WP...RP-1] is free portion of the buffer
		// Both pointers are running freely, not wrapping over buffer length,
		// so to convert them to buffer indices, one have take them by modulo of buffer size.

		/// Ring buffer container
		template < class T > class RingBuffer
		{
		public:
			T *m_Buf;							//!< pointer to underlying buffer
			size_t m_BufSize;					//!< buffer size in records of type T
												// we do not use STL for faster access in Debug configurations
			std::atomic< size_t > m_BufWP;		//!< buffer current write position (free-running)
			std::atomic< size_t > m_BufRP;		//!< buffer current read position (free-running)

												/// Constructor
			RingBuffer( size_t new_size )
			{
				m_BufSize = new_size;
				m_Buf = (T* )calloc( new_size, sizeof( T ) );	// fill with zeros
				m_BufWP = 0;
				m_BufRP = 0;
			}

			/// Destructor
			~RingBuffer()
			{
				if( m_Buf )
					free( m_Buf );
			}
		};

		/// String pool ring buffer
		class StringPool: public RingBuffer< char >
		{
			std::atomic_bool m_lock;

		public:
			/// Constructor
			StringPool( size_t new_size ): m_lock( false ), RingBuffer< char >( new_size ){}

			/// Put given string into circular buffer at given position
			/// properly handling wrapping over end
			/// \param[in] s - string to copy
			/// \param[in] slen - string length including trailing zero
			/// \param[in] pos - desired position in buffer (may be unwrapped)
			/// \return pointer to string in buffer
			const char *put( const char *s, size_t slen, size_t pos )
			{
				pos %= m_BufSize;
				if( slen <= m_BufSize - pos )
					memcpy( m_Buf + pos, s, slen );
				else
				{
					const size_t part_len = m_BufSize - pos;
					memcpy( m_Buf + pos, s, part_len );
					memcpy( m_Buf, s + part_len, slen - part_len );
				}
				return m_Buf + pos;
			}

			/// Get string from circular buffer at given position
			/// properly handling wrapping over end
			/// \param[in] ptr - pointer to the string beginning in the buffer
			/// \return string object
			std::string get( const char *ptr )
			{
				if( ptr < m_Buf || ptr >= m_Buf + m_BufSize )
					return "";	// out of range

				const size_t pos = ptr - m_Buf;
				const size_t till_end = m_BufSize - pos;
				const size_t part_len = strnlen( m_Buf + pos, till_end );
				if( part_len < till_end )
					return std::string( m_Buf + pos );

				std::string ret( m_Buf + pos, till_end );
				ret += m_Buf;
				return ret;
			}

			/// Lock pool
			inline void lock()
			{
				bool expected = false;
				while( !m_lock.compare_exchange_strong( expected, true ) )
					expected = false;
			}

			/// Unlock pool
			inline void unlock()
			{
				m_lock = false;
			}
		};

		// trace records buffer: arranged as circular buffer with free-running pointers
		RingBuffer< TraceRec > m_traceBuf;		//!< trace buffer
		StringPool m_stringPool;				//!< string pool

		std::thread m_thread;					//!< worker thread to periodically print the buffer
		std::condition_variable m_thread_cv;	//!< condition variable to wake up worker thread
		std::mutex m_thread_mutex;				//!< worker thread mutex
		std::atomic_bool m_poison;				//!< poison pill: flag to worker thread to exit
		std::atomic_bool m_do_flush;			//!< flush flag
		std::atomic_bool m_do_restart;			//!< restart flag

		std::ostream *m_outStream;				//!< pointer to active output stream object to print trace into
		std::ofstream m_outFileStream;			//!< file stream object to print trace into
		std::string m_outFileName;				//!< filename of that stream
		bool m_isOwnStream;						//!< 'is stream object owned by tracing facility' flag

		/// trace statistics
		struct TraceStats
		{
			long long total_duration_ns;		//!< total duration of all section instances, ns
			long long min_duration_ns;			//!< min duration among all section instances, ns
			long long max_duration_ns;			//!< max duration among all section instances, ns
			size_t count;						//!< # of section instances
		};

		std::map< const char *, TraceStats >
			m_trace_stats;						//!< trace statistics map: accumulated times and counts for each stop trace point

		/// Check if the worker thread is started and start it, if it was not started yet
		void ensureThreadRuns()
		{
			if( !m_thread.joinable() )
			{
				try
				{
					std::unique_lock< std::mutex > lk( m_thread_mutex );
					// start worker thread
					m_thread = std::thread( workerThreadFunc, this );
					// wait for start
					m_thread_cv.wait_for( lk, std::chrono::milliseconds( 1000 ) );
				}
				catch( ... )  // ignore all errors
				{}
			}
		}

		/// Worker thread function: prints accumulated trace buffer contents to trace file
		/// \param[in] me - pointer to parent TracingFacility object
		static void workerThreadFunc( TracingFacility *me )
		{
			// Trace-related state of a single thread
			struct ThreadState
			{
				char thread_label[3];			//!< printable thread label like AA (0), AB (1)
				std::vector< TraceRec > stack;	//!< stack of start-type records belonging to this thread
				long long prev_timestamp_ns;	//!< previous trace record time stamp (to calculate delta time)

				// Constructor
				ThreadState( size_t idx ): prev_timestamp_ns( -1 )
				{
					// generate thread label:
					// min label is "AA" = 0, max. label is "ZZ" = 675
					const size_t alphabet_size = ('Z' - 'A' + 1);
					thread_label[ 2 ] = 0;
					thread_label[ 1 ] = 'A' + idx % alphabet_size;
					idx /= alphabet_size;
					thread_label[ 0 ] = 'A' + idx % alphabet_size;
				}
			};

			std::map< std::thread::id, ThreadState > tread_map;	// map of thread IDs to thread state objects
			long long start_time_ns = -1;	// starting time stamp

											// printing lambda
											// prints records from rp to wp
											// returns indices of first not processed records in trace buffer and string pool (may be NOT wp!)
			auto tracePrintBuf = [&]( size_t rp, size_t wp, size_t wp_pool, size_t rp_pool ) -> std::tuple< size_t, size_t >
			{
				size_t lri = rp;		// linear (not wrapped) record index
				size_t lspp = rp_pool;	// linear (not wrapped) string pool position

				for( ; lri < wp; lri++ )
				{
					const size_t ri = lri % me->m_traceBuf.m_BufSize;	// ri - true record index
					auto &rec = me->m_traceBuf.m_Buf[ ri ];
					std::string pool_string;
					const char *message = rec.m_message;

					if( rec.m_type == TraceType::Invalid )
						// current record is in process of update from another thread:
						// mean we reached the end of filled part, just break
						break;

					// process string pool entry
					if( rec.m_Flags & TraceRec::InStringPool )
					{
						pool_string = me->m_stringPool.get( message );
						message = pool_string.c_str();
						// increment processed string pool position
						lspp += pool_string.length() + 1 /*trailing zero*/;						
					}

					// get printable thread index
					auto map_it = tread_map.find( rec.m_threadID );
					if( map_it == tread_map.end() )
						// new thread ID, not in map yet: add to map
						std::tie( map_it, std::ignore ) = tread_map.insert(
							{
								rec.m_threadID,
								ThreadState( tread_map.size() )
							} );
					auto &thread_state = map_it->second;

					if( start_time_ns < 0 )
						// starting time stamp not assigned: assign
						start_time_ns = std::chrono::duration_cast< std::chrono::nanoseconds >( rec.m_timeStamp.time_since_epoch() ).count();

					// get timestamp relative to start
					const long long timestamp_ns = std::chrono::duration_cast< std::chrono::nanoseconds >( rec.m_timeStamp.time_since_epoch() ).count()
						- start_time_ns;

					// get timestamp delta in respect to previous timestamp in the SAME thread
					const auto delta_ns = thread_state.prev_timestamp_ns >= 0 ? timestamp_ns - thread_state.prev_timestamp_ns : 0;
					thread_state.prev_timestamp_ns = timestamp_ns;

					// process stack of traces

					int indent_level = (int )thread_state.stack.size();	// nesting level of start/stop sections
					long long section_duration_ns = 0;					// start/stop section duration

					if( rec.m_type == TraceType::Start )
						thread_state.stack.push_back( rec );
					else if( rec.m_type == TraceType::Stop )
					{
						// remove matching start record from stack top, if any
						if( thread_state.stack.size() > 0 && rec.match( thread_state.stack.back() ) )
						{
							section_duration_ns = timestamp_ns - (std::chrono::duration_cast< std::chrono::nanoseconds >(
								thread_state.stack.back().m_timeStamp.time_since_epoch() ).count()
								- start_time_ns);
							thread_state.stack.pop_back();
							indent_level--;
						}
						else
							indent_level = -1;	// start/stop do not match: unknown indent

						if( g_TraceGroupsRegistry.m_TraceStatisticsEnable && indent_level >= 0 )
						{
							auto it = me->m_trace_stats.find( rec.m_traceName );
							if( it == me->m_trace_stats.end() )
								me->m_trace_stats[ rec.m_traceName ] = { section_duration_ns, section_duration_ns, section_duration_ns, 1 };
							else
							{
								it->second.total_duration_ns += section_duration_ns;
								it->second.min_duration_ns = std::min( it->second.min_duration_ns, section_duration_ns );
								it->second.max_duration_ns = std::max( it->second.max_duration_ns, section_duration_ns );
								it->second.count++;
							}
						}
					}


					// Trace file format
					/*
					Timestamp       Delta     ID     Type          Message              Start-stop
					in us           in us        Level Name                             Duration

					[           1.0 :     0.0] AA [5] / Trace Start (Trace message)
					[           2.0 :     1.0] AA [5]  / Trace 2 Start (Trace message)
					[           5.0 :     3.0] AA [5] - Trace Point (Trace message)
					[          10.0 :     5.0] AA [5]  \ Trace 2 Stop (Trace message) <-- 8 us
					[          11.0 :     1.0] AA [5] \ Trace Stop (Trace message)
					*/

					if( me->m_outStream->good() )
					{
						char sbuf[ DG_LOG_TRACE_BUF_SIZE ];

						const bool need_message = message != nullptr;
						const bool need_duration = rec.m_type == TraceType::Stop && indent_level >= 0;

						int printf_ret = snprintf( sbuf, sizeof sbuf,
							"%c%14.1f :%10.1f] %2s [%1u] %s%*s %s%s%s\n",
							(rec.m_Flags & TraceRec::TimingDistorted) ? '*' : '[',
							timestamp_ns * 1e-3,
							delta_ns * 1e-3,
							thread_state.thread_label,
							rec.m_level,
							indent_level < 0 ? "?" : "",
							indent_level < 0 ? 1 : indent_level + 1,
							rec.m_type == TraceType::Start ? "/" : (rec.m_type == TraceType::Stop ? "\\" : "-"),
							rec.m_traceName,
							need_message ? ": " : "",
							need_message ? message : ""
						);

						if( printf_ret > 0 && printf_ret < sizeof sbuf - 1 && need_duration )
						{
							printf_ret--;	// to overwrite trailing \n
							int printf_ret2 = snprintf( sbuf + printf_ret, sizeof sbuf - printf_ret,
								"  <-- %.1f usec\n", section_duration_ns * 1e-3 );
							if( printf_ret2 > 0 )
								printf_ret += printf_ret2;
							else
								sbuf[ printf_ret++ ] = '\n';
						}

						if( printf_ret > 0 )
							// note: '<<' stream operators are VERY slow even in release build;
							// using them for printing to stream degrades performance ten-fold
							me->m_outStream->write( sbuf, std::min( sizeof( sbuf ) - 1, (size_t )printf_ret ) );
					}

					rec.m_type = TraceType::Invalid; // clear record
				}
				if( me->m_do_flush )
				{
					if( me->m_outStream->good() )
						me->m_outStream->flush();
					me->m_do_flush = false;
				}
				return std::make_tuple( lri, lspp );
			};


			std::unique_lock< std::mutex > lk( me->m_thread_mutex );
			me->m_thread_cv.notify_one();	// notify ensureThreadRuns() that the thread is started

			for(;;)
			{
				// sleep on CV
				auto wait_ret = me->m_thread_cv.wait_for( lk, std::chrono::milliseconds( 1000 ) );
				// here we own m_thread_mutex

				//
				// process records in buffer, if any
				//

				// cache shared vars:
				const size_t wp_pool = me->m_stringPool.m_BufWP;	// string pool first (it is OK to release not whole pool)
				const size_t rp_pool = me->m_stringPool.m_BufRP;
				const size_t wp = me->m_traceBuf.m_BufWP;			// then trace buffer
				const size_t rp = me->m_traceBuf.m_BufRP;
				if( wp > rp || me->m_do_restart || me->m_do_flush )
				{
					me->ownStreamCheckOpen(); // open file stream if it is owned by facility and not opened yet or restart it if requested

					size_t first_non_processed_trace;
					size_t first_non_processed_string;
					std::tie( first_non_processed_trace, first_non_processed_string ) = tracePrintBuf( rp, wp, wp_pool, rp_pool );

					// reclaim processed records
					me->m_stringPool.m_BufRP = first_non_processed_string;	// string pool first
																			// (so when trace buffer is released, pool is ready)
					me->m_traceBuf.m_BufRP = first_non_processed_trace;		// only then trace buffer
				}

				if( me->m_poison )	// request to terminate
					break;
			}

			me->ownStreamClose();	// close file stream if it is owned by facility and not closed yet
		}

		/// Print statistics into stream
		void printStatistics()
		{
			if( m_outStream->good() )
			{
				// print statistics
				if( g_TraceGroupsRegistry.m_TraceStatisticsEnable )
				{
					*m_outStream << "\n--------------Statistics--------------\n\n";
					*m_outStream << std::setprecision( 1 ) << std::fixed;
					for( const auto &v : m_trace_stats )
					{
						*m_outStream << v.first << " = [" << 1e-3 * v.second.min_duration_ns << " < "
										<< ( 1e-3 * v.second.total_duration_ns ) / v.second.count << "/" << v.second.count << " < "
										<< 1e-3 * v.second.max_duration_ns << "] usec\n";
					}
					m_trace_stats.clear();
				}
			}
		}

		/// Open file stream if it is owned by facility and not opened yet
		inline void ownStreamCheckOpen();

		/// Close file stream if it is owned by facility and not closed yet, printing statistics and footer
		void ownStreamClose()
		{
			printStatistics();

			if( m_outFileStream.is_open() )
			{
				if( m_outFileStream.good() )
				{
					m_outFileStream << "\nFinished: " << DG::TimeHelper::curStringTime() << '\n';
					m_outFileStream << "\n--------------end of trace--------------\n";
				}
				m_outFileStream.close();
			}
		}
	};

	/// Define global variable to store tracing facility singleton with weak linkage.
	/// Weak linkage is needed so this variable will be linked only once when this file is
	/// included in multiple .cpp files
	inline TracingFacility g_TracingFacility;

	/// RAII-style tracer class, which issues starting trace on construction and ending trace on destruction
	class Tracer
	{
		TraceLevel_t *m_group;				//!< pointer to trace group variable
		const char *m_name;					//!< trace name
		TraceLevel_t m_level;				//!< trace level
		std::ostringstream m_stream;		//!< buffering stream to implement << operator
		TracingFacility *m_tracingFacility;	//!< tracing facility object to use for tracing

	public:

		/// Constructor
		/// Issues starting point
		/// \param[in] tracingFacility - tracing facility object to use for tracing
		/// \param[in] group - trace group
		/// \param[in] name - trace name
		/// \param[in] level - tracing level
		/// \param[in] message - printf-like message format string
		Tracer( TracingFacility *tracingFacility,
			TraceLevel_t *group, const char *name, TraceLevel_t level, const char *message = nullptr, ... ):
			m_tracingFacility( tracingFacility ),
			m_group( group ),
			m_name( name ),
			m_level( level )
		{
			if( m_level <= *m_group )
			{
				if( message != nullptr )
				{
					va_list args;
					va_start( args, message );
					m_tracingFacility->traceVPrintfDo( TracingFacility::TraceType::Start, name, level, message, args );
					va_end( args );
				}
				else
					m_tracingFacility->traceDo( TracingFacility::TraceType::Start, name, level, nullptr );
			}
		}

		/// Destructor
		/// Issues ending point
		~Tracer()
		{
			if( m_level <= *m_group )
				m_tracingFacility->traceDo( TracingFacility::TraceType::Stop, m_name, m_level, nullptr );
		}

		/// Issue trace
		/// \param[in] type - trace type (start/stop/point)
		/// \param[in] message - printf-like message format string
		void Trace( TracingFacility::TraceType type, const char *message, ... )
		{
			if( m_level <= *m_group )
			{
				va_list args;
				va_start( args, message );
				m_tracingFacility->traceVPrintfDo( type, m_name, m_level, message, args );
				va_end( args );
			}
		}

		/// Stream-style operator
		/// It adds value to the internal stream.
		/// Note: it is slow! Do not use it in performance critical code!
		template<typename T> Tracer& operator << ( const T& value )
		{
			if( m_level <= *m_group )
				m_stream << value;
			return *this;
		}

		/// Stream is traced as trace point when '\n' is received
		Tracer& operator << ( const char& value )
		{
			if( m_level <= *m_group )
			{
				if( value == '\n' )
				{
					Trace( TracingFacility::TraceType::Point, m_stream.str().c_str() );
					m_stream.str( "" );
					m_stream.clear();
				}
				else
					m_stream << value;
			}
			return *this;
		}
	};

}	// namespace DGTrace


#include "dg_file_utilities.h"

//
// Get path to DG temporary directory
//
inline std::string DGTrace::TraceGroupsRegistry::getTempPath()
{
	return DG::FileHelper::appdata_dg_dir() + "traces/";
}


//
// Open file stream if it is owned by facility and not opened yet
//
inline void DGTrace::TracingFacility::ownStreamCheckOpen()
{
	if( m_isOwnStream && ( !m_outFileStream.is_open() || m_do_restart ) )
	{
		// get full trace file path while preserving previous trace file content into .bak file
		m_outFileName = DG::FileHelper::notUsedFileInDirBackupAndGet( DGTrace::TraceGroupsRegistry::getTempPath(), DG_TRC_TRACE_FILE );

		if( m_outFileStream.is_open() )
			ownStreamClose();

		m_outFileStream.open( m_outFileName.c_str(), std::ios_base::out | std::ios_base::trunc );

		if( m_outFileStream.good() )
		{
			DG::FileHelper::lockFileStreamUnderlyingFileHandle( m_outFileStream );
			g_TraceGroupsRegistry.printHeader( m_outFileStream );
		}

		m_do_restart = false;
	}
}


//
// Read and return current trace file contents
// [in] offset - start read offset in file
// [in] size - size of data to read, -1 to read whole file
//
inline std::string DGTrace::TracingFacility::readTraceFile( size_t offset, size_t size )
{
	flush( true );
	if( m_isOwnStream && m_outFileStream.is_open() )
	{
		std::ifstream fin( m_outFileName.c_str(), std::ios::in | std::ios::binary );
		if( fin.good() )
		{
			if( offset > 0 )
				fin.seekg( offset );

			if( size == -1 )
				return std::string( std::istreambuf_iterator< char >( fin ), std::istreambuf_iterator< char >() );

			std::string ret( size, '\0' );
			fin.read( ret.data(), size );
			ret.resize( fin.gcount() );
			return ret;
		}
	}	
	return "";
}


#endif	// DG_TRACING_FACILITY_H
