///////////////////////////////////////////////////////////////////////////////
/// \file dg_model_api.h 
/// \brief DG Client API for model inference
///
/// This file contains declaration of DG Client API functions and classes
/// for model inference
///

// Copyright DeGirum Corporation 2021
// All rights reserved


//
// ****  ATTENTION!!! ****
//
// This file is used to build client documentation using Doxygen.
// Please clearly describe all public entities declared in this file,
// using full and grammatically correct sentences, avoiding acronyms,
// not omitting articles.
//


#ifndef DG_AI_MODEL_API_H
#define DG_AI_MODEL_API_H

#include <string>
#include <memory>
#include <vector>
#include <set>
#include <map>
#include <functional>
#include <cstdint>
#include "Utilities/dg_client_structs.h"

#ifdef FRAMEWORK_PATH
	#define DG_CLIENT_BUILD	// to use only client-side model parameters in dg_model_parameters.h
#endif
#include "Utilities/dg_model_parameters.h"
#ifdef FRAMEWORK_PATH
	#undef DG_CLIENT_BUILD
#endif


/// DG is DeGirum namespace which groups all DeGirum software declarations
namespace DG 
{
	
	/// Get the version of the library.
	/// \return library version in the following format: MAJOR.MINOR.REVISION.GIT_REVISION
	std::string versionGet();

	/// Get a list of supported models from AI server.
	/// In case of server connection errors throws std::exception.
	/// \param[in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	/// \param[out] modelzoo_list is the output vector of ModelInfo structures; each structure
	/// contains essential model attributes such as model ID, model name, input width and input height.
	/// The ModelInfo structure extracted from this vector can be used as a parameter for construction
	/// of AIModel and AIModelAsync instances.
	void modelzooListGet( const std::string &server, std::vector< ModelInfo > &modelzoo_list );

	/// Return host system information dictionary.
	/// \param[in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	json systemInfo( const std::string &server );

	/// AI server tracing facility management
	/// \param[in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	/// \param[in] req - management request
	/// \return results of management request completion (request-specific)
	json traceManage( const std::string &server, const json &req );

	/// AI server model zoo management
	/// \param[in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	/// \param[in] req - management request
	/// \return results of management request completion (request-specific)
	json modelZooManage( const std::string &server, const json &req );

	/// Send shutdown request to AI server
	/// [in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	void shutdown( const std::string &server );

	/// Model query structure used to search models on AI server which match a set of provided model attributes
	struct ModelQuery
	{
		/// Tri-state boolean enum
		enum TriState
		{
			Yes,								//!< yes / true
			No,									//!< no / false
			Dont_care							//!< not set / don't care
		};

		std::string model_name;					//!< any part of the model name (mandatory parameter)
		std::string device_type;				//!< device type to use for inference (optional, can be empty if don't care)
		std::string runtime_agent;				//!< runtime agent to use for inference (optional, can be empty if don't care)
		TriState model_quantized = Dont_care;	//!< model should be quantized
		TriState model_pruned = Dont_care;		//!< model should be pruned/sparse (not dense)
	};

	
	/// Find a model on a given AI server which matches given model query.
	/// \param[in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	/// \param[in] query is the ModelQuery structure of model parameters to look for.
	/// \return model descriptor to be used in AIModel and AIModelAsync constructors.
	/// If the model is not found, the model descriptor contains an empty model name.
	ModelInfo modelFind( const std::string &server, const ModelQuery &query );

	/// Check given AI server Json response for run-time errors. If AI server reported run-time error
	/// in the given Json response, then this function extracts and returns the error message string from 
	/// the provided Json array, otherwise it returns an empty string.
	std::string errorCheck( const json &json_response );

	/// Get model label dictionary
	/// \param[in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	/// \param[in] model_name specifies the AI model.
	/// To obtain valid model name, either modelFind() or modelzooListGet() functions should be used.
	/// \return JSON object containing model label dictionary
	json labelDictionary( const std::string &server, const std::string &model_name );

	/// Ping server with an instantaneous command
	/// \param[in] server is a string specifying server domain name/IP address and port.
	/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
	/// return true if no error occurred during the ping
	bool serverPing( const std::string &server );

	/// Enumerator showing server status as detected by the below function
	enum class DetectionStatus
	{
		OK,					//!< Server fully operational
		ProtocolMismatch,	//!< Server is up, but is outdated
		HostAlive			//!< Hardware is alive, but no server found. Unimplemented!
	};

	/// Detect all ORCA servers on a given subnet
	/// \param[in] root_ip is an IP address in the network, formatted as "xxx.xxx.xxx.xxx[:port]". The port part is optional. If included, all servers will be scanned on the port.
	///		If omitted, the default port 8778 will be used.
	/// \param[in] subnet_mask specifies the subnet with the associated IP, formatted as "xxx.xxx.xxx.xxx", where "xxx" is either 255 for fixed bits or 0 for variable bits.
	/// \return Vector of pairs containing IPs of ORCA servers on the network and their statuses
	std::vector< std::tuple< std::string, DetectionStatus > > detectSubnetServers( const std::string& root_ip, const std::string& subnet_mask );

	/// Detect all ORCA servers with hostnames generated from a prefix and a range
	/// \param[in] prefix is a string prefix, formatted as "nnnn[:port]". This will be used to generate two sets of hostnames, one with the pattern
	/// "nnnn#", and one "nnnn###". For example, the prefix "farm", range_start 1, range_end 2 and numeral_width 3 will scan the hosts "farm1",
	/// "farm001", "farm2", "farm002". Setting numeral_width to 0 disables pattern generation, and will just scan the prefix. The port part is
	/// optional. If included, all servers will be scanned on the port. If omitted, the default port 8778 will be used.
	/// \param[in] range_start is the lowest numeral to be applied to the pattern
	/// \param[in] range_end is the highest numeral to be applied to the pattern, inclusive
	/// \param[in] numeral_width is the width of the numeral in the padded set. This is optional, set to 3 by default. Set to 0 to disable pattern generation.
	/// \return Vector of strings containing hostnames of ORCA servers on the network.
	std::vector< std::tuple< std::string, DG::DetectionStatus > > detectHostnameServers( const std::string& prefix, const int range_start, const int range_end, const int numeral_width = 3 );


	class Client;	// forward declaration

	/// \brief AIModel is DeGirum AI client API class for simple non-pipelined sequential inference.
	///
	/// This class is used to perform AI model inference on AI server in a simple non-pipelined 
	/// sequential manner.
	/// For more efficient (but somewhat more complex) asynchronous pipelined inference use AIModelAsync class.
	///
	/// On construction this class performs connection to AI server, selection of AI model, and
	/// setting model run-time parameters.
	///
	/// Once constructed, it can be used to perform sequential AI inference by invoking predict() method.
	/// The predict() methods accept the input frame data in various formats and return the inference results.
	/// These are blocking methods, i.e. they return execution only when the inference of an input frame is complete.
	class AIModel
	{
	public:

		/// Constructor. Performs connection to AI server, selection of AI model, and optionally
		/// setting model run-time parameters.
		/// In case of server connection errors throws std::exception.
		/// \param[in] server is a string specifying server domain name/IP address and port.
		/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
		/// \param[in] model_name specifies the AI model to be used for inference.
		/// To obtain valid model name, either modelFind() or modelzooListGet() functions should be used.
		/// \param[in] model_params is runtime parameter collection, which defines the model runtime behavior.
		/// This is optional parameter: if not specified, then default runtime parameters
		/// (as defined in the model zoo) are used.
		/// ModelParamsWriter class instance can be used to conveniently define runtime parameters.
		/// \param[in] connection_timeout_ms is the AI server connection timeout in milliseconds.
		/// This is optional parameter: by default it is set to 10 sec.
		explicit AIModel(
			const std::string &server,
			const std::string &model_name,
			const ModelParamsReadAccess &model_params = ModelParamsReadAccess( {} ),
			size_t connection_timeout_ms = 10 );

		// Deleted copy constructor and copy assignment operator
		AIModel( const AIModel & ) = delete;
		AIModel &operator=( const AIModel & ) = delete;

		/// Destructor. Closes connection to AI server.
		~AIModel();

		/// Run the AI inference on provided byte array. The byte array contains the frame data,
		/// which depends on selected frame format. It can be either JPEG or bitmap depending on the model parameters.
		/// In case of errors throws std::exception.
		/// This is blocking method, i.e. it returns execution only when the inference of an input frame is complete.
		/// \param[in] data is a vector of input data for each model input where each data element is a vector of bytes.
		/// \param[out] json_response is the result of the inference. The response format depends on the model post-processor type.
		void predict( std::vector< std::vector< char > > &data, json &json_response );

	private:
		std::shared_ptr< Client > m_client;		//!< client protocol handler
	};


	/// \brief AIModelAsync is DeGirum AI client API class for efficient pipelined asynchronous inference.
	///
	/// This class is used to perform AI model inference on AI server in efficient pipelined
	/// asynchronous manner using the mechanism of callbacks.
	/// For simple (but less efficient) synchronous non-pipelined inference use AIModel class.
	///
	/// On construction this class performs connection to AI server, selection of AI model, 
	/// setting model run-time parameters, and installation of the client callback function.
	/// The client callback function is used to pass the inference results from the AI server to the client code.
	///
	/// Once constructed, it can be used to perform asynchronous AI inference by invoking predict() method.
	/// The predict() methods accept the input frame data and initiate the inference on the AI server.
	/// Each of those methods is a non-blocking method, i.e. it returns execution immediately after posting the frame data to the AI server.
	/// This allows calling those methods in a loop without waiting for the inference results, achieving the maximum AI server utilization.
	/// Once the inference of a frame is complete, and the inference result is received from the AI server, the client callback is invoked
	/// to dispatch the inference result. Such result handling via callback mechanism is performed in a thread,
	/// separate from the main execution thread. It means that the client callback function is called in asynchronous manner,
	/// thus the name of the class.
	class AIModelAsync
	{
	public:

		/// User callback type. The callback is called asynchronously from the main execution thread
		/// as soon as prediction result is ready.
		/// Consecutive prediction result in a form of Json array is passed as the inference_result argument.
		/// Corresponding frame info string (provided to predict() call) is passed as the frame_info argument.
		using callback_t = std::function< void( const json &inference_result, const std::string &frame_info ) >;

		/// Constructor. Performs connection to AI server, selection of AI model, installing client callback,
		/// and optionally setting model run-time parameters.
		/// In case of server connection errors throws std::exception.
		/// \param[in] server is a string specifying server domain name/IP address and port.
		/// Format: "domain_name:port" or "xxx.xxx.xxx.xxx:port". If port is omitted, the default port is 8778.
		/// \param[in] model_name specifies the AI model to be used for inference.
		/// To obtain valid model name, either modelFind() or modelzooListGet() functions should be used.
		/// \param[in] callback is user callback functional, which will be called asynchronously from the main
		/// execution thread as soon as prediction result is ready.
		/// \param[in] model_params is runtime parameter collection, which defines the model runtime behavior.
		/// This is optional parameter: if not specified, then default runtime parameters
		/// (as defined in the model zoo) are used.
		/// ModelParamsWriter class instance can be used to conveniently define runtime parameters.
		/// \param[in] frame_queue_depth is the depth of the internal frame queue.  If predict() methods are invoked
		/// too often and the number of non-processed (aka "outstanding") frames exceeds this parameter, the
		/// consecutive call to any predict() method will be blocked until the number of outstanding frames in the
		/// queue becomes smaller than the queue depth thus allowing to post one more frame.
		/// This is optional parameter: by default it is set to 8 frames.
		/// \param[in] connection_timeout_ms is the AI server connection timeout in milliseconds.
		/// This is optional parameter: by default it is set to 10 sec.
		/// \param[in] inference_timeout_ms is the AI server inference timeout in milliseconds.
		/// This is optional parameter: by default it is set to 180 sec.
		explicit AIModelAsync(
			const std::string &server,
			const std::string &model_name,
			callback_t callback,
			const ModelParamsReadAccess &model_params = ModelParamsReadAccess( {} ),
			size_t frame_queue_depth = 8,
			size_t connection_timeout_ms = 10000,
			size_t inference_timeout_ms = 180000
			);

		// Deleted copy constructor and copy assignment operator
		AIModelAsync( const AIModelAsync & ) = delete;
		AIModelAsync &operator=( const AIModelAsync & ) = delete;

		/// Destructor.
		/// Waits until all outstanding results are received and then closes the connection to AI server.
		/// Note: in case of server runtime error, all frames posted after that error was detected,
		/// will not be processed.
		~AIModelAsync();

		/// Set user callback
		/// \param[in] callback is user callback functional, which will be called asynchronously from the main
		/// execution thread as soon as prediction result is ready.
		void setCallback( callback_t callback );

		/// Start the inference on given byte data vector. The byte vector contains the frame data,
		/// which depends on selected frame format. It can be either JPEG or bitmap depending on the model parameters.
		/// In case of errors throws std::exception.
		/// This is non-blocking call meaning that it returns execution immediately after posting the frame data to the AI server.
		/// \param[in] data is a vector of input data for each model input where each data element is a vector of bytes.
		/// \param[in] frame_info is optional frame information string to be passed to the client callback along with the frame result.
		/// You can pass arbitrary information as frame info. This simplifies matching results to frames in the client callback.
		void predict( std::vector< std::vector< char > > &data, const std::string &frame_info = "" );

		/// Wait for completion of all outstanding inferences.
		/// This is blocking call: it returns when all outstanding frames are processed by AI server and all results
		/// are dispatched via client callback.
		/// You can continue calling predict() after call to this method - frame processing will restart automatically.
		void waitCompletion();

		/// Get the number of outstanding inference results posted so far.
		int outstandingResultsCountGet() const;

		/// If ever during consecutive calls to predict() methods AI server reported a run-time error, then
		/// this method will return the error message string, otherwise it returns an empty string.
		/// Note: in case of server runtime error, all frames posted after that error was detected,
		/// will not be processed.
		std::string lastError() const;

	private:
		std::shared_ptr< Client > m_client;		//!< client protocol handler
	};
}

#endif
