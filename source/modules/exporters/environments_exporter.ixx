export module exporters:environments;

import export_core;
import runtime_reflection;
import core_traits;
import stl;

namespace ge::exporter
{
	using environments_map = std::unordered_map< refl::type_id, void* >;

	using environments_query = refl::type_query::read< traits::environment >;

	environments_map export_environments( const environments_query& input, pack_writer& pack_writer )
	{
		environments_map map{};

		for( auto [ type, trait ] : input )
		{
			std::byte* address = pack_writer.reserve_bytes( trait.m_size, trait.m_alignment );
			trait.m_construct_at( address );
			map.emplace( type.m_id, address );
		}

		return map;
	}
} // namespace ge::exporter
