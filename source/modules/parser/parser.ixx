export module parser;

export import :tokeniser;

namespace ge
{
	export struct parsed_data
	{
		std::string m_attributes{};
		std::string m_name{};
		std::string m_type{};
	};

	export struct parsed_func
	{
		bool m_has_static_keyword{};
		bool m_has_inline_keyword{};
		bool m_has_virtual_keyword{};
		std::string m_attributes{};
		std::string m_name{};
		std::string m_return_type{};
		std::vector<parsed_data> m_parameters{};
	};

	export struct parsed_file
	{
		std::vector<parsed_func> m_funcs{};
	};

	export class parser
	{
	public:
		static constexpr std::string_view s_refl_func = "REFL_FUNC";

		enum class parse_state
		{
			waiting_for_refl,
			waiting_for_attrib_opening_parentheses,
			waiting_for_attrib_closing_parentheses,
			waiting_for_func_return_type_and_name,
			waiting_for_func_param_opening_parentheses,
			waiting_for_func_param_closing_parentheses,

		};
		parse_state m_state{};

		std::optional<parsed_func> m_currently_parsing_func{};
		int m_parentheses_count_before_parameters{};

		std::string* m_attributes_target{};
		parse_state m_state_after_attributes_found{};

		int m_parentheses_count{};

		API parsed_file parse(std::string_view file)
		{
			tokeniser tokeniser{ file };

			auto parse_until_type_end =
				[&](token_iterator& it)
				{
					std::string type{};
					int template_bracket_count = 0;

					for (; it != tokeniser.end(); ++it)
					{
						if (template_bracket_count == 0
							&& it->m_flag== token::flag::white_space)
						{
							break;
						}

						if (it->m_str == "<")
						{
							template_bracket_count++;
						}
						else if (it->m_str == ">")
						{
							template_bracket_count--;
						}

						type += it->m_str;
					}

					if (template_bracket_count != 0)
					{
						throw std::invalid_argument{ "\"<\"//\">\" mismatch" };
					}

					return type;
				};

			parsed_file parsed_file{};
			for (auto it = tokeniser.begin(); it != tokeniser.end(); ++it)
			{
				if (it->m_flag== token::flag::comment
					|| it->m_flag== token::flag::attribute
					|| it->m_flag== token::flag::white_space)
				{
					continue;
				}

				if (it->m_str == "(")
				{
					m_parentheses_count++;
				}
				else if (it->m_str == ")")
				{
					m_parentheses_count--;
				}

				switch (m_state)
				{
				case parse_state::waiting_for_refl:
					if (it->m_str == s_refl_func)
					{
						m_state = parse_state::waiting_for_attrib_opening_parentheses;
						m_state_after_attributes_found = parse_state::waiting_for_func_return_type_and_name;

						m_currently_parsing_func.emplace();
						m_attributes_target = &m_currently_parsing_func->m_attributes;
						break;
					}
					break;
				case parse_state::waiting_for_attrib_opening_parentheses:
					if (it->m_str == "(")
					{
						m_state = parse_state::waiting_for_attrib_closing_parentheses;
						m_parentheses_count_before_parameters = m_parentheses_count - 1;
					}
					break;
				case parse_state::waiting_for_attrib_closing_parentheses:
					if (it->m_str == ")"
						&& m_parentheses_count == m_parentheses_count_before_parameters)
					{
						m_state = m_state_after_attributes_found;
						break;
					}

					*m_attributes_target += it->m_str;
					break;

				case parse_state::waiting_for_func_return_type_and_name:

					// Parse until we hit an opening (, assume that
					// whatever is before that is the function name, 
					// and that whatever is before that is the return type.
					for (auto current = it; current != tokeniser.end(); ++current)
					{
						if (current->m_str == "(")
						{
							break;
						}
					
						if (current->m_flag != token::flag::valid_identifier)
						{
							continue;;
						}

						m_currently_parsing_func->m_return_type = std::exchange(m_currently_parsing_func->m_name, parse_until_type_end(current));
					}

					m_state = parse_state::waiting_for_func_param_opening_parentheses;
					[[fallthrough]];
				case parse_state::waiting_for_func_param_opening_parentheses:

					if (it->m_str == "inline")
					{
						m_currently_parsing_func->m_has_inline_keyword = true;
						break;
					}

					if (it->m_str == "static")
					{
						m_currently_parsing_func->m_has_static_keyword = true;
						break;
					}

					if (it->m_str == "virtual")
					{
						m_currently_parsing_func->m_has_virtual_keyword = true;
						break;
					}

					if (it->m_str == "(")
					{
						m_state = parse_state::waiting_for_func_param_closing_parentheses;
						m_parentheses_count_before_parameters = m_parentheses_count - 1;
					}
					break;
				case parse_state::waiting_for_func_param_closing_parentheses:

					if (it->m_str == ","
						&& m_parentheses_count == m_parentheses_count_before_parameters + 1)
					{
						m_currently_parsing_func->m_parameters.emplace_back();
						break;
					}

					if (it->m_str == ")"
						&& m_parentheses_count == m_parentheses_count_before_parameters)
					{

						m_state = parse_state::waiting_for_refl;
						parsed_file.m_funcs.emplace_back(*m_currently_parsing_func);
						m_currently_parsing_func.reset();
						break;
					}

					if (m_currently_parsing_func->m_parameters.empty())
					{
						m_currently_parsing_func->m_parameters.emplace_back();
					}

					parsed_data& param = m_currently_parsing_func->m_parameters.back();

					if (param.m_type.empty())
					{
						param.m_type = parse_until_type_end(it);
						break;
					}

					if (param.m_name.empty())
					{
						param.m_name = it->m_str;
					}

					break;
				}
			}

			return parsed_file;
		}
	};
}
