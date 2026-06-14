export module static_reflection:converter;

import :parser;

namespace ge::converter
{
	export struct module_partition
	{
		// For error messages
		std::string_view m_file_name{};

		parsed_file m_parse_result{};
	};

	export struct module
	{
		std::string_view m_name{};
		std::span<const module_partition> m_partitions{};
	};

	// For converting from C++ to the generated files needed to construct the runtime type registry
	export API std::string convert_module(const module& module);
}

namespace
{
	class converter_state
	{
	public:
		std::string output{};
		std::string current_scope{};
		int indent{};

		void begin_generated_file(std::string_view module_name)
		{
			write_line_fmt("import {};", module_name);
			write_line("import runtime_reflection;");
			write_line("");
			write_line("extern \"C\"");
			write_line("{");
			indent++;
			write_line("API void build_runtime_reflection(ge::refl::builders::registry_builder& builder)");
			write_line("{");
			indent++;
			write_line_fmt("builder.begin_module(\"{}\")", module_name);
		}

		void end_generated_file()
		{
			write_line(".end_module();");
			indent--;
			write_line("}");
			indent--;
			write_line("}");
			write_line("");
		}

		void write_indent()
		{
			if (indent <= 0)
			{
				return;
			}

			output.insert(output.end(), static_cast<size_t>(indent), '\t');
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

		void emit_error(const ge::source_error& error, std::string_view filename)
		{
			write_line_fmt("#line {} \"{}\"", error.m_source.m_line_number, filename);
			write_line_fmt("static_assert(false, R\"({})\");", error.m_msg);
		}
	};
}

std::string ge::converter::convert_module(const module& module)
{
	converter_state converter{};

	if (std::ranges::any_of(module.m_partitions, [](const module_partition& partition) { return !partition.m_parse_result.m_errors.empty(); }))
	{
		for (const module_partition& partition : module.m_partitions)
		{
			for (const source_error& error : partition.m_parse_result.m_errors)
			{
				converter.emit_error(error, partition.m_file_name);
			}
		}
	}
	else
	{
		converter.begin_generated_file(module.m_name);

		for (const module_partition& partition : module.m_partitions)
		{
			converter.convert_scope(partition.m_parse_result);
		}

		converter.end_generated_file();
	}

	return converter.output;
}
