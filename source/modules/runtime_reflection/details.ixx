export module runtime_reflection:details;

import stl;

namespace ge::refl
{
	template< typename Base >
		requires( sizeof( Base ) == sizeof( size_t ) )
	class inplace_vtable
	{
		size_t m_vtable{};

	public:
		const Base* operator->() const
		{
			return std::bit_cast< const Base* >( &m_vtable );
		}

		template< std::derived_from< Base > Derived >
			requires( sizeof( Derived ) == sizeof( Base ) )
		void set()
		{
			void* dst = &m_vtable;
			new( dst ) Derived();
		}
	};
} // namespace ge::refl
