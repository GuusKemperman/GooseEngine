export module core_traits:environment;

import runtime_reflection;

namespace ge::traits
{
	export struct environment : refl::type_trait
	{
		size_t m_size{};
		size_t m_alignment{};

		void ( *m_construct_at )( void* dst );

		template< typename T >
		void on_apply( const refl::builders::type_builder< T >& )
		{
			m_size = sizeof( T );
			m_alignment = alignof( T );
			m_construct_at = +[]( void* dst )
			{
				new( dst ) T();
			};
		}
	};
} // namespace ge::traits
