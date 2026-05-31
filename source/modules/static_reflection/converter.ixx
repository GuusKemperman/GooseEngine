export module static_reflection:converter;

export import :parser;

namespace ge
{

	//import runtime_reflection;

	//void generated_registry_builder(ge::refl::builder::registry_builder& builder)
	//{
	//	builder.begin_module(\"{}\")	



	//		.end_module();
	//}

	class converter
	{
	public:

		bool begin_module(std::ostream& stream, std::string_view module_name)
		{
			stream << R"(
			import runtime_reflection;

			void generated_registry_builder(ge::refl::builder::registry_builder & builder)
			{
				builder.begin_module(")"{}\")	



		}

		std::string convert_to_builder(std::string_view source_content)
		{
			// TODO match generated source with original source
			// TODO re-use parser

			const std::expected<parsed_file, std::string> result = parse(source_content);

			if (!result.has_value())
			{
				return std::format("static_assert(false, R\"({})\"", result.error());
			}

			const parsed_file& parsed = result.value();

			// Assume module part is already there:

			for (parsed.)

		}
		
	private:
		// TODO store parser and tokeniser here, re-use the buffers.
	};
}