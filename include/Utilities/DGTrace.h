//////////////////////////////////////////////////////////////////////
/// \file  DGTrace.h
/// \brief DG tracing facility
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains declarations of tracing facility functionality.
/// Tracing facility is intended to be used for frequent events, when
/// the speed of tracing is very important, but it is OK to lose the trace
/// information in case of app crash.
/// For high-reliability (but slow) logging, use logging facility (Utilities/DGLog.h)

#ifndef DG_TRACE_H
#define DG_TRACE_H


#include "DGLog.h"
#include "dg_tracing_facility.h"

//
// NOTE: Legacy macros
// For new development use macros from "dg_tracing_facility.h"
// To enable tracing via these legacy macros, create dg_trace.ini file in the
// current directory of the process you work with, and add line LegacyTrace=Basic into that file.
//
#define DG_TRACE() DGTrace::Tracer __dg_trace_( &DGTrace::g_TracingFacility, &DG_TRC_GROUP_VAR( LegacyTrace ), FUNCTION_NAME, DGTrace::lvlBasic )
#define DG_TRACE_OP( x ) \
	DGTrace::Tracer      \
		__dg_trace_( &DGTrace::g_TracingFacility, &DG_TRC_GROUP_VAR( LegacyTrace ), FUNCTION_NAME, DGTrace::lvlBasic, "Operation = %d", (int)( x ) )
#define DG_TRACE_COMMENT( a )         __dg_trace_.Trace( DGTrace::TracingFacility::TraceType::Point, a )
#define DG_TRACE_STREAM               __dg_trace_
#define DG_TRACE_STREAM_OUT()         __dg_trace_ << '\n';
#define DG_TRACE_OPTIONAL_INT( a )    __dg_trace_.Trace( DGTrace::TracingFacility::TraceType::Point, #a " = %d", (int)( a ) )
#define DG_TRACE_INTERVAL_START()     __dg_trace_.Trace( DGTrace::TracingFacility::TraceType::Start, "" )
#define DG_TRACE_INTERVAL_REPORT( a ) __dg_trace_.Trace( DGTrace::TracingFacility::TraceType::Stop, a )
#define DG_TRACE_CLEAR_LOG()
#define DG_OUT_START() DG_TRACE()
#define DG_OUT         DG_TRACE_STREAM

DG_TRC_GROUP_DEF( LegacyTrace )

#endif  // DG_TRACE_H
