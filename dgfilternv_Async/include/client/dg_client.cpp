///////////////////////////////////////////////////////////////////////////////
/// \file dg_client.cpp 
/// \brief Class, which implements client-side functionality of
///  DG client-server system
///
/// Copyright DeGirum Corporation 2021
///
/// This file contains implementation of DG::Client class:
/// which handles protocol of DG client-server system
///

#include "dg_client.h"
#include <fstream>
#include <climits>
#include "Utilities/DGTrace.h"

/// profiler group
DG_TRC_GROUP_DEF( AIClient );

///////////////////////////////////////////////////////////////////////////////
// Static functions

/// Extract file name from file path
/// \param[in] model_path - full file path
/// \param[out] model_ext - file extension
/// \return file name w/o path and w/o extension
static std::string modelPath2ModelName( const std::string &model_path, std::string &model_ext )
{
	if( model_path.empty() )
		return "";

	size_t start_pos = model_path.find_last_of( "/" );
	if( start_pos == std::string::npos )
	{
		start_pos = model_path.find_last_of( "\\" );
		if( start_pos == std::string::npos )
			start_pos = 0;
		else
			start_pos++;
	}
	else
		start_pos++;

	size_t ext_pos = model_path.find_last_of( "." );
	if( ext_pos == std::string::npos )
		ext_pos = model_path.length();

	const size_t sub_len = ext_pos - start_pos;

	model_ext = model_path.substr( ext_pos + 1, sub_len );
	return model_path.substr( start_pos, sub_len );
}


/// Read contents of given file into given string buffer
/// \param[in] file_name - file name
/// \return string with file contents
static std::string readFileContent( const char *file_name )
{
	std::ifstream pf( file_name, std::ifstream::binary );
	if ( !pf.is_open() )
		throw std::runtime_error( "File '" + std::string( file_name ) + "' does not exist" );
	return std::string( std::istreambuf_iterator< char >( pf ), std::istreambuf_iterator< char >() );
}


/// Read contents of given file into given vector buffer
/// \param[in] file_name - file name
/// \param[out] data - vector with file contents
static void readFileContent( const char *file_name, std::vector< char > &data )
{
	std::ifstream is( file_name, std::ifstream::binary );
	if( !is.is_open() )
		throw std::runtime_error( "File '" + std::string( file_name ) + "' does not exist" );
	is.seekg( 0, std::ios_base::end );
	std::size_t size = is.tellg();
	is.seekg( 0, std::ios_base::beg );
	data.resize( size );
	is.read( data.data(), size );
}


///////////////////////////////////////////////////////////////////////////////
// DG::Client class methods

namespace DG 
{
	/// Constructor. Sets up active server using address.
	/// [in] server_address - server address
	/// [in] connection_timeout_ms - connection timeout in milliseconds
	/// [in] inference_timeout_ms - AI server inference timeout in milliseconds
	Client::Client( const std::string &server_address, size_t connection_timeout_ms, size_t inference_timeout_ms ) :
		m_command_socket( m_io_context ),
		m_stream_socket( m_io_context ),
		m_async_result_callback( nullptr ),
		m_io_context(),
		m_async_outstanding_results( 0 ),
		m_async_stop( false ),
		m_read_size( 0 ),
		m_frame_queue_depth( 0 ),
		m_connection_timeout_ms( connection_timeout_ms ),
		m_inference_timeout_ms( inference_timeout_ms )
	{
		DG_TRC_BLOCK( AIClient, constructor, DGTrace::lvlBasic );
		const std::string::size_type	delimiter = server_address.find( ":" );

		if( delimiter == std::string::npos ) 
		{ 
			m_server_address.ip = server_address;
			m_server_address.port = DG::DEFAULT_PORT;
		}
		else
		{
			m_server_address.ip = server_address.substr( 0, delimiter );
			m_server_address.port = atoi( server_address.substr( delimiter + 1, server_address.length() ).c_str() );
		}

		{
			DG_TRC_BLOCK( AIClient, constructor::socket_connect, DGTrace::lvlBasic );
			m_command_socket = main_protocol::socket_connect( m_io_context, m_server_address.ip, m_server_address.port, m_connection_timeout_ms / 1000 );
		}
	}


	// Destructor
	Client::~Client()
	{
		DG_TRC_BLOCK( AIClient, destructor, DGTrace::lvlBasic );
		try
		{
			if( m_async_thread.joinable() )
				dataEnd();
			closeStream();
			m_command_socket.close();

			// Stop the asynchronous executor
			main_protocol::stop( m_io_context );
		}
		catch(...)
		{}
	}


	// Send shutdown request to AI server
	void Client::shutdown()
	{
		DG_TRC_BLOCK( AIClient, shutdown, DGTrace::lvlBasic );
		const json request = json( { { "op", main_protocol::commands::SHUTDOWN } } );
		json response;

		transmitCommand( "shutdown", request, response );

		// establish new client connection and send final empty packet to push server out of accept loop
		{
			DG_TRC_BLOCK( AIClient, shutdown::socket_connect, DGTrace::lvlBasic );
			auto temp_socket = main_protocol::socket_connect( m_io_context, m_server_address.ip, m_server_address.port, m_connection_timeout_ms / 1000 );
			main_protocol::write( temp_socket, "", 0 );
			main_protocol::socket_close( temp_socket );
		}
	}


	// Get model label dictionary
	// [in] model_name specifies the AI model.
	// return JSON object containing model label dictionary
	json Client::labelDictionary( const std::string &model_name )
	{
		DG_TRC_BLOCK( AIClient, labelDictionary, DGTrace::lvlBasic );

		const json request = json( { { "op", main_protocol::commands::LABEL_DICT }, { "name", model_name } } );
		json response;

		transmitCommand( "labelDictionary", request, response );
		return response[ main_protocol::commands::LABEL_DICT ];
	}


	// 'stream' op handler: creates and opens socket for stream of frames to be used by subsequent predict() calls
	// [in] model_name - model name, which defines op destination
	// [in] frame_queue_depth - the depth of internal frame queue
	// [in] additional_model_parameters - additional model parameters in Json format compatible with DG Json model configuration	
	void Client::openStream( const std::string &model_name, size_t frame_queue_depth, const json &additional_model_parameters )
	{
		DG_TRC_BLOCK( AIClient, openStream, DGTrace::lvlBasic );
		m_frame_queue_depth = frame_queue_depth;

		json j_request = json( { { "op", main_protocol::commands::STREAM }, { "name", model_name } } );
		if( !additional_model_parameters.empty() )
			j_request[ "config" ] = additional_model_parameters;

		const auto request = DG::messagePrepare( j_request );

		{
			DG_TRC_BLOCK( AIClient, openStream::socket_connect, DGTrace::lvlBasic );
			m_stream_socket = main_protocol::socket_connect( m_io_context, m_server_address.ip, m_server_address.port, m_connection_timeout_ms / 1000 );
		}
		main_protocol::write( m_stream_socket, request.data(), request.size() );
	}


	// Close stream opened by openStream()
	void Client::closeStream( void )
	{
		DG_TRC_BLOCK( AIClient, closeStream, DGTrace::lvlBasic );
		if( m_stream_socket.is_open() )
		{
			// send empty packet to indicate end-of-stream;
			// we use async write to avoid long timeouts when writing to closed socket
			main_protocol::write_async( m_stream_socket, "", 0 );
			main_protocol::run_async( m_io_context, m_connection_timeout_ms );

			main_protocol::socket_close( m_stream_socket );
		}
	}


	// Get list of models in all model zoos of all active servers
	// [out] modelzoo_list - array of models in model zoos
	void Client::modelzooListGet( std::vector<DG::ModelInfo> &modelzoo_list )
	{
		DG_TRC_BLOCK( AIClient, modelzooListGet, DGTrace::lvlBasic );
		const json request = json( { { "op", main_protocol::commands::MODEL_ZOO } } );
		json response;

		if( transmitCommand( "modelzooListGet", request, response ) )
		{
			for( size_t i = 0; i < response[ main_protocol::commands::MODEL_ZOO ].size(); i++ )
			{
				DG::ModelInfo mi;
				const auto &node = response[ main_protocol::commands::MODEL_ZOO ][ i ];
				mi.id = node[ "id" ].get< size_t >();
				mi.name = node[ "name" ].get< std::string >();
				mi.W = node[ "W" ].get< int >();
				mi.H = node[ "H" ].get< int >();
				mi.C = node[ "C" ].get< int >();
				mi.N = node[ "N" ].get< int >();
				mi.device_type = node[ "DeviceType" ].get< std::string >();
				mi.runtime_agent = node[ "RuntimeAgent" ].get< std::string >();
				mi.model_quantized = node[ "Quantized" ].get< bool >();
				mi.model_pruned = node[ "Pruned" ].get< bool >();
				mi.input_type = node[ "InputType" ].get< std::string >();
				mi.input_tensor_layout = node[ "InputTensorLayout" ].get< std::string >();
				mi.input_color_space = node[ "InputColorSpace" ].get< std::string >();
				mi.input_image_format = node[ "InputImageFormat" ].get< std::string >();
				mi.input_raw_data_type = node[ "InputRawDataType" ].get< std::string >();
				mi.extended_params = DG::ModelParamsWriter( node[ "ModelParams" ].get< std::string >() );
				modelzoo_list.emplace_back( mi );
			}
		}
	}


	// Return host system information dictionary
	json Client::systemInfo()
	{
		DG_TRC_BLOCK( AIClient, systemInfo, DGTrace::lvlBasic );
		const json request = json( { { "op", main_protocol::commands::SYSTEM_INFO } } );
		json response;

		if( transmitCommand( "systemInfo", request, response ) )
			return response[ main_protocol::commands::SYSTEM_INFO ];
		return {};
	}


	// AI server tracing facility management
	// [in] req - management request
	// return results of management request completion (request-specific)
	json Client::traceManage( const json &req )
	{
		DG_TRC_BLOCK( AIClient, traceManage, DGTrace::lvlBasic );
		const json request = json( { { "op", main_protocol::commands::TRACE_MANAGE }, { "args", req } } );
		json response;

		if( transmitCommand( "traceManage", request, response ) )
			return response[ main_protocol::commands::TRACE_MANAGE ];
		return {};
	}


	// AI server model zoo management
	// [in] req - management request
	// return results of management request completion (request-specific)
	json Client::modelZooManage( const json &req )
	{
		DG_TRC_BLOCK( AIClient, modelZooManage, DGTrace::lvlBasic );
		const json request = json( { { "op", main_protocol::commands::ZOO_MANAGE }, { "args", req } } );
		json response;

		if( transmitCommand( "modelZooManage", request, response ) )
			return response[ main_protocol::commands::ZOO_MANAGE ];
		return {};
	}


	// Ping server with an instantaneous command
	// return true if no error occurred during the ping
	bool Client::ping()
	{
		DG_TRC_BLOCK( AIClient, ping, DGTrace::lvlBasic );
		const json request = json( { { "op", main_protocol::commands::SLEEP } } );
		json response;

		try
		{
			transmitCommand( "ping", request, response );
			return true;
		}
		catch( ... )
		{
			return false;
		}
	}


	// Run prediction on given data frame. Stream should be opened by openStream()
	// [in] data - array containing frame data
	// [out] output - prediction result (Json array)
	void Client::predict( std::vector< std::vector< char > > &data, json &output )
	{
		DG_TRC_BLOCK( AIClient, predict::vector, DGTrace::lvlBasic );
		if( !m_stream_socket.is_open() )
			throw std::runtime_error( "predict: socket was not opened" );

		// Send frame data
		for( const auto &d : data )
			main_protocol::write( m_stream_socket, d.data(), d.size() );

		// Read reply message
		DG::JsonHelper::serial_container_t response_buffer;
		main_protocol::read( m_stream_socket, response_buffer );
		output = DG::JsonHelper::jsonDeserialize( response_buffer );
	}


	//
	// Install prediction results observation callback
	// [in] callback - user callback, which will be called as soon as prediction result is ready when using dataSend() methods
	//
	void Client::resultObserve( callback_t callback )
	{
		DG_TRC_BLOCK( AIClient, resultObserve, DGTrace::lvlBasic );
		if( m_async_thread.joinable() )
			throw std::runtime_error( "resultObserve: cannot install observation callback while result receiving thread is running" );

		m_async_result_callback = callback;
	}

		
	//
	// Send given data frame for prediction. Prerequisites:
	//   stream should be opened by openStream();
	//   user callback to receive prediction results should be installed by resultObserve().
	// On the very first frame the method launches result receiving thread. This thread asynchronously
	// retrieves prediction results and for each result calls user callback installed by resultObserve().
	// To terminate this thread dataEnd() should be called when no more data frames are expected.
	// [in] data - array containing frame data
	//
	void Client::dataSend( const std::vector< std::vector< char > > &data, const std::string &frame_info )
	{
		DG_TRC_BLOCK( AIClient, dataSend, DGTrace::lvlDetailed );

		if( !m_stream_socket.is_open() )
			throw std::runtime_error( "dataSend: socket was not opened" );

		if( m_async_result_callback == nullptr )
			throw std::runtime_error( "dataSend: observation callback is not installed" );
		
		{
			std::unique_lock< std::mutex > lock( m_communication_mutex );

			// check if error was detected
			auto check_last_error = [ & ]() {				
				return m_async_stop && !m_last_error.empty();
			};

			// If error occurred then return: no need to send frame
			if( check_last_error() )
				return;

			// Wait until number of outstanding frames becomes less than frame queue depth
			if( m_async_outstanding_results >= m_frame_queue_depth )
			{
				if( !m_waiter.wait_for( lock, std::chrono::milliseconds( m_inference_timeout_ms ), [ & ] {
					return m_async_outstanding_results < m_frame_queue_depth || m_async_stop;
				} ) )
				{
					DG_ERROR(
						DG_FORMAT(
							"Timeout waiting for inference response from server '"
							<< m_command_socket.remote_endpoint().address().to_string() << ":"
							<< m_command_socket.remote_endpoint().port() ),
						ErrTimeout );
				}

			}

			// Check for error one more time: it may happen while waiting in the previous statement
			if( check_last_error() )
				return;

			// Put frame info into the queue first
			m_frame_info_queue.push( frame_info );

			m_async_outstanding_results++;

			// If no read is in progress, initialize read
			if( m_async_outstanding_results == 1 )				
				main_protocol::initiate_read( m_stream_socket, &m_read_size, [ this ]() { this->dataReceive(); } );
		}

		// Send data frames to server
		for( const auto &d : data )
			main_protocol::write( m_stream_socket, d.data(), d.size() );

		// Start result receiving thread if not started yet
		if( !m_async_thread.joinable() )
		{
			// If dataSend() is called on stopped thread, this means that either this is the very first call,
			// or previous batch of frames was stopped, and somebody wants to restart it;
			// either way we want to restart pipeline, so we reset m_async_stop flag
			m_async_stop = false;
			m_last_error = "";
			
			m_async_thread = std::thread(
				[ this ]()
				{
					try
					{
						// Loop until stop is requested AND then all outstanding results are received
						while( !m_async_stop || m_async_outstanding_results > 0 )
						{
							// Run ASIO executor indefinitely until result is received or server disconnects
							main_protocol::run_async( m_io_context );

							// Wait until a restart is signaled
							{
								std::unique_lock< std::mutex > lock( m_communication_mutex );
								m_waiter.wait( lock, [ & ] { return m_async_outstanding_results > 0 || m_async_stop; } );
							}
						}
					}
					catch( std::exception &e )
					{
						// signal abort in case of communication error
						{
							std::lock_guard< std::mutex > lock( m_communication_mutex );
							m_last_error = e.what();
							m_async_outstanding_results = 0;
							m_async_stop = true;
						}
						m_waiter.notify_all();  // notify main thread to stop waiting
					}
				} );
		}
		else
		{
			// Signal a restart
			m_waiter.notify_all();  // notify result receiving thread
		}
	}


	//
	// Callback for asynchronous read. Receives incoming message size and completes read. Schedules next read, if any are outstanding.
	//
	void Client::dataReceive()
	{
		DG_TRC_BLOCK( AIClient, dataReceive, DGTrace::lvlDetailed );

		// We now know size of incoming message, and can complete its read
		DG::JsonHelper::serial_container_t response_buffer;
		main_protocol::handle_read( m_stream_socket, response_buffer, m_read_size );

		// Parse result and check for error
		const json result = DG::JsonHelper::jsonDeserialize( response_buffer );
		const std::string err_msg = DG::JsonHelper::errorCheck( result, "", false );

		std::string frame_info;
		{
			std::lock_guard< std::mutex > lock( m_communication_mutex );

			// Get frame info
			frame_info = m_frame_info_queue.front();
			m_frame_info_queue.pop();

			if( !err_msg.empty() )
			{
				m_last_error = err_msg;

				// abort worker thread in case of error
				m_async_outstanding_results = 0;
				m_async_stop = true;
			}
			else
				m_async_outstanding_results--;

			// If there are unanswered requests, initialize another read
			if( m_async_outstanding_results > 0 )
				main_protocol::initiate_read( m_stream_socket, &m_read_size, [ this ]() { this->dataReceive(); } );

			m_waiter.notify_all();  // notify result receiving thread
		}

		// Invoke user callback: catch all errors
		try
		{
			m_async_result_callback( result, frame_info );
		}
		catch( ... )
		{}
	}


	//
	// Finalize the sequence of data frames. Should be called when no more data frames are
	// expected to terminate result receiving thread started by dataSend().
	//
	void Client::dataEnd()
	{
		DG_TRC_BLOCK( AIClient, dataEnd, DGTrace::lvlBasic );

		// set stop flag under lock to serialize with result receiving thread
		{
			std::lock_guard< std::mutex > lock( m_communication_mutex );
			m_async_stop = true;
		}
		m_waiter.notify_all();  // notify result receiving thread

		// join result receiving thread
		if( m_async_thread.joinable() )
			m_async_thread.join();
	}


	/// Transmit command Json packet to server, receive response, parse it, and analyze for errors
	/// \param[in] source - description of the server operation initiator (for error reports only)
	/// \param[in] request - Json array with command
	/// \param[out] response - Json array with command response packet
	/// \return true, if some response was received, false otherwise
	bool Client::transmitCommand( const std::string& source, const json& request, json& response )
	{
		DG_TRC_BLOCK( AIClient, transmitCommand, DGTrace::lvlDetailed );

		const std::string request_buffer = DG::messagePrepare( request );
		std::vector< char >	response_buffer;

		// Send request message
		main_protocol::write( m_command_socket, request_buffer.data(), request_buffer.size() );

		// Read reply message
		main_protocol::read( m_command_socket, response_buffer );

		response = json::parse( response_buffer );
		if( !response.is_object() )
			DG_ERROR(
				DG_FORMAT(
					"Response from server '" << m_command_socket.remote_endpoint().address().to_string() << ":"
											 << m_command_socket.remote_endpoint().port() << "' is incorrect." ),
				ErrNotSupportedVersion );
		if( !response.contains( DG::PROTOCOL_VERSION_TAG ) )
			DG_ERROR(
				DG_FORMAT(
					"AI server protocol version data is missing in response from server '" << m_command_socket.remote_endpoint().address().to_string()
																						   << ":" << m_command_socket.remote_endpoint().port()
																						   << "'. Please upgrade AI server instance to newer one." ),
				ErrNotSupportedVersion );

		DG::JsonHelper::errorCheck( response, source );
		return true;
	}


	/// Transmit arbitrary string
	/// \param[in] source - description of the server command initiator (for error reports only)
	/// \param[in] request - request string
	void Client::transmitCommand( const std::string& source, const std::string& request )
	{
		DG_TRC_BLOCK( AIClient, transmitCommand, DGTrace::lvlDetailed );
		main_protocol::write( m_command_socket, request.data(), request.size() );
	}

}  // namespace DG
