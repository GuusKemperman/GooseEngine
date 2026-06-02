export module static_reflection:converter;

export import :parser;

namespace ge
{
	export class converter
	{
	public:

		API void begin_module(std::ostream& stream, std::string_view module_name)
		{
			write_line(stream, "import runtime_reflection");
			write_line(stream, "");
			write_line(stream, "void build_runtime_reflection(ge::refl::builder::registry_builder& builder)");
			write_line(stream, "{");
			indent++;
			write_line_fmt(stream, "builder.begin_module(\"{}\")", module_name);
			indent++;
		}

		API void convert_to_builder(std::ostream& stream, std::string_view source_content)
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

			convert_scope(stream, parsed);
		}

		API void end_module(std::ostream& stream)
		{
			indent--;
			write_line(stream, ".end_module();");
			indent--;
			write_line(stream, "}");
		}

	private:
		int indent = 0;
		std::string location{};

		void write_line(std::ostream& stream, std::string_view line)
		{
			write_indent(stream);
			stream << line << '\n';
		}

		void push_scope(const parsed_scope& scope)
		{
			location += scope.m_name;
			location += "::";
		}

		void pop_scope()
		{
			// pop the "::"
			location = location.substr(0, location.size() - 2);

			// pop until we find the next ':'
			while (!location.empty() && location.back() != ':')
			{
				location.pop_back();
			}
		}

		template<typename... Args>
		void write_line_fmt(std::ostream& stream, std::format_string<Args...> fmt, Args&&... fmtArgs)
		{
			write_indent(stream);
			stream << std::format(fmt, std::forward<Args>(fmtArgs)...) << '\n';
		}

		void write_traits(std::ostream& stream, std::string_view traits)
		{
			if (!traits.empty())
			{
				write_line_fmt(stream, ".add_traits({})", traits);
			}
		}

		void write_indent(std::ostream& stream)
		{
			for (int i = 0; i < indent; i++)
			{
				stream.put('\t');
			}
		}

		void convert_scope(std::ostream& stream, const parsed_scope& scope)
		{
			push_scope(scope);

			for (const parsed_type& type : scope.m_types)
			{
				convert_type(stream, type);
			}

			for (const parsed_func& func : scope.m_funcs)
			{
				convert_func(stream, func);
			}

			for (const parsed_data& data : scope.m_data)
			{
				convert_data(stream, data);
			}

			pop_scope();
		}

		void convert_func(std::ostream& stream, const parsed_func& func)
		{
			// TODO handle overloads
			write_line_fmt(stream, ".begin_func<&{0}{1}>(\"{1}\")", location, func.m_name);
			indent++;

			write_traits(stream, func.m_traits);

			indent--;
			write_line(stream, ".end_func()");
		}

		void convert_data(std::ostream& stream, const parsed_data& data)
		{
			write_line_fmt(stream, ".begin_data<&{0}{1}>(\"{1}\")", location, data.m_name);
			indent++;

			write_traits(stream, data.m_traits);

			indent--;
			write_line(stream, ".end_data()");
		}

		void convert_type(std::ostream& stream, const parsed_type& type)
		{
			write_line_fmt(stream, ".begin_type<{0}{1}>(\"{1}\")", location, type.m_name);
			indent++;

			write_traits(stream, type.m_traits);

			convert_scope(stream, type);

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
