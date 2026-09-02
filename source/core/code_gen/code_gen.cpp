import static_reflection;
import stl;

#include <assert.h>

int main( int arg_c, const char** arg_v )
{
	assert( arg_c >= 3 );
	const std::string_view module_name = arg_v[ 1 ];
	const std::string_view output_file = arg_v[ 2 ];

	ge::converter::module module_to_convert{ .m_name = module_name };

	std::vector< ge::converter::module_partition > partitions{};
	partitions.reserve( static_cast< size_t >( arg_c ) - 3ull );

	for( const char** it = arg_v + 3; it < arg_v + arg_c; ++it )
	{
		const std::string contents = [ it ]()
		{
			std::ifstream stream{ *it };
			assert( stream.is_open() );
			return std::string{ std::istreambuf_iterator{ stream }, std::istreambuf_iterator< char >{} };
		}();

		ge::token_range tokenized{ contents };
		partitions.push_back( { .m_file_name = *it, .m_file_contents = contents, .m_parse_result = ge::parse( tokenized ) } );
	}

	module_to_convert.m_partitions = partitions;

	std::ofstream output_stream{ output_file.data() };
	assert( output_stream.is_open() );
	output_stream << ge::converter::convert_module( module_to_convert );
	return 0;
}
