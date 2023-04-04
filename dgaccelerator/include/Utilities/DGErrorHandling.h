//////////////////////////////////////////////////////////////////////
/// \file  DGErrorHandling.h
/// \brief DG error handling facility
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declaration related to error handling facility:
/// DG exception type, error handling macros, global error codes
///


#ifndef DG_ERROR_HANDLING_H
#define DG_ERROR_HANDLING_H

/// List of all global error codes
#define DG_ERROR_LIST \
	_( ErrSuccess,				0x00000000, "Success" ) \
	_( Success,					0x00000000, "Success" ) \
	_( ErrNotImplemented,		0x00000001, "Functionality is not implemented" ) \
	_( ErrNotSupported,			0x00000002, "Functionality is not supported" ) \
	_( ErrNotInitialized,		0x00000003, "Subsystem is not initialized" ) \
	_( ErrBadParameter,			0x00000004, "Incorrect value of parameter" ) \
	_( ErrTimeout,				0x00000005, "Timeout detected" ) \
	_( ErrInsufficientMemory,	0x00000006, "Insufficient memory" ) \
	_( ErrOperationFailed,		0x00000007, "Operation failed" ) \
	_( ErrInvalidAddress,		0x00000008, "Invalid address" ) \
	_( ErrBadValue,				0x00000009, "Incorrect value" ) \
	_( ErrResourceError,		0x0000000A, "Resource access error" ) \
	_( ErrOverFlow,				0x0000000B, "Overflow is detected" ) \
	_( ErrFileNotFound,			0x0000000C, "File not found" ) \
	_( ErrDeviceNotFound,		0x0000000D, "Device not found" ) \
	_( ErrNullPointer,			0x0000000E, "Null pointer passed" ) \
	_( ErrInconsistentData,		0x0000000F, "Inconsistent data structure" ) \
	_( ErrLoadModel,			0x00000010, "Loading model failed" ) \
	_( ErrFileWriteFailed,		0x00000011, "File writing failure" ) \
	_( ErrFileReadFailed,		0x00000012, "File reading failure" ) \
	_( ErrFileOperationFailed,	0x00000013, "File operation failed" ) \
	_( ErrParseError,			0x00000014, "Parsing error" ) \
	_( ErrDirNotFound,			0x00000015, "Directory not found" ) \
	_( ErrDeviceAccess,			0x00000016, "Device access error" ) \
	_( ErrDeviceBusy,			0x00000017, "Device is busy" ) \
	_( ErrNotSupportedVersion,	0x00000018, "Version is not supported" ) \
	_( ErrCompilerBadState,		0x00000019, "Failure in compiler stage" ) \
	_( ErrFailedUserConstraints,0x0000001A, "Failed to satisfy user-specified constraints" ) \
	\
	_( ErrContinue,				0x00001000, "<continued>" ) \
	_( ErrAssert,				0x00001001, "Execution failed" ) \
	_( ErrSystem,				0x00010000, "OS error" ) \


/// List of all error types
#define DG_ERROR_TYPE_LIST \
	_( WARNING,				"[WARNING]",	"warning: no exception is thrown, but error is registered in global error collection" ) \
	_( VALIDATION_ERROR,	"[VALIDATION]",	"validation error: exception is thrown, but error is NOT registered in global error collection" ) \
	_( RUNTIME_ERROR,		"[ERROR]",		"regular runtime error: exception is thrown, and error is registered in global error collection" ) \
	_( CRITICAL_ERROR,		"[CRITICAL]",	"critical error: like RUNTIME_ERROR, but also sets global critical error flag in global error collection" ) \
	_( EXIT_ERROR,			"[EXIT]",		"exit error: like CRITICAL_ERROR, but with another label" ) \


namespace DG
{
	/// Global error codes
	enum DGErrorID
	{
		#define _( id, code, desc ) id,
		DG_ERROR_LIST
		#undef _
	};

	/// Error types
	enum class ErrorType
	{
		#define _( id, label, desc ) id,
		DG_ERROR_TYPE_LIST
		#undef _
	};
};

//
// Error reporting macros
//

/// Report warning, do not throw
#define DG_WARNING( err_msg, err_code )		do { DG::ErrorHandling::warningAdd( __FILE__, TOSTRING(__LINE__), FUNCTION_NAME, DG::err_code, err_msg ); } while( 0 )

/// Report and throw runtime error
#define DG_ERROR( err_msg, err_code )		do { DG::ErrorHandling::errorAdd( __FILE__, TOSTRING(__LINE__), FUNCTION_NAME, \
	DG::ErrorType::RUNTIME_ERROR, DG::err_code, err_msg ); } while( 0 )

/// Throw validation error, do not report
#define DG_VALIDATION_ERROR( err_msg )		do { DG::ErrorHandling::errorAdd( __FILE__, TOSTRING(__LINE__), FUNCTION_NAME, \
	DG::ErrorType::VALIDATION_ERROR, DG::ErrCompilerBadState, err_msg ); } while( 0 )

/// Report and throw exit error, set critical flag
#define DG_EXIT_ERROR( err_msg, err_code )	do { DG::ErrorHandling::errorAdd( __FILE__, TOSTRING(__LINE__), FUNCTION_NAME, \
	DG::ErrorType::EXIT_ERROR, DG::err_code, err_msg ); } while( 0 )

/// Report and throw critical error, set critical flag
#define DG_CRITICAL_ERROR( err_msg, err_code )	do { DG::ErrorHandling::errorAdd( __FILE__, TOSTRING(__LINE__), FUNCTION_NAME, \
	DG::ErrorType::CRITICAL_ERROR, DG::err_code, err_msg ); } while( 0 )

/// Add a message to given exception object and throw combined exception
#define DG_ERROR_COMMENT( msg, e )	do { DG::ErrorHandling::errorAdd( __FILE__, TOSTRING(__LINE__), FUNCTION_NAME, \
	DG::ErrorType::RUNTIME_ERROR, DG::ErrContinue, msg, e.what() ); } while( 0 )

/// Execute expression, catch any thrown std::exception, combine it with given message and rethrow DG as exception
/// Useful to add upper-level comments to exceptions generated on lower-levels
#define DG_RETHROW( expr, msg )	do { try { expr; } catch( std::exception &e ) { DG_ERROR_COMMENT( msg, e ); } } while( 0 )



//
// Error handling macros
//

/// Print all registered errors to console
#define DG_ERROR_PRINT()				DG::ErrorHandling::errorsPrint()

/// Print all registered errors to given stream object
#define DG_ERROR_STREAM( strm )			DG::ErrorHandling::errorsPrint( strm, "" )

/// Clean all registered errors
#define DG_ERROR_CLEANUP()				DG::ErrorHandling::clear()

/// Check, if given exception object contains DG critical error
#define DG_IF_ERROR_CRITICAL( e )		DG::errorTypeCheck( e, DG::ErrorType::CRITICAL_ERROR )

/// Check, if given exception object contains DG runtime error
#define DG_IF_ERROR_RUNTIME( e )		DG::errorTypeCheck( e, DG::ErrorType::RUNTIME_ERROR )

/// Check, if given exception object contains DG validation error
#define DG_IF_ERROR_VALIDATION( e )		DG::errorTypeCheck( e, DG::ErrorType::VALIDATION_ERROR )


/// Assertion macro
#ifdef NDEBUG
	#define DG_ASSERT( expr )			( (void ) 0 )
#else
	#define DG_ASSERT( expr )			DG::ErrorHandling::assertHandle( __FILE__, TOSTRING(__LINE__), FUNCTION_NAME, #expr, expr )
#endif

#define DG_ERROR_TRUE( expr )		DG_CHECK_HELPER_BOOL_( ( expr ), #expr, false, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_FALSE( expr )		DG_CHECK_HELPER_BOOL_( !( expr ), #expr, true, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_EQ( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, EQ, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_NE( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, NE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_LT( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, LT, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_GT( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, GT, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_LE( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, LE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_GE( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, GE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )

#define DG_ASSERT_TRUE( expr )		 DG_CHECK_HELPER_BOOL_( ( expr ), #expr, false, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )
#define DG_ASSERT_FALSE( expr )		 DG_CHECK_HELPER_BOOL_( !( expr ), #expr, true, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )
#define DG_ASSERT_EQ( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, EQ, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )
#define DG_ASSERT_NE( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, NE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )
#define DG_ASSERT_LT( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, LT, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )
#define DG_ASSERT_GT( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, GT, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )
#define DG_ASSERT_LE( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, LE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )
#define DG_ASSERT_GE( expr1, expr2 ) DG_CHECK_HELPER_2_( expr1, expr2, GE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::assertHandleCallAdapter )

#define DG_ERROR_RANGE_STRICT( expr1, expr2, expr3 ) \
	DG_CHECK_HELPER_3_( expr1, expr2, expr3, LT, LT, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_RANGE_LB_STRICT( expr1, expr2, expr3 ) \
	DG_CHECK_HELPER_3_( expr1, expr2, expr3, LT, LE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_RANGE_UB_STRICT( expr1, expr2, expr3 ) \
	DG_CHECK_HELPER_3_( expr1, expr2, expr3, LE, LT, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )
#define DG_ERROR_RANGE( expr1, expr2, expr3 ) \
	DG_CHECK_HELPER_3_( expr1, expr2, expr3, LE, LE, DG::ErrorType::RUNTIME_ERROR, ErrAssert, DG::ErrorHandling::errorAdd )

//////////////////////////////////////////////////////////////////////
// Internal implementation. Try not to use directly.


#include "dg_com_decl.h"
#include <exception>
#include <string>
#include <cstring>
#include <iostream>
#include <sstream>
#include <deque>
#include <mutex>
#include <filesystem>
#include <array>


/// DG exception class: used for all exceptions in this error facility
class DGException: public std::exception
{
public:

	/// Constructor
	/// \param[in] message - error message string
	/// \param[in] inclSysMsg - true to append system error message string taken from errno
	DGException( const std::string &message, bool inclSysMsg = false ): m_err_msg( message )
	{
		if( inclSysMsg )
		{
		#ifdef _MSC_VER
			char buf[ 256 ];
			strerror_s( buf, sizeof buf, errno );
		#else
			const char *buf = std::strerror( errno );
		#endif
			m_err_msg += " : " + std::string( buf );
		}
	}

	/// Destructor
	virtual ~DGException()
	{}

	/// what() override: return error message
	virtual const char *what() const noexcept override
	{
		return m_err_msg.c_str();
	}

private:
	std::string m_err_msg;	//!< error message accessible by what()
};


namespace DG
{

	/// Error handling facility class
	class ErrorHandling
	{
	public:

		/// Register error
		/// \param[in] file - file where error happens
		/// \param[in] line - line in file where error happens
		/// \param[in] func - function where error happens
		/// \param[in] type - error type
		/// \param[in] err_code - error code
		/// \param[in] msg - error message
		/// \param[in] comment - optional error comment; will prepend error message
		[[noreturn]] inline static void errorAdd( const char *file, const char *line, const char *func,
			ErrorType type, DGErrorID err_code, const std::string& msg, const std::string& comment = {} );

		/// Register warning (do not throw exception)
		/// \param[in] file - file where error happens
		/// \param[in] line - line in file where error happens
		/// \param[in] func - function where error happens
		/// \param[in] err_code - error code
		/// \param[in] msg - error message
		/// \param[in] comment - optional error comment; will prepend error message
		static void warningAdd( const char *file, const char *line, const char *func,
			DGErrorID err_code, const std::string& msg, const std::string& comment = {} )
		{
			try
			{
				errorAdd( file, line, func, ErrorType::WARNING, err_code, msg, comment );
			}
			catch( ... )
			{}
		}

		/// Print all registered errors into given stream
		/// \param[in] strm - stream to print critical errors into, but when err_file_path is empty, all errors will be printed
		/// \param[in] err_file_path - file to print all error into; can be empty
		/// \return true if there are any critical error registered
		inline static bool errorsPrint( std::ostream &strm = std::cout, const std::string err_file_path = "profile.txt" );

		/// Clear collection of registered errors
		static void clear()
		{
			m_error_collection.clear();
		}

		/// Check, if there are any critical errors registered
		static bool isErrorCritical()
		{
			return m_error_collection.m_most_severe_error_type >= ErrorType::CRITICAL_ERROR;
		}

		/// Get # of registered error (for unit tests only)
		static size_t errorCount() { return m_error_collection.m_err_deque.size(); }

		/// Handle assertion
		/// \param[in] file - file where assertion happens
		/// \param[in] line - line in file where assertion happens
		/// \param[in] func - function where assertion happens
		/// \param[in] expr_str - expression string
		/// \param[in] expr - logical expression to check, if false, do assertion actions
		inline static void assertHandle( const char *file, const char *line, const char *func, const char *expr_str, bool expr );

		/// Format location string
		/// \param[in] file - file where assertion happens
		/// \param[in] line - line in file where assertion happens
		/// \param[in] func - function where assertion happens
		inline static std::string location2str( const char *file, const char *line, const char *func );

	private:

		/// Error record in error collection
		struct ErrorRecord
		{
			/// Constructor
			/// \param[in] err_msg - error message string
			/// \param[in] err_type - error type
			/// \param[in] err_code - error code
			ErrorRecord( const std::string &err_msg, const ErrorType err_type, DGErrorID err_code ):
				m_err_msg( err_msg ), m_err_type( err_type ), m_err_code( err_code ){}

			std::string			m_err_msg;		//!< error message string
			DGErrorID			m_err_code;		//!< error code
			ErrorType			m_err_type;		//!< error type
		};

		/// Collection of error records
		struct ErrorCollection
		{
			std::mutex m_mx;						//!< thread protection mutex
			std::deque< ErrorRecord > m_err_deque;	//!< collection of registered errors
			ErrorType m_most_severe_error_type;		//!< most severe registered error type
			size_t m_max_size;						//!< maximum collection size

			/// Constructor
			explicit ErrorCollection( size_t max_size = 100 ):
				m_most_severe_error_type( ErrorType::WARNING ),
				m_max_size( max_size )
			{}

			/// Clear collection
			void clear()
			{
				std::lock_guard< std::mutex > lk( m_mx );
				m_most_severe_error_type = ErrorType::WARNING;
				m_err_deque.clear();
			}

			/// Add error record to collection
			/// \param[in] err_rec - error record to add
			void add( const ErrorRecord &err_rec )
			{
				std::lock_guard< std::mutex > lk( m_mx );
				if( m_err_deque.size() > m_max_size )
					m_err_deque.pop_front();
				m_err_deque.push_back( err_rec );
				if( m_most_severe_error_type < err_rec.m_err_type )
					m_most_severe_error_type = err_rec.m_err_type;
			}
		};

		/// Get string message corresponding to given error code
		static const char* code2str( DGErrorID err_code )
		{
			switch( err_code )
			{
				#define _( id, code, desc ) case id: return desc;
				DG_ERROR_LIST
				#undef _
			}
			return "";
		}

		/// Collect stack back-trace into a string
		/// \param[in] skip - how many frames to skip from stack top
		/// \param[in] depth - desired stack depth to capture
		inline static std::string stackTrace( size_t skip, size_t depth );

		static ErrorCollection m_error_collection;	//!< global collection of registered errors

public:
		/// Last error recorded
		inline static ErrorRecord lastErrorRecord()
		{
			return m_error_collection.m_err_deque.back();
		}
	};

	inline ErrorHandling::ErrorCollection ErrorHandling::m_error_collection;	//!< global collection of registered errors

	/// Check, if given exception object contains certain DG error type
	/// \param[in] e - exception object containing DG error
	/// \param[in] type - error type to check for
	/// \return True if exception object contains an error of given type
	static inline bool errorTypeCheck( const std::exception &e, DG::ErrorType type )
	{
		const char *type_str = nullptr;
		switch( type )
		{
			#define _( id, label, desc ) case DG::ErrorType::id: type_str = label; break;
			DG_ERROR_TYPE_LIST
			#undef _
		}

		if( type_str != nullptr )
			return std::strstr( e.what(), type_str ) != nullptr;
		return false;
	}


/// Assert message handler
	template< int N >
	class AssertMessage
	{
	public:
		/// contructor
		explicit AssertMessage() : m_args(), m_arg_counter()
		{}

		/// general operator<< implementation
		/// \tparam T argument type
		/// \param val argument value
		/// \return self
		template< typename T >
		inline AssertMessage &operator<<( const T &val )
		{
			m_out << val;
			return *this;
		}

		/// operator<< overload for pointer
		/// \tparam T argument type
		/// \param val argument value
		/// \return self
		template< typename T >
		inline AssertMessage &operator<<( T *const &pointer )
		{
			m_out << ( pointer != nullptr ? pointer : "(NULL)" );
			return *this;
		}

		/// operator<< overload for bool argument
		/// \param b boolean value
		/// \return self
		AssertMessage &operator<<( bool b )
		{
			m_out << ( b ? "true" : "false" );
			return *this;
		}

		/// operator<< overload ostream manipulators
		/// \param val manipulator value
		/// \return self
		AssertMessage &operator<<( std::ostream &( *val )(std::ostream &))
		{
			m_out << val;
			return *this;
		}

		/// specify argument name based on counter
		/// \param s argument name
		/// \return self
		AssertMessage &arg( const std::string &s )
		{
			if( m_arg_counter < m_args.size() )
				m_args[ m_arg_counter++ ] = s;
			return *this;
		}

		/// specify argument name for argument number pos
		/// \param s argument name
		/// \param pos argument position
		/// \return self
		AssertMessage &arg( const std::string &s, size_t pos )
		{
			if( pos < m_args.size() )
				m_args[ pos ] = s;
			return *this;
		}

		/// get argument values
		/// \return arguments array
		std::array< std::string, N > args() const
		{
			return m_args;
		}

		/// get accumulated message
		/// @return
		std::string str() const
		{
			return m_out.str();
		}

	private:
		std::array< std::string, N > m_args;  //!< assert message arguments storage
		std::ostringstream m_out;			  //!< string stream to store additional assert message (if used)
		int m_arg_counter;					  //!< automatic arguments counter, used to enumerate arguments during arg call
	};

	/// representation of the assert check result class
	/// stores assert check results and operands values
	template< int N >
	class AssertCheckResult
	{
	public:
		/// failed assertion check context: operands and operations values
		struct AssertionCheckContext
		{
			std::array< std::string, N - 1 > m_operation;						//!< operations values
			std::array< std::pair< std::string, std::string >, N > m_operands;	//!< operands values

			/// constructor
			AssertionCheckContext()
			{}

			/// constructor
			/// \param operation array of operations values
			/// \param operands array of operands value
			AssertionCheckContext(
				const std::array< std::string, N - 1 > &operation,
				const std::array< std::pair< std::string, std::string >, N > &operands ) :
				m_operation( operation ),
				m_operands( operands )
			{}
		};

		/// constructor
		/// \param status assertion check status
		explicit AssertCheckResult( bool status ) : m_status( status ), m_context( nullptr )
		{}

		/// constructor
		/// \param status assertion check status
		/// \param operation array of operations used during check
		/// \param operands array of operands value used during check
		AssertCheckResult(
			bool status,
			const std::array< std::string, N - 1 > &operation,
			const std::array< std::pair< std::string, std::string >, N > &operands ) :
			m_status( status ),
			m_context( nullptr )
		{
			m_context = new AssertionCheckContext( operation, operands );
		}

		/// remove default constructors/assignments
		AssertCheckResult( const AssertCheckResult &t ) = delete;
		AssertCheckResult( AssertCheckResult &&t ) = delete;
		AssertCheckResult &operator=( const AssertCheckResult &t ) = delete;
		AssertCheckResult &operator=( AssertCheckResult &&t ) = delete;

		/// destructor
		~AssertCheckResult()
		{
			if( m_context != nullptr )
			{
				delete m_context;
				m_context = nullptr;
			}
		}

		/// conversion to bool: get assertion check status
		operator bool() const
		{
			return m_status;
		}

		/// create a copy of current check context
		/// \return assertion check context
		AssertionCheckContext contextGet() const
		{
			return m_context == nullptr ? AssertionCheckContext() : AssertionCheckContext( *m_context );
		}

		/// bool-like assertion check implementation
		/// \tparam T expression type
		/// \param val expression value
		/// \param expr expression string representation
		/// \param actual actual value string representation
		/// \return assertion check result
		template< typename T >
		inline static AssertCheckResult< 1 > CompareHelperBool( const T &val, const char *expr, const char *actual )
		{
			if( val )
				return AssertCheckResult< 1 >( true );
			else
				return AssertCheckResult< 1 >( false, {}, { std::make_pair( expr, actual ) } );
		}

		/// two expression assertion check comparison implementations generator
		/// \tparam T1 first expression type
		/// \tparam T2 second expression type
		/// \param val1 first expression value
		/// \param val2 second expression value
		/// \param expr1 first expression string representation
		/// \param expr2 second expression string representation
		/// \return assertion check result
#define DG_DEFINE_COMPARISON_HELPER( op_name, op )                                                                                      \
	template< typename T1, typename T2 >                                                                                                \
	inline static AssertCheckResult< 2 > CompareHelper##op_name( const T1 &val1, const T2 &val2, const char *expr1, const char *expr2 ) \
	{                                                                                                                                   \
		if( val1 op val2 )                                                                                                              \
			return AssertCheckResult< 2 >( true );                                                                                      \
		else                                                                                                                            \
			return AssertCheckResult< 2 >(                                                                                              \
				false,                                                                                                                  \
				{ #op },                                                                                                                \
				{ std::make_pair( expr1, ( ( std::ostringstream& )( std::ostringstream() << val1 ) ).str() ),                           \
				  std::make_pair( expr2, ( ( std::ostringstream& )( std::ostringstream() << val2 ) ).str() ) } );                       \
	}

		DG_DEFINE_COMPARISON_HELPER( EQ, == )
		DG_DEFINE_COMPARISON_HELPER( NE, != )
		DG_DEFINE_COMPARISON_HELPER( LE, <= )
		DG_DEFINE_COMPARISON_HELPER( LT, < )
		DG_DEFINE_COMPARISON_HELPER( GE, >= )
		DG_DEFINE_COMPARISON_HELPER( GT, > )
#undef DG_DEFINE_COMPARISON_HELPER

#define DG_DEFINE_COMPARISON_HELPER_3( op_name1, op1, op_name2, op2 )                                                                                \
	template< typename T1, typename T2, typename T3 >                                                                                                \
	inline static AssertCheckResult< 3 >                                                                                                             \
		CompareHelper##op_name1##op_name2( const T1 &val1, const T2 &val2, const T3 &val3, const char *expr1, const char *expr2, const char *expr3 ) \
	{                                                                                                                                                \
		if( ( val2 op1 val1 ) && ( val1 op2 val3 ) )                                                                                                 \
			return AssertCheckResult< 3 >( true );                                                                                                   \
		else                                                                                                                                         \
			return AssertCheckResult< 3 >(                                                                                                           \
				false,                                                                                                                               \
				{ #op1, #op2 },                                                                                                                      \
				{ std::make_pair( expr1, ( ( std::ostringstream& )( std::ostringstream() << val1 ) ).str() ),                                        \
				  std::make_pair( expr2, ( ( std::ostringstream& )( std::ostringstream() << val2 ) ).str() ),                                        \
				  std::make_pair( expr3, ( ( std::ostringstream& )( std::ostringstream() << val3 ) ).str() ) } );                                    \
	}

		DG_DEFINE_COMPARISON_HELPER_3( LT, <, LT, < )
		DG_DEFINE_COMPARISON_HELPER_3( LT, <, LE, <= )
		DG_DEFINE_COMPARISON_HELPER_3( LE, <=, LT, < )
		DG_DEFINE_COMPARISON_HELPER_3( LE, <=, LE, <= )
#undef DG_DEFINE_COMPARISON_HELPER_3

	private:
		bool m_status;					   //!< current assertion check status
		AssertionCheckContext *m_context;  //!< string assertion check result representation
	};

	/// assertion check printer helper class
	/// \tparam T assertion check context type
	/// \tparam F function type to call to register error
	template< typename T, typename F >
	class AssertErrorPrinter
	{
	public:
		/// constructor
		/// \param type error type
		/// \param err_code error code
		/// \param file filename where error happens
		/// \param line number of line in file where error happens
		/// \param func function name where error happens
		/// \param result assertion check result context
		/// \param error_handler error handler function
		AssertErrorPrinter(
			DG::ErrorType type,
			DG::DGErrorID err_code,
			const char *file,
			const char *line,
			const char *func,
			const T &result,
			F error_handler ) :
			m_type( type ),
			m_err_code( err_code ), m_file( file ), m_line( line ), m_func( func ), m_result( result ), m_error_handler( error_handler )
		{}

		/// AssertMessage assignment operator: used as the place to call DG error register function
		/// \param message additional error message to add to the assertion check message
		AssertErrorPrinter &operator=( const DG::AssertMessage< std::tuple_size< decltype( T::m_operands ) >::value > &message )
		{
			{
				const auto additional_args = message.args();
				const auto sz = std::min< size_t >( m_result.m_operands.size(), additional_args.size() );
				for( size_t i = 0; i < sz; ++i )
					if( !additional_args[ i ].empty() )
						m_result.m_operands[ i ].first = additional_args[ i ];
			}

			std::ostringstream res;
			{
				static_assert(
					std::tuple_size< decltype( m_result.m_operands ) >::value - 1 == std::tuple_size< decltype( m_result.m_operation ) >::value );

				res << "Condition '";
				if constexpr( std::tuple_size< decltype( m_result.m_operands ) >::value == 1 )
				{
					res << m_result.m_operands[ 0 ].first << ( ( m_result.m_operands[ 0 ].second == "true" ) ? " is false" : " is true" );
				}
				else if constexpr( std::tuple_size< decltype( m_result.m_operands ) >::value == 2 )
				{
					res << m_result.m_operands[ 0 ].first << ' ' << m_result.m_operation[ 0 ] << ' ' << m_result.m_operands[ 1 ].first;
				}
				else if constexpr( std::tuple_size< decltype( m_result.m_operands ) >::value == 3 )
				{
					res << m_result.m_operands[ 1 ].first << ' ' << m_result.m_operation[ 0 ] << ' ' << m_result.m_operands[ 0 ].first << ' '
						<< m_result.m_operation[ 1 ] << ' ' << m_result.m_operands[ 2 ].first;
				}
				else
				{
					static_assert(
						0 < std::tuple_size< decltype( m_result.m_operands ) >::value &&
							std::tuple_size< decltype( m_result.m_operands ) >::value < 3,
						"Number of operands is not supported" );
				}
				res << "' is not met";
				{
					std::ostringstream tmp;
					for( const auto &[ name, val ] : m_result.m_operands )
						if( name != val )
							tmp << ", " << name << " is " << val;
					if( tmp )
						res << ":" << tmp.str().substr( 1 );
				}
			}

			const auto additional_message = message.str();
			if( !additional_message.empty() )
				res << ". " << additional_message;

			m_error_handler( m_file, m_line, m_func, m_type, m_err_code, res.str(), "" );
			return *this;
		}

	private:
		const DG::ErrorType m_type;		 //!< DG error type to register
		const DG::DGErrorID m_err_code;	 //!< error code to use
		const char *m_file;				 //!< file to register error
		const char *m_line;				 //!< position in file to register error
		const char *m_func;				 //!< function name to register error
		T m_result;						 //!< assertion check result context
		F m_error_handler;				 //!< function-like object to register error
	};

	/// errorAdd to assertHandle adapter function
	static inline void assertHandleCallAdapter(
		const char *file,
		const char *line,
		const char *func,
		DG::ErrorType type,
		DGErrorID err_code,
		const std::string &msg,
		const std::string &comment )
	{
		DG::ErrorHandling::assertHandle( file, line, func, msg.c_str(), false );
	}

	/// assertion check macros for bool-like value
	/// \param expr expression to check value
	/// \param str_expr expression string representation
	/// \param actual expression string value
	/// \param err_type error type
	/// \param error_id error code
	/// \param err_handler error handler function to process the error
#define DG_CHECK_HELPER_BOOL_( expr, str_expr, actual, error_type, error_id, err_handler )             \
	switch( 0 )                                                                                        \
	case 0:                                                                                            \
	default:                                                                                           \
		if( const auto ar = DG::AssertCheckResult< 1 >::CompareHelperBool( expr, str_expr, #actual ) ) \
			;                                                                                          \
		else                                                                                           \
			DG::AssertErrorPrinter(                                                                    \
				error_type,                                                                            \
				DG::DGErrorID::error_id,                                                               \
				__FILE__,                                                                              \
				TOSTRING( __LINE__ ),                                                                  \
				FUNCTION_NAME,                                                                         \
				ar.contextGet(),                                                                       \
				err_handler ) = DG::AssertMessage< 1 >()

	/// assertion check macros for two values comparison
	/// \param expr1 first expression to check value
	/// \param expr2 second expression to check value
	/// \param op_str comparison operator string: used to select proper comparator function
	/// \param err_type error type
	/// \param error_id error code
	/// \param err_handler error handler function to process the error
#define DG_CHECK_HELPER_2_( expr1, expr2, op_str, error_type, error_id, err_handler )                                   \
	switch( 0 )                                                                                                         \
	case 0:                                                                                                             \
	default:                                                                                                            \
		if( const auto ar = DG::AssertCheckResult< 2 >::CompareHelper##op_str( ( expr1 ), ( expr2 ), #expr1, #expr2 ) ) \
			;                                                                                                           \
		else                                                                                                            \
			DG::AssertErrorPrinter(                                                                                     \
				error_type,                                                                                             \
				DG::DGErrorID::error_id,                                                                                \
				__FILE__,                                                                                               \
				TOSTRING( __LINE__ ),                                                                                   \
				FUNCTION_NAME,                                                                                          \
				ar.contextGet(),                                                                                        \
				err_handler ) = DG::AssertMessage< 2 >()

	/// assertion check macros for 3 values comparison: for example value1 < value2 && value2 < value3
	/// \param expr1 first expression to check value
	/// \param expr2 second expression to check value
	/// \param expr3 third expression to check value
	/// \param op1 comparison operator string: used to select proper comparator function
	/// \param op2 comparison operator string: used to select proper comparator function
	/// \param err_type error type
	/// \param error_id error code
	/// \param err_handler error handler function to process the error
#define DG_CHECK_HELPER_3_( expr1, expr2, expr3, op1, op2, error_type, error_id, err_handler )                                               \
	switch( 0 )                                                                                                                              \
	case 0:                                                                                                                                  \
	default:                                                                                                                                 \
		if( const auto ar = DG::AssertCheckResult< 3 >::CompareHelper##op1##op2( ( expr1 ), ( expr2 ), ( expr3 ), #expr1, #expr2, #expr3 ) ) \
			;                                                                                                                                \
		else                                                                                                                                 \
			DG::AssertErrorPrinter(                                                                                                          \
				error_type,                                                                                                                  \
				DG::DGErrorID::error_id,                                                                                                     \
				__FILE__,                                                                                                                    \
				TOSTRING( __LINE__ ),                                                                                                        \
				FUNCTION_NAME,                                                                                                               \
				ar.contextGet(),                                                                                                             \
				err_handler ) = DG::AssertMessage< 3 >()

	}	// namespace DG


#include "dg_tracing_facility.h"
#include "dg_time_utilities.h"
#include "DGLog.h"

#ifdef _MSC_VER
	#include <intrin.h>
	#ifndef NDEBUG
		#include <dbghelp.h>
		#pragma comment(lib, "dbghelp.lib")
	#endif
#endif
#ifdef __linux__
	#include <execinfo.h>
#endif


// Format location string (implementation)
inline std::string DG::ErrorHandling::location2str( const char *file, const char *line, const char *func )
{
	const std::string cropped_file = std::filesystem::path( file ).filename().string();

	std::string cropped_func = std::string( func );
	cropped_func = cropped_func.substr( 0, cropped_func.find_last_of( "(" ) );		// cutoff arglist
	const auto last_space_pos = cropped_func.find_last_of( " " );
	if( last_space_pos != std::string::npos )
		cropped_func = cropped_func.substr( cropped_func.find_last_of( " " ) + 1 );	// cutoff type

	return cropped_file + ": " + line + " [" + cropped_func + "]";
}


// Register error (implementation)
inline void DG::ErrorHandling::errorAdd( const char *file, const char *line, const char *func,
	DG::ErrorType type, DGErrorID err_code, const std::string& msg, const std::string& comment )
{
	const char *type_str;
	switch( type )
	{
		#define _( id, label, desc ) case ErrorType::id: type_str = label; break;
		DG_ERROR_TYPE_LIST
		#undef _
	}
	std::string full_msg = std::string( type_str ) + code2str( err_code ) + "\n" + msg + "\n" +
		location2str( file, line, func ) + "\n";

	if( !comment.empty() )
		full_msg = comment + "...\n" + full_msg;

	// add to collection
	if( type != ErrorType::VALIDATION_ERROR )
		m_error_collection.add( ErrorRecord( full_msg, type, err_code ) );

	// trace it
	DG_TRC_CRITICAL( type_str, (msg + " | "  + location2str( file, line, func ) ).c_str() );
	DG_LOG_PRINTS( DG::TimeHelper::curStringTime() + full_msg );

	#ifndef NDEBUG
	{
		auto stack = '\n' + stackTrace( 2, 8 );
		DG_TRC_CRITICAL( "Call stack", stack.c_str() );
		DG_LOG_PRINTS( "Call stack:" + stack );
	}
	#endif

	// throw exception
	throw DGException( full_msg );
}


// Print all registered errors into given stream (implementation)
inline bool DG::ErrorHandling::errorsPrint( std::ostream &strm, const std::string err_file_path )
{
	std::stringstream all_errors;
	std::stringstream critical_errors;

	if( m_error_collection.m_err_deque.empty() )
		return false;

	for( auto &rec : m_error_collection.m_err_deque )
	{
		all_errors << rec.m_err_msg;
		if( rec.m_err_type >= ErrorType::CRITICAL_ERROR )
			critical_errors << rec.m_err_msg;
	}
	all_errors << "\n\n";
	critical_errors << "\n\n";

	if( err_file_path.empty() )
	{
		// print all errors into stream
		strm << all_errors.str();
	}
	else
	{
		// print only critical errors into stream
		if( isErrorCritical() )
		{
			strm << "There are CRITICAL errors. Check " << err_file_path << " for details.\n";
			strm << critical_errors.str();
		}

		// then print all error into file
		{
			std::ofstream fout( err_file_path, std::ios::app );
			if( !fout.fail() )
				fout.write( all_errors.str().c_str(), all_errors.str().length() );
		}
	}

	return isErrorCritical();
}

// Handle assertion (implementation)
inline void DG::ErrorHandling::assertHandle( const char *file, const char *line, const char *func, const char *expr_str, bool expr )
{
	if( !expr )
	{
		const std::string msg = std::string( "Assertion failed: '" ) + expr_str + "'. " + location2str( file, line, func );

		// trace it
		DG_TRC_CRITICAL( "Assertion", msg.c_str() );
		DG_LOG_PRINTS( DG::TimeHelper::curStringTime() + msg + "\n\n" );

		// print it
		std::cout << msg << '\n';
		#ifdef _MSC_VER
			__debugbreak();
		#endif
	}
}


// Collect stack back-trace into a string (implementation)
inline std::string DG::ErrorHandling::stackTrace( size_t skip, size_t depth )
{
	std::string ret;

#if defined( _MSC_VER )
	std::mutex mx;							// thread protection mutex
	static BOOL symInitialized = FALSE;		// SymInitialize() was called

	std::lock_guard< std::mutex > lk( mx );
	HANDLE process = GetCurrentProcess();
	void *stack[ 32 ];

	size_t frames = CaptureStackBackTrace( (DWORD )skip, (DWORD )(depth < sizeof stack ? depth : sizeof stack), stack, NULL );

	#ifndef NDEBUG
	if( !symInitialized )
		symInitialized = SymInitialize( process, NULL, TRUE );
	#endif

	for( size_t fi = 0; fi < frames; fi++ )
	{
		DWORD64 address = (DWORD64 )stack[ fi ];

		std::string frame_desc = DG_FORMAT( std::hex << "[0x" << address << "]\n" );
		#ifndef NDEBUG
		if( symInitialized )
		{
			const size_t max_func_len = 256;
			char symbol_buf[ sizeof( SYMBOL_INFO ) + max_func_len ] = {};
			SYMBOL_INFO *symbol = (SYMBOL_INFO *)&symbol_buf;
			symbol->MaxNameLen = max_func_len;
			symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
			IMAGEHLP_LINE64 line = {};
			line.SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
			DWORD displacement;
			if( SymFromAddr( process, address, NULL, symbol ) &&
				SymGetLineFromAddr64( process, address, &displacement, &line ) )
			{
				frame_desc = DG_FORMAT(
					' ' << std::filesystem::path( line.FileName ).filename().string()
					<< '(' << symbol->Name << ':' << line.LineNumber << ")\n" );
			}
		}
		#endif
		ret += frame_desc;
	}

#elif defined( __linux__ )

	void *stack[ 32 ];
	depth += skip;
	size_t frames = backtrace( stack, depth < sizeof stack ? depth : sizeof stack );
	char **strings = backtrace_symbols( stack, frames );
	for( size_t fi = skip; fi < frames; fi++ )
		ret += ' ' + std::filesystem::path( strings[ fi ] ).filename().string() + '\n';

#endif
	return ret;
}


#endif	// DG_ERROR_HANDLING_H
