
export module static_reflection:converter;

export import :parser;

namespace ge
{
	export class converter
	{
	public:

		void begin_module(std::ostream& stream, std::string_view module_name)
		{
			write_line(stream, "import runtime_reflection");
			write_line(stream, "");
			write_line(stream, "void build_runtime_reflection(ge::refl::builder::registry_builder& builder)");
			write_line(stream, "{");
			indent++;
			write_line_fmt(stream, "builder.begin_module(\"{}\")", module_name);
			indent++;
		}

		void convert_to_builder(std::ostream& stream, std::string_view source_content)
		{
			// TODO match generated source with original source
			// TODO re-use parser

			const std::expected<parsed_file, std::string> result = parse(source_content);

			if (!result.has_value())
			{
				emit_error(stream, result.error());
				return;
			}

			const parsed_file& parsed = result.value();

			for (const parsed_type& type : parsed.m_types)
			{
				convert_type(stream, type);
			}
		}

		void end_module(std::ostream& stream)
		{
			indent--;
			write_line(stream, ".end_module();");
			indent--;
			write_line(stream, "}");
		}

	private:
		int indent = 0;

		void write_line(std::ostream& stream, std::string_view line)
		{
			write_indent(stream);
			stream << line << '\n';
		}

		template<typename... Args>
		void write_line_fmt(std::ostream& stream, std::format_string<Args...> fmt, Args&&... fmtArgs)
		{
			write_indent(stream);
			stream << std::format(fmt, std::forward<Args>(fmtArgs)...) << '\n';
		}

		void write_indent(std::ostream& stream)
		{
			for (int i = 0; i < indent; i++)
			{
				stream.put('\t');
			}
		}

		void convert_type(std::ostream& stream, const parsed_type& type)
		{
			write_line_fmt(stream, ".begin_type<{0}>(\"{0}\")", type.m_name);
			indent++;
			
			indent--;
			write_line(stream, ".end_type()");
		}
		
		void emit_error(std::ostream& stream, std::string_view error)
		{
			stream << "static_assert(false, R\"( " << error << ")\");";
		}

		// TODO store parser and tokeniser here, re-use the buffers.
	};
}