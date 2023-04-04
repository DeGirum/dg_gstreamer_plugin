//////////////////////////////////////////////////////////////////////
/// \file  DGLog.h
/// \brief DG logging facility
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declarations of logging facility functionality.
/// Logging facility is intended to be used for rare events, when
/// the speed of logging is not relevant, but log information needs to
/// be preserved even in case of app crash.
/// For high-efficient tracing, use trace facility (Utilities/DGTrace.h)

#ifndef DG_LOG_H
#define DG_LOG_H


/// Write printf-like message to system log file
#define DG_LOG_PRINTF( msg, ...)	DG::FileLogger::instance.log( msg, ##__VA_ARGS__ )


/// Write std::string message to system log file
#define DG_LOG_PRINTS( msg )		DG::FileLogger::instance.log( "%s", msg )

/// Write string message to system log file with newline ending
#define DG_LOG_PUTS( msg )		DG::FileLogger::instance.log( "%s\n", msg )

/// Clear log file
#define DG_LOG_CLEAR()				DG::FileLogger::instance.clear()



/// Legacy macros (DO NOT USE IN NEW CODE)

/// Define log object and log provided header string
#define DG_LOG( header )			DGLog oLog( header )

/// Define log object and start new log file
#define DG_LOG_RESET()				DGLog oLog( "START TESTING", true )

/// Set 'Failed' status
#define DG_LOG_SETFAILED()			oLog.SetFailed()

/// Set 'Passed' status
#define DG_LOG_SETPASSED()			oLog.SetPassed()

/// Log comment
#define DG_COMMENT( a )				do{ oLog << (a); } while( 0 )

/// Log comment and set 'Passed' status 
#define DG_PASS_COMMENT( a )		do{ oLog.SetPassed(); oLog << (a); } while( 0 )

/// Log comment(s) and set 'Failed' status 
#define DG_FAIL_COMMENT( a )			do{ oLog.SetFailed(); oLog << (a); } while( 0 )
#define DG_FAIL_COMMENT2( a, b )		do{ oLog.SetFailed(); oLog << (a) << " " << (b); } while( 0 )
#define DG_FAIL_COMMENT3( a, b, c )		do{ oLog.SetFailed(); oLog << (a) << " " << (b) << " " << (c); } while( 0 )
#define DG_FAIL_COMMENT4( a, b, c, d )	do{ oLog.SetFailed(); oLog << (a) << " " << (b) << " " << (c) << " " << (d); } while( 0 )


//////////////////////////////////////////////////////////////////////
// Internal implementation. Try not to use directly.


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <iomanip>
#include <mutex>
#include <filesystem>
#include <stdarg.h>
#include "dg_com_decl.h"
#include "dg_time_utilities.h"

namespace DG
{

	/// Simple thread-safe, file-based, non-buffering, always flushing logger class
	class FileLogger
	{
	public:

		/// Constructor
		/// \param[in] log_fname - log filename suffix
		FileLogger( const std::string &log_fname = "dg_log.txt" ) : m_is_initialized( false ), m_fname_suffix( log_fname )
		{}

		/// Clear log file
		inline bool clear();

		/// Implementation of formatted logging
		/// \param[in] fmt - printf-like format string
		/// \param[in] ... - variable-length list of items to log according to format
		/// \return false in case of file failure
		bool _log( const char *fmt, ... )
		{
			// lock mutex
			std::lock_guard< std::recursive_mutex > lk( m_mx );

			// initialize on first access
			if( !m_is_initialized )
				clear();

			if( !m_file.is_open() || !m_file.good() )
				return false;

			// print message
			char msg_buf[ DG_LOG_TRACE_BUF_SIZE ];
			msg_buf[ sizeof msg_buf - 1 ] = '\0';
			va_list args;
			va_start( args, fmt );
			const size_t msg_len = std::min(
				(size_t )vsnprintf( msg_buf, sizeof msg_buf, fmt, args ),
				sizeof msg_buf );
			va_end( args );

			// write message
			if( msg_len > 0 )
			{
				m_file.write( msg_buf, msg_len );
				m_file.flush();
			}
			
			return msg_len > 0 && m_file.good();
		}

		/// Log formatted args to log file
		/// \param[in] fmt - printf-like format string
		/// \param[in] args... - variable-length list of items to log according to format
		/// \return false in case of file failure
    template<typename... Args>
		bool log( const char *fmt, const Args& ...args )
		{
      const auto unwrap_std_strings = [](auto arg) constexpr {
        if constexpr(std::is_same_v<decltype(arg), std::string>)
          return arg.c_str();
        else
          return arg;
      };
			return _log(fmt, unwrap_std_strings(args)...);
		}

		/// System logger singleton
		static FileLogger instance;

	private:
		std::recursive_mutex m_mx;	//!< thread protecting mutex
		std::string m_fname_suffix;	//!< log filename suffix
		std::string m_fname;		//!< log filename
		std::ofstream m_file;		//!< log file stream
		bool m_is_initialized;		//!< log is initialized flag
	};

	/// System logger singleton
	inline FileLogger FileLogger::instance;

}	// namespace DG



/// Legacy logger
class DGLog
{
public:
	/// Constructor
	/// \param[in] header - log header to print in the beginning of the log
	/// \param[in] reset - true to start log with clean file
	explicit DGLog( const char *header, bool reset = false ):
		m_header( header ),
		m_start_time( 0. ),
		m_timestamp( 0 ),
		m_status( -1 )
	{
		// reset log, record and print log reset timestamp
		if( reset )
		{
			time( &m_timestamp );
			DG_LOG_CLEAR();
		}

		// record start time
		m_start_time = get_epoch_time_s();
	}

	/// Destructor
	virtual ~DGLog()
	{
		if( !m_header.empty() || m_status == 0 )
		{
			const std::string log_message = DG_FORMAT(
				"[" << std::left << std::setw(45) << m_header << "]: " << 
				std::fixed << std::setw( 12 ) << ReportInterval() <<
				(m_status == 0 ? " FAILED " : (m_status == 1 ? " Passed " : " ")) <<
				m_ss.str() << '\n' );

			#ifndef NOT_VERBOSE
				std::string status_string;
				switch( m_status )
				{
				case 0:
					status_string = DG_FORMAT( "\x1B[31m" << std::setw( 12 ) << ReportInterval() << " FAILED " );
					break;
				case 1:
					status_string = DG_FORMAT( "\x1B[33m" << std::setw( 12 ) << ReportInterval() << " Passed " );
					break;
				default:
					status_string = DG_FORMAT( "\x1B[33m" << std::setw( 12 ) << ReportInterval() << " " );
					break;
				}
				const std::string cout_message = DG_FORMAT(
					"\r\x1B[33m" << "[" << std::left << std::setw(45) << m_header << "]:" << "\033[0m" <<
					" " << std::fixed << status_string << m_ss.str() << "\033[0m" << '\n' );
			#else
				const std::string cout_message = "\r" + log_message;
			#endif

			std::cout << cout_message;
			DG_LOG_PRINTS( log_message );
		}
	}

	const int64_t getTimestamp() const { return m_timestamp; }

	/// Set 'Passed' status
	void SetPassed() { m_status = 1; }

	/// Set 'Failed' status
	void SetFailed() { m_status = 0; }

	/// Add given value to comment buffer. To flush this buffer use commentFlush()
	template< typename T > DGLog& operator << ( const T &value )
	{
		m_ss << value;
		return *this;
	}

	/// Return time in seconds passed since creation of log object
	double ReportInterval() { return get_epoch_time_s() - m_start_time; }

private:
	std::string	m_header;			//!< cached log header
	std::stringstream m_ss;			//!< log comments buffer
	double m_start_time;			//!< log object creation time
	time_t m_timestamp;				//!< timestamp of log file reset
	int m_status;					//!< runtime status: 0 - Failed, 1 - Passed, -1 - unknown
};


#include "dg_file_utilities.h"


// Clear log file (implementation)
inline bool DG::FileLogger::clear()
{
	if( m_is_initialized && m_file.is_open() )
		m_file.close();

	m_fname = DG::FileHelper::notUsedFileInDirBackupAndGet( DG::FileHelper::appdata_dg_dir() + "traces/", m_fname_suffix );
	m_file.open( m_fname, std::ios_base::out | std::ios_base::trunc );
	m_is_initialized = true;
	const bool ret = m_file.is_open() && m_file.good();
	if( ret )
	{
		DG::FileHelper::lockFileStreamUnderlyingFileHandle( m_file );
		log( "Started: %s ----------------------------------------\n",
			 TimeHelper::curStringTime().c_str() );
	}
	return ret;
}

#endif	// DG_LOG_H
