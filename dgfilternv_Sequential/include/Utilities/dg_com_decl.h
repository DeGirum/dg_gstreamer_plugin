//////////////////////////////////////////////////////////////////////
/// \file  dg_com_decl.h
/// \brief DG common declarations
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declarations common for all DG projects. See dos/donts comment below for rules.
/// 

#ifndef DG_COM_DECL_H
#define DG_COM_DECL_H


// Do's:
//  - place here really common declarations, which has global usefulness and global meaning
//  - place here only declarations which do not have external header dependencies
//  - this code is cross-platform
// Dont's:
//  - do not place here project-specific declarations
//  - do not include other header files


/// Cross-platform signature of the current function
#ifndef FUNCTION_NAME
	#if defined( _WIN32 ) && !defined( LINUX_CMAKE )
		#define FUNCTION_NAME __FUNCSIG__
	#else
		#define FUNCTION_NAME __PRETTY_FUNCTION__
	#endif
#endif


/// Cross-platform macro to define visibility attribute
/// Possible parameters: hidden or default (see more here: https://gcc.gnu.org/wiki/Visibility)
#ifndef CPPVISIBILITY
	#if defined( __GNUG__ )
		#define CPPVISIBILITY(vis) __attribute__((visibility(#vis)))
    #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
	#else
		#define CPPVISIBILITY(vis)
    #define DLL_PUBLIC
    #define DLL_LOCAL
	#endif
#endif


/// Stringifying macros
#ifndef STR
	#define STR(s) #s
#endif
#ifndef XSTR
	#define XSTR(s) STR(s)
#endif
#ifndef STRINGIFY
	#define STRINGIFY(x) #x
#endif
#ifndef TOSTRING
	#define TOSTRING(x) STRINGIFY(x)
#endif


// macros to concatenate _values_ of other macros such as __LINE__
#define DG_CONCAT2(x, y) x ## y
#define DG_CONCAT(x, y) DG_CONCAT2(x, y)


/// String formatting helper macro:
/// print values into std::string in stream-like << manner
/// DG_FORMAT( "some message" << some_value << "another message" << another_value )
#define DG_FORMAT( items ) ((std::ostringstream& )(std::ostringstream() << std::dec << items)).str()

/// Size of various string buffers used in DG tracing and logging facilities to buffer strings
#define DG_LOG_TRACE_BUF_SIZE 2048

#endif	// DG_COM_DECL_H
