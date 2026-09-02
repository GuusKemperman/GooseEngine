export module static_reflection:source_error;

import stl;

namespace ge
{
	export struct source_location
	{
		std::uint32_t m_line_number{};
		std::uint32_t m_column_number{};
	};

	export struct source_error
	{
		source_error( std::string msg, source_location source )
			: m_msg( std::move( msg ) )
			, m_source( source )
		{
		}

		std::string m_msg{};
		source_location m_source{};
	};

	export API std::string format_source_error( std::string_view file_contents, const source_error& error )
	{
		if( file_contents.empty() )
		{
			return error.m_msg;
		}

		int start_line = std::max( 0, static_cast< int >( error.m_source.m_line_number ) - 3 );

		std::string formatted = "\n";

		for( auto [ index, line ] : std::views::split( file_contents, '\n' ) | std::views::drop( start_line )
										| std::views::take( error.m_source.m_line_number - start_line ) | std::views::enumerate )
		{
			formatted += std::format( "{:6} | {}\n", start_line + index, std::string_view{ line.begin(), line.end() } );
		}

		// + to account for us inserting the line number, and - since that spot will be taken by the '^'
		formatted.insert( formatted.end(), error.m_source.m_column_number + 9 - 1, ' ' );
		formatted.append( "^\n" );

		formatted.append( error.m_msg );
		return formatted;
	}
} // namespace ge
