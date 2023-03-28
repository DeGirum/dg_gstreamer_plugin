///////////////////////////////////////////////////////////////////////////////
/// \file dg_client.h 
/// \brief Class, which implements client-side functionality of
///  DG client-server system
///
/// Copyright DeGirum Corporation 2021
///
/// This file contains declaration of DG::Client class:
/// which handles protocol of DG client-server system
///

#ifndef DG_TCPCLIENT_H_
#define DG_TCPCLIENT_H_

#include <vector>
#include <string>
#include <map>
#include <queue>
#include <functional>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include "Utilities/dg_socket.h"
#include "Utilities/dg_client_structs.h"
#include "Utilities/dg_json_helpers.h"

using json = nlohmann::json;

namespace DG 
{
	class StreamSocket;

	/// Client-side protocol handler of DG client-server system
	class Client final
	{
	public:
		/// User callback type. The callback is called as soon as prediction result is ready.
		/// Consecutive prediction result (Json array) is passed as the first argument.
		/// Corresponding frame info string (provided to dataSend() call) is passed as the second argument.
		using callback_t = std::function< void( const json&, const std::string& ) >;

		/// Constructor. Sets up active server using address.
		/// \param[in] server_address - server address
		/// \param[in] connection_timeout_ms - connection timeout in milliseconds
		/// \param[in] inference_timeout_ms - AI server inference timeout in milliseconds
		Client( const std::string &server_address, size_t connection_timeout_ms = 10000, size_t inference_timeout_ms = 180000 );

		Client( const Client &s ) = delete;

		/// Destructor
		~Client();

		/// Get list of models in all model zoos of all active servers
		/// \param[out] modelzoo_list - array of models in model zoos
		void modelzooListGet( std::vector<DG::ModelInfo> &modelzoo_list );

		/// Return host system information dictionary
		json systemInfo();

		/// AI server tracing facility management
		/// \param[in] req - management request
		/// \return results of management request completion (request-specific)
		json traceManage( const json &req );

		/// AI server model zoo management
		/// \param[in] req - management request
		/// \return results of management request completion (request-specific)
		json modelZooManage( const json &req );

		/// Ping server with an instantaneous command
		/// \return true if no error occurred during the ping
		bool ping();

		/// 'stream' op handler: creates and opens socket for stream of frames to be used by subsequent predict() calls
		/// \param[in] model_name - model name, which defines op destination
		/// \param[in] frame_queue_depth - the depth of internal frame queue
		/// \param[in] additional_model_parameters - additional model parameters in Json format compatible with DG Json model configuration
		void openStream( const std::string &model_name, size_t frame_queue_depth, const json &additional_model_parameters = {} );

		/// Send shutdown request to AI server
		void shutdown();

		/// Get model label dictionary
		/// \param[in] model_name specifies the AI model.
		/// \return JSON object containing model label dictionary
		json labelDictionary( const std::string &model_name );

		//
		// Synchronous prediction API
		//

		/// Run prediction on given data frame. Stream should be opened by openStream()
		/// \param[in] data - array containing frame data
		/// \param[out] output - prediction result (Json array)
		void predict( std::vector< std::vector< char > > &data, json &output );

		//
		// Asynchronous prediction API
		//

		/// Install prediction results observation callback
		/// \param[in] callback - user callback, which will be called as soon as prediction result is ready when using dataSend() methods
		void resultObserve( callback_t callback );

		/// Send given data frame for prediction. Prerequisites:
		///   stream should be opened by openStream();
		///   user callback to receive prediction results should be installed by resultObserve().
		/// On the very first frame the method launches result receiving thread. This thread asynchronously
		/// retrieves prediction results and for each result calls user callback installed by resultObserve().
		/// To terminate this thread dataEnd() should be called when no more data frames are expected.
		/// \param[in] data - array containing frame data
		/// \param[in] frame_info - optional frame information string to be passed to the callback along the frame result
		void dataSend( const std::vector< std::vector< char > > &data, const std::string &frame_info = "" );

		/// Finalize the sequence of data frames. Should be called when no more data frames are
		/// expected to terminate result receiving thread started by dataSend().
		/// NOTE: will wait until all outstanding results are received
		void dataEnd();

		/// Get # of outstanding inference results scheduled so far
		int outstandingResultsCountGet() const
		{
			return m_async_outstanding_results;
		}

		/// If ever during consecutive calls to predict() methods server reported run-time error, then
		/// this method will return error message string, otherwise it returns empty string.
		std::string lastError() const
		{
			return m_last_error;
		}

	private:

		/// Transmit command Json packet to server, receive response, parse it, and analyze for errors
		/// \param[in] source - description of the server operation initiator (for error reports only)
		/// \param[in] request - Json array with command
		/// \param[out] response - Json array with command response packet
		/// \return true, if some response was received, false otherwise
		bool transmitCommand( const std::string& source, const json& request, json& response );

		/// Transmit arbitrary string
		/// \param[in] source - description of the server command initiator (for error reports only)
		/// \param[in] request - request string
		void transmitCommand( const std::string& source, const std::string& request );

		//
		// Callback for asynchronous read. Receives incoming message size and completes read. Schedules next read, if any are outstanding.
		//
		void dataReceive();

		/// Close stream opened by openStream()
		void closeStream();

		main_protocol::io_context_t m_io_context;				//!< ASIO context
		main_protocol::socket_t m_stream_socket;				//!< socket object for streaming operation
		main_protocol::socket_t m_command_socket;				//!< socket object for sending commands
		DG::ServerAddress m_server_address;						//!< address of active server
		
		// asynchronous prediction support
		std::thread m_async_thread;								//!< result receiving thread
		callback_t m_async_result_callback;						//!< asynchronous inference result callback
		std::atomic_int m_async_outstanding_results;			//!< # of outstanding inference results scheduled so far
		std::condition_variable m_waiter;						//!< condition variable for result receiving thread synchronization
		std::atomic_bool m_async_stop;							//!< stop request for receiving thread
		std::mutex m_communication_mutex;						//!< mutex to protect the above
		uint32_t m_read_size;									//!< size of received response
		size_t m_frame_queue_depth;								//!< depth of frame queue
		std::queue< std::string > m_frame_info_queue;			//!< frame info queue
		std::string m_last_error;								//!< last prediction error (or empty)
		size_t m_connection_timeout_ms;							//!< socket connection timeout, in milliseconds
		size_t m_inference_timeout_ms;							//!< AI server inference timeout, in milliseconds
	};
}  // namespace DG

#endif
