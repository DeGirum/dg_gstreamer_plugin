///////////////////////////////////////////////////////////////////////////////
/// \file dg_socket.h 
/// \brief DG socket support: classes for client-server communications over TCP/IP
///
///
/// Copyright DeGirum Corporation 2021
/// All rights reserved

#pragma once

#include <string>
#include <exception>
#include "Utilities/dg_tensor_structs.h"

#include <asio.hpp>

namespace {
	/// DG socket exception class
	class SocketException : public std::exception
	{
	public:
		// Construct a SocketException with a explanatory message.
		// \param[in] message - explanatory message
		// \param[in] incSysMsg - true if system message (from strerror(errno))
		// should be postfixed to the user provided message
		SocketException( const std::string &message, bool inclSysMsg = false ) throw()
			: m_userMessage( message )
		{
			if( inclSysMsg )
			{
				std::stringstream ss;
				ss << ": " << errno;
				m_userMessage += ss.str();
			}
		}

		//   Provided just to guarantee that no exceptions are thrown.
		~SocketException() throw() {}

		// Get the exception message
		// \return exception message
		const char* what() const throw() { return m_userMessage.c_str(); }

	private:
		std::string m_userMessage;	//!< Exception message
	};
}  // namespace


namespace DG 
{
	namespace main_protocol
	{

		/// Socket object type
		using socket_t = asio::ip::tcp::socket;

		/// I/O context object type
		using io_context_t = asio::io_context;

		/// Protocol callback type. The callback is called as soon as server reply size is available.
		using callback_t = std::function< void( void ) >;

		// Header size, just a four byte int
		const int HEADER_SIZE = sizeof( uint32_t ) / sizeof( char );

		//codes for supported commands
		namespace commands
		{
			constexpr const char* STREAM		= "stream";
			constexpr const char* MODEL_ZOO		= "modelzoo";
			constexpr const char* SLEEP			= "sleep";
			constexpr const char* SHUTDOWN		= "shutdown";
			constexpr const char* LABEL_DICT	= "label_dictionary";
			constexpr const char* SYSTEM_INFO	= "system_info";
			constexpr const char* TRACE_MANAGE	= "trace_manage";
			constexpr const char* ZOO_MANAGE	= "zoo_manage";
		}  // namespace commands


		/// Throw exception if error is serious
		/// \param[in] error - error code to check/process
		inline void throw_exception_if_error_is_serious( const asio::error_code& error )
		{
			if( error && !( error == asio::error::eof ) )
				throw asio::system_error( error );
		}


		/// Run executor in I/O context to handle asynchronous operations
		/// \param[in] io_context - I/O context object to run
		/// \param[in] timeout_ms - running timeout in ms; 0 to run until completion of all tasks
		inline void run_async( io_context_t &io_context, size_t timeout_ms = 0 )
		{
			// If ASIO executor was stopped, we need to restart it
			if( io_context.stopped() )
				io_context.restart();

			// Run ASIO executor
			if( timeout_ms > 0 )
				io_context.run_for( std::chrono::milliseconds( timeout_ms ) );
			else			
				io_context.run();
		}


		/// Stop executor
		/// \param[in] io_context - I/O context object to run
		inline void stop( io_context_t &io_context )
		{
			io_context.stop();
		}


		/// Open socket and connect to server
		/// \param[in,out] io_context - execution context
		/// \param[in] ip - server domain name or IP address string
		/// \param[in] port - server TCP port number
		/// \param[in] timeout_s - intended timeout in seconds
		/// \return socket object with established connection to server
		inline socket_t socket_connect( io_context_t &io_context, const std::string &ip, int port, size_t timeout_s )
		{
			asio::error_code error;
			asio::ip::tcp::resolver resolver( io_context );
			asio::ip::tcp::resolver::results_type endpoints = resolver.resolve( asio::ip::tcp::v4(), ip, std::to_string( port ) );
			socket_t ret( io_context );

			// Start the asynchronous operation itself. The lambda that is used as a
			// callback will update the error variable when the operation completes.
			asio::async_connect(
				ret,
				endpoints,
				[ &error ]( const asio::error_code &result_error, const asio::ip::tcp::endpoint & /*result_endpoint*/ ) { error = result_error; } );

			// Run the operation until it completes, or until the timeout.
			run_async( io_context, timeout_s * 1000 );

			// If the asynchronous operation completed successfully then the io_context
			// would have been stopped due to running out of work. If it was not
			// stopped, then the io_context::run_for call must have timed out.
			if( !io_context.stopped() )
			{
				// Close the socket to cancel the outstanding asynchronous operation.
				ret.close();

				// Run the io_context again until the operation completes.
				io_context.run();
			}

			// Restart the io_context to leave it pristine
			io_context.restart();

			// Determine whether a connection was successfully established.
			if( error )
				throw std::system_error( error );

			ret.set_option( asio::ip::tcp::no_delay( true ) );
			return ret;
		}


		/// Close given socket
		inline void socket_close( socket_t &socket )
		{
			socket.shutdown( socket_t::shutdown_both );
			socket.close();
		}


		/// Read all incoming data to buffer, synchronously
		/// \param[in] socket - socket to use. Must be connected
		/// \param[out] response_buffer - buffer for response. Will be resized as needed
		/// \return number of read bytes
		template < typename T = char >
		size_t read( socket_t &socket, std::vector< T > &response_buffer )
		{
			asio::error_code error;
			size_t bytes_read = 0;
			uint32_t big_endian_size = 0;
			char *size_buffer = reinterpret_cast< char * >( &big_endian_size );
			size_t packet_size = 0;

			// Read first 4 bytes to get message length
			bytes_read = asio::read( socket, asio::buffer( size_buffer, HEADER_SIZE ), error );

			if( bytes_read == 0 )
				return 0;
			else if( bytes_read < HEADER_SIZE )
				throw SocketException( "Fail to read incoming packet length from socket " + socket.remote_endpoint().address().to_string() );
			throw_exception_if_error_is_serious( error );

			// Use the length to allocate buffer
			packet_size = ntohl( big_endian_size );
			response_buffer.resize( packet_size );

			// Complete read
			bytes_read = asio::read( socket, asio::buffer( response_buffer ), error );
			throw_exception_if_error_is_serious( error );

			return bytes_read;
		}


		/// Read all incoming data to buffer, synchronously
		/// \param[in] socket - socket to use. Must be connected
		/// \param[out] response_buffer - pointer to tensor buffer for response. Will be resized as needed
		/// \return number of read bytes
		inline size_t read( socket_t &socket, BasicTensor *response_buffer )
		{
			asio::error_code error;
			size_t bytes_read = 0;
			uint32_t big_endian_size = 0;
			char *size_buffer = reinterpret_cast< char * >( &big_endian_size );
			size_t packet_size = 0;

			DG_ASSERT( response_buffer != nullptr );

			// Read first 4 bytes to get message length
			bytes_read = asio::read( socket, asio::buffer( size_buffer, HEADER_SIZE ), error );

			if( bytes_read == 0 )
				return 0;
			else if( bytes_read < HEADER_SIZE )
				throw SocketException( "Fail to read incoming packet length from socket " + socket.remote_endpoint().address().to_string() );
			throw_exception_if_error_is_serious( error );

			// Use the length to allocate buffer
			packet_size = ntohl( big_endian_size );
			response_buffer->alloc< char >( 0, "", { packet_size } );

			// Complete read
			bytes_read = asio::read( socket, asio::buffer( response_buffer->data< char >(), packet_size ), error );
			throw_exception_if_error_is_serious( error );

			return bytes_read;
		}


		/// Write data to socket, synchronously
		/// \param[in] socket - socket to use. Must be connected
		/// \param[in] request_buffer - data to send
		/// \param[in] packet_size - size of data to send, in bytes
		/// \return number of written bytes
		inline size_t write( socket_t &socket, const char *request_buffer, size_t packet_size )
		{
			asio::error_code error;
			size_t bytes_sent = 0;
			uint32_t big_endian_size = 0;
			const char *size_buffer = reinterpret_cast< const char * >( &big_endian_size );

			// Prepare a 4 byte packet to signal message length
			assert( int( packet_size ) < (std::numeric_limits< int32_t >::max)() );
			big_endian_size = htonl( static_cast< uint32_t >( packet_size ) );

			// Signal message length
			asio::write( socket, asio::buffer( size_buffer, HEADER_SIZE ), error );
			throw_exception_if_error_is_serious( error );

			// Send message
			bytes_sent += asio::write( socket, asio::buffer( request_buffer, packet_size ), error );
			throw_exception_if_error_is_serious( error );
			return bytes_sent;
		}


		/// Asynchronously write data to socket.
		/// run_async() should be running in some worker thread to process event loop.
		/// \param[in] socket - socket to use. Must be connected
		/// \param[in] request_buffer - data to send
		/// \param[in] packet_size - size of data to send, in bytes
		/// \return completion waiting function.
		/// It returns true if all bytes were sent, false otherwise.
		/// It accepts timeout in ms. Pass zero to query current state without waiting.
		inline std::function< bool( size_t ) > write_async( socket_t &socket, const char *request_buffer, size_t packet_size )
		{
			// Prepare a 4 byte packet to signal message length
			assert( int( packet_size ) < (std::numeric_limits< int32_t >::max)() );
			
			/// Data structure which stores async. write execution context
			struct WriteContext
			{
				const size_t bytes_total;				//!< total bytes to transfer
				std::atomic< size_t > bytes_sent = 0;	//!< transferred bytes
				std::condition_variable cv;				//!< condition variable to wait for transfer completion
				uint32_t big_endian_size;				//!< header to transfer
				std::vector< char > data;				//!< data to transfer
		
				/// Constructor
				/// \param[in] request_buffer - data to send
				/// \param[in] packet_size - size of data to send, in bytes
				WriteContext( const char *request_buffer, size_t packet_size ):
					big_endian_size( htonl( static_cast< uint32_t >( packet_size ) ) ),
					bytes_total( HEADER_SIZE + packet_size ),
					data( packet_size )
				{
					std::memcpy( data.data(), request_buffer, packet_size );
				}
			};

			// Create write context
			auto write_context = std::make_shared< WriteContext >( request_buffer, packet_size );

			// Write completion event handler
			auto write_handler = [ write_context ]( const asio::error_code &ec, std::size_t bytes_transferred ) {
				write_context->bytes_sent += bytes_transferred;
				// Notify waiter
				write_context->cv.notify_all();
				throw_exception_if_error_is_serious( ec );
			};

			// Prepare buffers
			std::vector< asio::const_buffer > bufs = {
				asio::buffer( (const char *)&write_context->big_endian_size, HEADER_SIZE ),  // header
				asio::buffer( write_context->data )                                          // body
			};

			// Start sending message
			asio::async_write( socket, bufs, write_handler );

			// Return a function, which effectively waits until all bytes will be sent
			// (assuming run_async() is running in some worker thread to process event loop)
			return [ write_context ]( size_t timeout_ms ) -> bool {
				if( timeout_ms == 0 )
					return write_context->bytes_sent == write_context->bytes_total;
				else
				{
					std::mutex mx;
					std::unique_lock< std::mutex > lk( mx );
					const bool ret = write_context->cv.wait_for( lk, std::chrono::milliseconds( timeout_ms ), [ & ] {
						return write_context->bytes_sent == write_context->bytes_total;
					} );
					return ret;
				}
			};
		}

		/// Initiate a reply read, the result of which will be supplied to a callback. Use after sending a frame.
		/// This function just queues a read of four bytes to get the message length. Do not call more than once before handling the reply!
		/// run_async() should be running in some worker thread to process event loop.
		/// \param[in] socket - socket to use. Must be connected
		/// \param[in] read_size - pointer to result packet size. Must be preallocated
		/// \param[in] async_result_callback - callback for reply
		inline void initiate_read( socket_t& socket, uint32_t* read_size, callback_t async_result_callback )
		{
			char *read_size_buffer = reinterpret_cast< char * >( read_size );

			// Begin waiting for read
			asio::async_read( socket, asio::buffer( read_size_buffer, HEADER_SIZE ),
				[ &socket, read_size, async_result_callback ]( const asio::error_code &error, size_t bytes_transferred )
				{
					// Handle the error
					throw_exception_if_error_is_serious( error );

					// Convert size to host endianness
					*read_size = ntohl( *read_size );

					// Call real callback
					async_result_callback();					
				}
			);
		}


		/// Handle reading a reply. Call after the callback from initiate_read has received packet length
		/// \param[in] socket - socket to use. Must be connected
		/// \param[in] async_result_callback - callback for reply
		/// \param[in] read_size - packet size to read in bytes as returned from initiate_read()
		template < typename T = char >
		inline void handle_read( socket_t &socket, std::vector< T >& response_buffer, uint32_t read_size )
		{
			asio::error_code error;

			// We now know size of incoming message, and can queue its read synchronously, since it should be received imminently
			response_buffer.resize( read_size );
			asio::read( socket, asio::buffer( response_buffer ), error );

			throw_exception_if_error_is_serious( error );
		}

	}  // namespace protocol


	namespace video_hub_protocol
	{
		// Header size, two four byte ints: frame ordinal, chunk ordinal, data length
		const int HEADER_ITEM_SIZE = sizeof( uint32_t ) / sizeof( char );	//!< Size of int in bytes
		const int HEADER_SIZE = HEADER_ITEM_SIZE * 4;						//!< Number of ints in header
		const int SUBCANVAS_WIDTH = 640;									//!< view port width
		const int SUBCANVAS_HEIGHT = 480;									//!< view port height
		const double TRANSMISSION_SCALE = 0.5;								//!< image scaling for transmission
		const int CHUNK_SIZE = 64000;										//!< Data length in bytes per datagram
		const int MESSAGE_SIZE = CHUNK_SIZE + HEADER_SIZE;					//!< Total length of message we send


		/// Throw exception if error is serious
		/// \param[in] error - ASIO error
		inline void throw_exception_if_error_is_serious( const asio::error_code& error )
		{
			if( error && !( error == asio::error::eof ) )
				throw asio::system_error( error );
		}


		/// Read all incoming data to buffer, synchronously. Returns one chunk's worth of data
		/// \param[in] socket - ASIO socket to use. Must be connected
		/// \param[out] response_buffer - buffer for response. Should be sized properly
		/// \param[out] remote_endpoint - source of read
		/// \param[out] frame_ordinal - number of frame this message belongs to
		/// \param[out] chunks_expected - total number of data chunks expected
		/// \param[out] chunk_ordinal - number of this chunk in frame
		/// \param[out] data_length - length of the data in bytes. Might differ from vector size!
		/// \return number of read bytes
		inline size_t read(
			asio::ip::udp::socket& socket,
			std::vector< char >& response_buffer,
			asio::ip::udp::endpoint& remote_endpoint,
			uint32_t& frame_ordinal,
			uint32_t& chunks_expected,
			uint32_t& chunk_ordinal,
			uint32_t& data_length
		)
		{
			asio::error_code	error;
			size_t				bytes_read = 0;
			uint32_t*			big_endian_int = 0;

			// Complete read
			bytes_read = socket.receive_from( asio::buffer( response_buffer ), remote_endpoint, 0, error );
			throw_exception_if_error_is_serious( error );

			if( bytes_read != 0 )
			{
				// Parse out the header
				// Frame ordinal
				big_endian_int = (uint32_t* )response_buffer.data();
				frame_ordinal = ntohl( *big_endian_int );

				// Expected number of chunks
				big_endian_int++;
				chunks_expected = ntohl( *big_endian_int );

				// Chunk ordinal
				big_endian_int++;
				chunk_ordinal = ntohl( *big_endian_int );

				// Current chunk size
				big_endian_int++;
				data_length = ntohl( *big_endian_int );
			}

			return bytes_read;
		}


		/// Write data to socket, synchronously. Automatically chunks data if needed
		/// \param[in] socket - ASIO socket to use. Must be connected
		/// \param[in] request_buffer - data to send
		/// \param[in] packet_size - size of data to send, in bytes
		/// \param[in] frame_ordinal - number of frame being sent
		/// \param[out] send_buffers - ASIO forces us to keep data until write completion handler.
		///		This function will allocate temporary transmission vectors and store them here.
		///		An internal handler will deallocate them, eventually. This object should live as along as socket does.
		/// \return number of written bytes
		inline size_t write( asio::ip::udp::socket &socket,
							 const char *request_buffer,
							 const size_t packet_size,
							 const int frame_ordinal,
							 std::deque< std::vector< char > >& send_buffers )
		{
			asio::error_code error;

			size_t bytes_sent = 0;
			uint32_t big_endian_int = 0;
			const char* int_buffer = reinterpret_cast< const char* >( &big_endian_int );

			int chunks_expected = (int )ceil( (double )packet_size / CHUNK_SIZE );

			auto callback = [ &send_buffers ]( const asio::error_code &error, size_t bytes_transferred ) {
				// Handle the error
				throw_exception_if_error_is_serious( error );

				// Call real callback
				send_buffers.pop_front();
			};

			// UDP limits our message size, so split it into chunks
			for( int chunk = 0; chunk < chunks_expected; chunk++ )
			{
				// Prepare buffer
				send_buffers.emplace_back( MESSAGE_SIZE );
				std::vector< char >& send_buffer = send_buffers.back();

				// Prepare header
				// Frame ordinal
				assert( int( frame_ordinal ) < ( std::numeric_limits< int32_t >::max )() );
				big_endian_int = htonl( static_cast< uint32_t >( frame_ordinal ) );
				memcpy( send_buffer.data(), int_buffer, HEADER_ITEM_SIZE );

				// Expected number of chunks
				assert( int( chunks_expected ) < ( std::numeric_limits< int32_t >::max )() );
				big_endian_int = htonl( static_cast< uint32_t >( chunks_expected ) );
				memcpy( send_buffer.data() + HEADER_ITEM_SIZE, int_buffer, HEADER_ITEM_SIZE );

				// Chunk ordinal
				big_endian_int = htonl( static_cast< uint32_t >( chunk ) );
				memcpy( send_buffer.data() + HEADER_ITEM_SIZE * 2, int_buffer, HEADER_ITEM_SIZE );

				// Current chunk size
				int chunk_size = std::min( (int )( packet_size - chunk * CHUNK_SIZE ), CHUNK_SIZE );
				assert( int( chunk_size ) < ( std::numeric_limits< int32_t >::max )() );
				big_endian_int = htonl( static_cast< uint32_t >( chunk_size ) );
				memcpy( send_buffer.data() + HEADER_ITEM_SIZE * 3, int_buffer, HEADER_ITEM_SIZE );

				// Copy data
				memcpy( send_buffer.data() + HEADER_SIZE, request_buffer + chunk * CHUNK_SIZE, chunk_size );

				// Send chunk
				socket.async_send( asio::buffer( send_buffer.data(), MESSAGE_SIZE ), callback );
				
				throw_exception_if_error_is_serious( error );
			}

			return bytes_sent;
		}


		/// Send disconnect command to server
		/// \param[in] socket - ASIO socket to use. Must be connected
		inline void stop( asio::ip::udp::socket &socket )
		{
			asio::error_code error;

			// send empty packet to indicate end-of-stream
			for( int i = 0; i < 5; i++ )
				socket.send( asio::buffer( "x", 1 ), 0, error );
			socket.shutdown( asio::ip::udp::socket::shutdown_both );
			socket.close();
			throw_exception_if_error_is_serious( error );
		}
	}  // namespace protocol
}  // namespace DG
