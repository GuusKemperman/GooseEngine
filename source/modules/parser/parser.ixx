export module parser;

export import :tokeniser;

namespace ge
{
	export struct parsed_data
	{
		std::string m_name{};
		std::string m_type{};
	};

	export struct parsed_func
	{
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
			waiting_for_func_return_type,
			waiting_for_func_return_type_end,
			waiting_for_func_name,
			waiting_for_func_param_opening_parentheses,
			waiting_for_func_param_closing_parentheses,
			
		};
		parse_state m_state{};

		std::optional<parsed_func> m_currently_parsing_func{};
		int m_parentheses_count_before_parameters{};

		int m_parentheses_count{};

		API parsed_file parse(std::string_view file)
		{
			parsed_file parsed_file{};
			tokeniser tokeniser{ file };

			auto parse_until_type_end = 
				[&](tokeniser::token token)
				{
					std::string type{ token.m_str };
					int template_bracket_count = 0;
					
					while (!tokeniser.m_remaining_file.empty())
					{
						token = tokeniser.consume_token();

						if (template_bracket_count == 0 
							&& token.m_flag == tokeniser::token::flag::white_space)
						{
							break;
						}

						if (token.m_str == "<")
						{
							template_bracket_count++;
						}
						else if (token.m_str == ">")
						{
							template_bracket_count--;
						}

						type += token.m_str;
					}

					if (template_bracket_count != 0)
					{
						throw std::invalid_argument{ "\"<\"//\">\" mismatch" };
					}

					return type;
				};

			while (!tokeniser.m_remaining_file.empty())
			{
				tokeniser::token token = tokeniser.consume_token();

				if (token.m_flag == tokeniser::token::flag::comment
					|| token.m_flag == tokeniser::token::flag::attribute
					|| token.m_flag == tokeniser::token::flag::white_space)
				{
					continue;
				}

				if (token.m_str == "(")
				{
					m_parentheses_count++;
				}
				else if (token.m_str == ")")
				{
					m_parentheses_count--;
				}

				switch (m_state)
				{
				case parse_state::waiting_for_refl:
					if (token.m_str == s_refl_func)
					{
						m_state = parse_state::waiting_for_func_return_type;
						m_currently_parsing_func.emplace();
					}
					break;
				case parse_state::waiting_for_func_return_type:
					if (token.m_flag != tokeniser::token::flag::valid_identifier)
					{
						break;
					}

					if (token.m_str == "static"
						|| token.m_str == "inline"
						|| token.m_str == "virtual"
						|| token.m_str == "extern")
					{
						break;
					}

					// Assume that this is the return type
					m_currently_parsing_func->m_return_type = parse_until_type_end(token);
					m_state = parse_state::waiting_for_func_name;
					break;
				case parse_state::waiting_for_func_name:
					m_currently_parsing_func->m_name = token.m_str;
					m_state = parse_state::waiting_for_func_param_opening_parentheses;
					m_parentheses_count_before_parameters = m_parentheses_count;
					break;
				case parse_state::waiting_for_func_param_opening_parentheses:

					if (token.m_str == "("
						&& m_parentheses_count == m_parentheses_count_before_parameters + 1)
					{
						m_state = parse_state::waiting_for_func_param_closing_parentheses;
					}

					break;
				case parse_state::waiting_for_func_param_closing_parentheses:

					if (token.m_str == ","
						&& m_parentheses_count == m_parentheses_count_before_parameters + 1)
					{
						m_currently_parsing_func->m_parameters.emplace_back();
						break;
					}

					if (token.m_str == ")"
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
						param.m_type = parse_until_type_end(token);
						break;
					}
					
					if (param.m_name.empty())
					{
						param.m_name = token.m_str;
					}

					break;
				}
			}
		
			return parsed_file;
		}
	};
}
