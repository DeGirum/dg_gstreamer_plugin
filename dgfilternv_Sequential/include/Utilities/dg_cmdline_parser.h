//////////////////////////////////////////////////////////////////////
/// \file  dg_cmdline_parser.h
/// \brief command line parser class
///
/// Copyright 2021 DeGirum Corporation
///
/// This file contains implementation of simple command line parser class
///

#ifndef CORE_DG_CMDLINE_PARSER_H_
#define CORE_DG_CMDLINE_PARSER_H_

#include <vector>
#include <string>
#include <utility>
#include <algorithm>


namespace DG
{
	/// Command line parser class
	class InputParser
	{
	public:

		/// Construtor
		/// \param[in] argc - # of command line arguments
		/// \param[in] argv - array of command line arguments
		InputParser( int &argc, char **argv )
		{
			for( int ai = 1; ai < argc; ai++ )
			{
				if( isOption( argv[ ai ] ) )
				{
					const auto eq_pos = std::string( argv[ ai ] ).find( '=' );
					if( eq_pos != std::string::npos )
						m_options.push_back( { std::string( argv[ ai ], eq_pos ), std::string( argv[ ai ] + eq_pos + 1 ) } );
					else if( ai + 1 < argc && !isOption( argv[ ai + 1 ] ) )
					{
						m_options.push_back( { argv[ ai ], argv[ ai + 1 ] } );
						ai++;
					}
					else
						m_options.push_back( { argv[ ai ], "" } );
				}
				else
					m_others.push_back( argv[ ai ] );
			}
		}

		/// Get string value of argument with given name
		/// \param[in] option - argument name
		/// \param[in] default_value - default value argument not found
		/// \return value or default_value string if not found
		const std::string& getCmdOption( const std::string &option, const std::string &default_value = "" ) const
		{
			auto it = optionFind( option );
			if( it != m_options.end() )
				return it->second;
			return default_value;
		}

		/// Get integer value of argument with given name
		/// \param[in] option - argument name
		/// \param[in] default_value - default value in case argument not found
		/// \return value or default_value if not found
		int getCmdInt( const std::string &option, int default_value ) const
		{
			auto it = optionFind( option );
			if( it != m_options.end() )
				return atoi( it->second.c_str() );
			return default_value;
		}

		/// Get floating value of argument with given name
		/// \param[in] option - argument name
		/// \param[in] default_value - default value in case argument not found
		/// \return value or default_value if not found
		double getCmdDouble( const std::string &option, double default_value ) const
		{
			auto it = optionFind( option );
			if( it != m_options.end() )
				return atof( it->second.c_str() );
			return default_value;
		}

		/// Check if argument with given name exists
		/// \param[in] option - argument name
		bool cmdOptionExists( const std::string &option ) const
		{
			return optionFind( option ) != m_options.end();
		}

		/// Return the list of all non-option parameters
		std::vector< std::string > getNonOptions() const { return m_others; }

	private:
		using token_t = std::pair< std::string, std::string >;	//!< token type: .first is option name, .second is option value
			
		std::vector< token_t > m_options;		//!< array of all options
		std::vector< std::string > m_others;	//!< array of all non-options

		/// Is given token defines an option? It is, if it starts with '-' or '--'
		bool isOption( const char *arg ) const 
		{
			return arg[ 0 ] == '-';
		}

		/// Extract option name by stripping leading option prefix characters
		std::string optionName( const std::string &option ) const
		{
			int pos = 0;
			if( option[ pos ] == '-' )
			{
				pos++;
				if( option[ pos ] == '-'  )
					pos++;
			}
			return option.substr( pos );
		}

		/// Find option by name (ignoring leading option prefix characters)
		std::vector< token_t >::const_iterator optionFind( const std::string &option ) const
		{
			const std::string optName = optionName( option );
			return std::find_if( m_options.cbegin(), m_options.cend(), [&]( const token_t& arg )
				{
					return optionName( arg.first ) == optName;
				} );
		}

	};

}  // namespace DG

#endif // CORE_DG_CMDLINE_PARSER_H_
