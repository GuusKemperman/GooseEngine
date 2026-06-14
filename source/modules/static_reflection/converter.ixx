export module static_reflection:converter;

import :parser;

namespace ge
{
	// For converting from C++ to the generated files needed to construct the runtime type registry

	export API std::string begin_generated_file(std::string_view module_name)
	{
		std::string output =
			"import runtime_reflection;\n"
			"\n"
			"void build_runtime_reflection(ge::refl::builders::registry_builder& builder)\n"
			"{\n";
		return output + std::format("\tbuilder.begin_module(\"{}\")\n", module_name);
	}

	export API std::string convert_source_file(const parsed_file& file, std::optional<std::string_view> filename = std::nullopt);

	export API std::string end_generated_file()
	{
		return
			"	.end_module();\n"
			"}\n";
	}
}

namespace
{
	class converter
	{
	public:
		std::string output{};
		std::string current_scope{};
		size_t indent = 2;

	void write_indent()
	{
		output.insert(output.end(), indent, '\t');
	}

	void write_line(std::string_view line)
	{
		write_indent();
		output += line;
		output += '\n';
	}

	void push_scope(const ge::parsed_scope& scope)
	{
		current_scope += scope.m_name;
		current_scope += "::";
	}

	void pop_scope()
	{
		// pop the "::"
		current_scope = current_scope.substr(0, current_scope.size() - 2);

		// pop until we find the next ':'
		while (!current_scope.empty() && current_scope.back() != ':')
		{
			current_scope.pop_back();
		}
	}

	template<typename... Args>
	void write_line_fmt(std::format_string<Args...> fmt, Args&&... fmtArgs)
	{
		write_line(std::format(fmt, std::forward<Args>(fmtArgs)...));
	}

	void write_traits(std::string_view traits)
	{
		if (!traits.empty())
		{
			write_line_fmt(".add_traits({})", traits);
		}
	}

	void convert_scope(const ge::parsed_scope& scope)
	{
		push_scope(scope);

		for (const ge::parsed_type& type : scope.m_types)
		{
			convert_type(type);
		}

		for (const ge::parsed_func& func : scope.m_funcs)
		{
			convert_func(func);
		}

		for (const ge::parsed_data& data : scope.m_data)
		{
			convert_data(data);
		}

		pop_scope();
	}

	void convert_func(const ge::parsed_func& func)
	{
		// TODO handle overloads
		write_line_fmt(".begin_func<&{0}{1}>(\"{1}\")", current_scope, func.m_name);
		indent++;

		write_traits(func.m_traits);

		indent--;
		write_line(".end_func()");
	}

	void convert_data(const ge::parsed_data& data)
	{
		write_line_fmt(".begin_data<&{0}{1}>(\"{1}\")", current_scope, data.m_name);
		indent++;

		write_traits(data.m_traits);

		indent--;
		write_line(".end_data()");
	}

	void convert_type(const ge::parsed_type& type)
	{
		write_line_fmt(".begin_type<{0}{1}>(\"{1}\")", current_scope, type.m_name);
		indent++;

		write_traits(type.m_traits);

		convert_scope(type);

		indent--;
		write_line(".end_type()");
	}

	void emit_error(const ge::source_error& error, std::optional<std::string_view> filename)
	{
		if (filename.has_value())
		{
			write_line_fmt("#line {} \"{}\"", error.m_source.m_line_number, *filename);
		}

		write_line_fmt("static_assert(false, R\"({})\");", error.m_msg);
	}
	};
}

std::string ge::convert_source_file(const parsed_file& file, std::optional<std::string_view> filename)
{
	converter converter{};

	if (!file.m_errors.empty())
	{
		for (const source_error& err : file.m_errors)
		{
			converter.emit_error(err, filename);
		}
	}
	else
	{
		converter.convert_scope(file);
	}

	return converter.output;
}
