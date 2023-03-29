///////////////////////////////////////////////////////////////////////////////
/// \file dg_model_api.cpp 
/// \brief DG Client API for model inference implementation
///
/// Copyright DeGirum Corporation 2021
/// All rights reserved
///
/// This file contains implementation of DG Client API functions and classes
/// for model inference
///

#include "dg_model_api.h"
#include "dg_client.h"
#include "Utilities/dg_version.h"

#ifndef _MSC_VER
	#include <strings.h>
	#define _stricmp strcasecmp	//!< case-insensitive string comparison
#endif


/// Find substring in a string with case insensitive comparison
/// \param[in] str - string to search in
/// \param[in] substr - substring to search for
/// \return true if substring is found
static bool stristr( const std::string &str, const std::string &substr )
{
	auto it = std::search( str.begin(), str.end(), substr.begin(), substr.end(),
		[]( char ch1, char ch2 ) { return std::toupper( ch1 ) == std::toupper( ch2 ); }
	);
	return it != str.end();
}


//
// Get version of the library
//
std::string DG::versionGet()
{
	std::string version;
	std::stringstream ss;
	ss << DG_VERSION_MAJOR << "." << DG_VERSION_MINOR << "." << DG_VERSION_REVISION << "." << DG_GIT_REV;
	return ss.str();
}


//
// Get a list of supported models from AI server.
// In case of server connection errors throws std::exception.
// [in] server takes a string of server IP address and port.
// format: "xxx.xxx.xxx.xxx:port", default port is 8778
// [in] modelzoo_list is output vector with a structure, which
// contains model id, model name, input width and input height
//
void DG::modelzooListGet( const std::string &server, std::vector< DG::ModelInfo > &modelzoo_list )
{
	DG::Client( server ).modelzooListGet( modelzoo_list );
}


//
// Return host system information dictionary.
// [in] server is a string specifying server domain name/IP address and port.
// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
//
json DG::systemInfo( const std::string &server )
{
	return DG::Client( server ).systemInfo();
}


//
// AI server tracing facility management
// [in] server is a string specifying server domain name/IP address and port.
// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
// [in] req - management request
// return results of management request completion (request-specific)
//
json DG::traceManage( const std::string &server, const json &req )
{
	return DG::Client( server ).traceManage( req );
}


//
// AI server model zoo management
// [in] server is a string specifying server domain name/IP address and port.
// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
// [in] req - management request
// return results of management request completion (request-specific)
//
json DG::modelZooManage( const std::string &server, const json &req )
{
	return DG::Client( server ).modelZooManage( req );
}


//
// Send shutdown request to AI server
// [in] server is a string specifying server domain name/IP address and port.
// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
//
void DG::shutdown( const std::string &server )
{
	DG::Client( server ).shutdown();
}


//
// Find a model by name on AI server.
// [in] server takes a string of server IP address and port.
// format: "xxx.xxx.xxx.xxx:port", default port is 8778.
// [in] query is the structure of model parameters to look for.
// return model descriptor to be used in AIModel/AIModelAsync constructors.
// If the model is not found, model descriptor contains empty model name
//
DG::ModelInfo DG::modelFind( const std::string &server, const ModelQuery &query )
{
	try
	{
		std::vector< DG::ModelInfo > full_list;
		modelzooListGet( server, full_list );

		for( const auto &m : full_list )
		{
			if( !stristr( m.name, query.model_name ) )
				continue;
			if( !query.device_type.empty() && _stricmp( m.device_type.c_str(), query.device_type.c_str() ) != 0 )
				continue;
			if( !query.runtime_agent.empty() && _stricmp( m.runtime_agent.c_str(), query.runtime_agent.c_str() ) != 0 )
				continue;
			if( query.model_quantized != ModelQuery::Dont_care &&
				((query.model_quantized == ModelQuery::Yes) ^ m.model_quantized) )
				continue;
			if( query.model_pruned != ModelQuery::Dont_care &&
				((query.model_pruned == ModelQuery::Yes) ^ m.model_pruned) )
				continue;
			return m;
		}
	}
	catch( ... )
	{}

	return {};	// model not found
}


//
// Check given Json response for run-time errors. If server reported run-time error, then
// this function will return error message string, otherwise it returns empty string
//
std::string DG::errorCheck( const json &json_response )
{
	return DG::JsonHelper::errorCheck( json_response, "", false );
}


// Get model label dictionary
// \param[in] server is a string specifying server domain name/IP address and port.
// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
// [in] model_info specifies the AI model.
// Either DG::ModelInfo::id or DG::ModelInfo::name fields must be assigned.
// To obtain valid model name, either modelFind() or modelzooListGet() functions should be used.
// return JSON object containing model label dictionary
DG::json DG::labelDictionary( const std::string &server, const std::string &model_name )
{
	return DG::Client( server ).labelDictionary( model_name );
}


//
// Ping server with an instantaneous command
// \param[in] server is a string specifying server domain name/IP address and port.
// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
// return true if no error occurred during the ping
//
bool DG::serverPing( const std::string &server )
{
	try
	{
		return DG::Client( server ).ping();
	}
	catch( ... )
	{
		return false;
	}	
}


//
// Detect all ORCA servers from a given list of host names
// \param[in] source is an IP address in the network, formatted as "xxx.xxx.xxx.xxx[:port]". The port part is optional. If included, all servers
// will be scanned on the port.
//		If omitted, the default port 8778 will be used.
// \return Vector of strings containing IPs of ORCA servers on the network.
//
static std::vector< std::tuple< std::string, DG::DetectionStatus > > detectServers( const std::set< std::string >& source )
{
	const static int BATCH_SIZE = 255;
	const size_t connection_timeout = 3;

	std::mutex result_mutex;
	std::vector< std::tuple< std::string, DG::DetectionStatus > > result;

	// Ping all the servers
	std::vector< std::future< void > > ping_futures( source.size() );

	for( int batch = 0; batch < source.size(); batch += BATCH_SIZE )
	{
		for( int i = batch; i < std::min( batch + BATCH_SIZE, (int )source.size() ); i++ )
			ping_futures[ i ] = std::async(
				std::launch::async, [ &result, &result_mutex, connection_timeout ]( const std::set< std::string >& source, int i ) {
					std::set< std::string >::const_iterator element = source.begin();
					std::advance( element, i );

					try
					{
						DG::Client client = DG::Client( *element, connection_timeout );

						bool ping_result = client.ping();

						std::lock_guard< std::mutex > lock( result_mutex );
						if( ping_result )
							result.push_back( std::tuple< std::string, DG::DetectionStatus >( *element, DG::DetectionStatus::OK ) );
						else
							result.push_back( std::tuple< std::string, DG::DetectionStatus >( *element, DG::DetectionStatus::ProtocolMismatch ) );
					}
					catch( asio::system_error e )
					{}
				},
				source,
				i
			);

		for( int i = batch; i < std::min( batch + BATCH_SIZE, (int )ping_futures.size() ); i++ )
			ping_futures[ i ].get();
	}

	return result;
}


//
// Constructs a hostname string from a pattern
// \param[in] pattern is a pattern suitable for snprintf
// \param[in] numeral is a the number to insert
// \param[in] port is a port number to append to the end of the hostname. Can be empty to skip the port
// \return hostname made by snprintf
//
static std::string hostnameFromPattern( const std::string& pattern, int numeral, const std::string& port )
{
	int raw_size = std::snprintf( nullptr, 0, pattern.c_str(), numeral ) + 1;  // Extra space for '\0'
	if( raw_size <= 0 )
		throw std::runtime_error( "Error during formatting." );
	size_t size = static_cast< size_t >( raw_size );
	std::unique_ptr< char[] > buffer( new char[ size ] );
	std::snprintf( buffer.get(), size, pattern.c_str(), numeral );
	return port.empty() ? std::string( buffer.get(), buffer.get() + size - 1 ) : std::string( buffer.get(), buffer.get() + size - 1 ) + ":" + port;
}


//
// Detect all ORCA servers on a given subnet
// \param[in] root_ip is an IP address in the network, formatted as "xxx.xxx.xxx.xxx[:port]". The port part is optional. If included, all servers
// will be scanned on the port.
//		If omitted, the default port 8778 will be used.
// \param[in] subnet_mask specifies the subnet with the associated IP, formatted as "xxx.xxx.xxx.xxx", where "xxx" is either 255 for fixed bits or 0
// for variable bits.
// \return Vector of strings containing IPs of ORCA servers on the network.
//
std::vector< std::tuple< std::string, DG::DetectionStatus > > DG::detectSubnetServers( const std::string& root_ip, const std::string& subnet_mask )
{
	asio::error_code error_code;
	std::string server_address;
	std::string server_port;
	std::set< std::string > source;

	// Deal with port
	const std::string::size_type delimiter = root_ip.find( ":" );

	if( delimiter == std::string::npos )
	{
		server_address = root_ip;
	}
	else
	{
		server_address = root_ip.substr( 0, delimiter );
		server_port = root_ip.substr( delimiter + 1, server_address.length() );
	}

	// Generate root address
	asio::ip::address_v4 root = asio::ip::make_address_v4( server_address, error_code );
	if( error_code )
		throw std::invalid_argument( error_code.message() );

	// Generate subnet mask
	asio::ip::address_v4 mask = asio::ip::make_address_v4( subnet_mask, error_code );
	if( error_code )
		throw std::invalid_argument( error_code.message() );

	// Generate subnet list
	asio::ip::network_v4 subnet = asio::ip::network_v4( root, mask );
	for( asio::ip::basic_address_iterator< asio::ip::address_v4 > iterator = subnet.hosts().begin(); iterator != subnet.hosts().end(); iterator++ )
		source.insert( server_port.empty() ? ( *iterator ).to_string() : ( *iterator ).to_string() + ":" + server_port );

	return detectServers( source );
}


//
// Detect all ORCA servers with hostnames generated from a prefix and a range
// \param[in] prefix is a string prefix, formatted as "nnnn[:port]". This will be used to generate two sets of hostnames, one with the pattern "nnnn#", and one "nnnn###". For example,
// the prefix "farm", range_start 1, range_end 2 and numeral_width 3 will scan the hosts "farm1", "farm001", "farm2", "farm002". Setting numeral_width to 0 disables pattern generation,
// and will just scan the prefix. The port part is optional. If included, all servers will be scanned on the port. If omitted, the default port 8778 will be used.
// \param[in] range_start is the lowest numeral to be applied to the pattern
// \param[in] range_end is the highest numeral to be applied to the pattern, inclusive
// \param[in] numeral_width is the width of the numeral in the padded set. This is optional, set to 3 by default. Set to 0 to disable pattern generation.
// \return Vector of strings containing hostnames of ORCA servers on the network.
//
std::vector< std::tuple< std::string, DG::DetectionStatus > > DG::detectHostnameServers( const std::string& prefix, const int range_start, const int range_end, const int numeral_width )
{
	std::string base_prefix;
	std::string padded_prefix;
	std::string server_port;
	std::set< std::string > source;

	// Deal with port
	const std::string::size_type delimiter = prefix.find( ":" );

	if( delimiter == std::string::npos )
	{
		base_prefix = prefix + "%d";
		padded_prefix = prefix + "%0" + std::to_string( numeral_width ) + "d";
	}
	else
	{
		base_prefix = prefix.substr( 0, delimiter ) + "%d";
		padded_prefix = prefix.substr( 0, delimiter ) + "%0" + std::to_string( numeral_width ) + "d";
		server_port = prefix.substr( delimiter + 1, prefix.length() );
	}

	// Generate subnet list
	if( numeral_width == 0 )
	{
		source.insert( prefix );
	}
	else
	{
		for( int i = range_start; i <= range_end; i++ )
		{
			source.insert( hostnameFromPattern( base_prefix, i, server_port ) );
			source.insert( hostnameFromPattern( padded_prefix, i, server_port ) );
		}
	}

	return detectServers( source );
}


//
// Constructor.
// In case of server connection errors throws std::exception.
// [in] server takes a string of server IP address and port.
// format: "xxx.xxx.xxx.xxx:port", default port is 8778.
// [in] model_name is an input and should contain the model name.
// [in] model_params is runtime parameters, which define model runtime behavior (optional)
// [in] connection_timeout_ms is the AI server connection timeout in milliseconds (optional)
//
DG::AIModel::AIModel(
	const std::string &server,
	const std::string &model_name,
	const ModelParamsReadAccess &model_params,
	size_t connection_timeout_ms ) :
	m_client( new DG::Client( server, connection_timeout_ms ) )
{
	m_client->openStream( model_name, 0, model_params.jsonGet() );
}


//
// Destructor
//
DG::AIModel::~AIModel()
{
}


//
// Run the inference on provided byte array.
// In case of errors throws std::exception.
// [in] data is a vector of input data for each model input where each data element is a vector of bytes.
// [out] json_response is the result of the inference
// 
void DG::AIModel::predict( std::vector< std::vector< char > > &data, json &json_response )
{
	m_client->predict( data, json_response );
}


//
// Constructor.
// In case of server connection errors throws std::exception.
// [in] server takes a string of server IP address and port.
// format: "xxx.xxx.xxx.xxx:port", default port is 8778.
// [in] model_name is an input and should contain the model name.
// [in] callback is user callback functional, which will be called as soon as prediction result is ready
// [in] model_params is runtime parameters, which define model runtime behavior (optional) 
// [in] frame_queue_depth is the depth of internal frame queue (optional)
// [in] connection_timeout_ms is the AI server connection timeout in milliseconds (optional)
// [in] inference_timeout_ms is the AI server inference timeout in milliseconds (optional)
//
DG::AIModelAsync::AIModelAsync(
	const std::string &server,
	const std::string &model_name,
	callback_t callback,
	const ModelParamsReadAccess &model_params,
	size_t frame_queue_depth,
	size_t connection_timeout_ms,
	size_t inference_timeout_ms ) :
	m_client( new DG::Client( server, connection_timeout_ms, inference_timeout_ms ) )
{
	m_client->openStream( model_name, frame_queue_depth, model_params.jsonGet() );
	m_client->resultObserve( callback );
}


//
// Destructor
// Will wait until all outstanding results are received
//
DG::AIModelAsync::~AIModelAsync()
{
	m_client->dataEnd();
}


//
// Set user callback
// [in] callback is user callback functional, which will be called asynchronously from the main
// execution thread as soon as prediction result is ready.
//
void DG::AIModelAsync::setCallback( callback_t callback )
{
	m_client->resultObserve( callback );
}


//
// Start the inference on given data vector.
// In case of errors throws std::exception.
// This is non-blocking call.
// [in] data is a vector of input data for each model input where each data element is a vector of bytes.
// [in] frame_info is optional frame information string to be passed to the callback along the frame result
//
void DG::AIModelAsync::predict( std::vector< std::vector< char > > &data, const std::string &frame_info )
{
	m_client->dataSend( data, frame_info );
}


//
// Wait for completion of all outstanding inferences
//
void DG::AIModelAsync::waitCompletion()
{
	m_client->dataEnd();
}


//
// Get # of outstanding inference results scheduled so far
//
int DG::AIModelAsync::outstandingResultsCountGet() const
{
	return m_client->outstandingResultsCountGet();
}


//
// If ever during consecutive calls to predict() methods server reported run-time error, then
// this method will return error message string, otherwise it returns empty string.
//
std::string DG::AIModelAsync::lastError() const
{
	return m_client->lastError();
}
