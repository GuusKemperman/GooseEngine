export module exporters:environments;

import export_core;
import runtime_reflection;
import core_traits;
import stl;

namespace ge::exporter
{
	struct environments
	{
		template<refl::undecorated T>
		const T* try_get()
		{ 
			auto it = m_map.find( refl::make_type_id< T >() );
			return it == m_map.end() ? nullptr : std::bit_cast< const T* >( &it->second.get() );
		}

		std::unordered_map< refl::type_id, std::reference_wrapper<const std::byte> > m_map{};
	};

	using environments_query =  refl::type_query::read< traits::environment >;

	environments export_environments( const environments_query& input, pack_writer& pack_writer )
	{
		environments map{};
		
		for( auto [type, trait] : input )
		{
			std::byte* address = pack_writer.reserve_bytes( trait.m_size, trait.m_alignment );
			trait.m_emplace( address );
			map.m_map.emplace( type.m_id, std::cref( *address ) );
		}

		return map;
	}
}