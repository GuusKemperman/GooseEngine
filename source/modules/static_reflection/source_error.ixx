export module static_reflection:source_error;

import stl;

namespace ge
{
	export struct source_location
	{
		std::uint32_t m_line_number{};
	};

	export struct source_error
	{
		source_error( std::string msg, source_location source )
			: m_msg( std::move( msg ) ),
			  m_source( source )
		{
		}

		std::string m_msg{};
		source_location m_source{};
	};
}
