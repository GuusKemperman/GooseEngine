
import static_reflection;
import stl;

#include <assert.h>

int main( int arg_c, const char** arg_v )
{
	assert(arg_c >= 3);
	const std::string_view module_name = arg_v[1];
	const std::string_view output_file = arg_v[2];

	std::string output = ge::begin_generated_file(module_name);

	for ( const char** it = arg_v + 2; it < arg_v + arg_c; ++it )
	{
		const std::string contents = [it]()
			{
				std::ifstream stream{ *it };
				assert(stream.is_open());
				return std::string{ std::istreambuf_iterator{ stream }, std::istreambuf_iterator<char>{} };
			}();

		ge::token_range tokenized{ contents };
		ge::parsed_file parsed = ge::parse(tokenized);

		output += ge::convert_source_file(parsed);
	}

	output += ge::end_generated_file();

	std::ofstream output_stream{ output_file.data() };
	assert(output_stream.is_open());
	output_stream << output;
	return 0;
}
