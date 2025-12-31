export module parser;

export import :tokeniser;
import utils;

namespace ge
{
	export struct parsed_type;

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

	export struct parsed_scope
	{
		std::string m_name{};

		std::vector<parsed_func> m_funcs{};
		std::vector<parsed_data> m_data{};

		std::vector<unique_ref<parsed_scope>> m_namespaces{};
		std::vector<unique_ref<parsed_type>> m_types{};
	};

	struct parsed_type : parsed_scope
	{
		std::string m_attributes{};
		std::vector<std::string> m_base_types{};
	};

	export struct parsed_file : parsed_scope
	{
	};

	export class parser
	{
	public:
		static constexpr std::string_view s_refl_func = "REFL_FUNC";
		static constexpr std::string_view s_refl_class = "REFL_TYPE";

		enum class parse_state
		{
			waiting_for_refl,
			waiting_for_attrib_opening_parentheses,
			waiting_for_attrib_closing_parentheses,
			waiting_for_func_return_type_and_name,
			waiting_for_func_param_closing_parentheses,
			waiting_for_namespace_opening_bracket,

		};
		parse_state m_state{};

		struct scope_stack_entry
		{
			std::reference_wrapper<parsed_scope> m_parsed_scope;
			int m_curly_brackets_count_before_scope{};
		};
		std::stack<scope_stack_entry> m_scope_stack{};
		
		std::optional<parsed_func> m_currently_parsing_func{};

		std::string* m_attributes_target{};
		parse_state m_state_after_attributes_found{};

		int m_parentheses_count_before_parameters{};

		API parsed_file parse(std::string_view file)
		{
			tokeniser tokeniser{ file };

			auto parse_until_type_end =
				[&](token_iterator& it)
				{
					std::string type{};

					for (; it != tokeniser.end(); ++it)
					{
						if (it.template_bracket_count() == 0
							&& it->m_flag == token::flag::white_space)
						{
							break;
						}

						type += it->m_str;
					}

					return type;
				};

			parsed_file parsed_file{};
			m_scope_stack.emplace(parsed_file, std::numeric_limits<int>::min());

			for (auto it = tokeniser.begin(); it != tokeniser.end(); ++it)
			{
				if (it->m_flag== token::flag::comment
					|| it->m_flag== token::flag::attribute
					|| it->m_flag== token::flag::white_space)
				{
					continue;
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

					if (it->m_str == "namespace")
					{
						parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
						parsed_scope& new_namespace = parent.m_namespaces.emplace_back(make_unique_ref<parsed_scope>());
						m_scope_stack.emplace(new_namespace, it.curly_bracket_count());
						m_state = parse_state::waiting_for_namespace_opening_bracket;
						break;
					}
					
					if (it->m_str == "}"
						&& m_scope_stack.size() > 1
						&& m_scope_stack.top().m_curly_brackets_count_before_scope == it.curly_bracket_count())
					{
						m_scope_stack.pop();
					}

					break;
				case parse_state::waiting_for_namespace_opening_bracket:
					if (it->m_str == "{")
					{
						m_state = parse_state::waiting_for_refl;
						break;
					}

					m_scope_stack.top().m_parsed_scope.get().m_name += it->m_str;
					break;

				case parse_state::waiting_for_attrib_opening_parentheses:
					if (it->m_str == "(")
					{
						m_state = parse_state::waiting_for_attrib_closing_parentheses;
						m_parentheses_count_before_parameters = it.parentheses_count() - 1;
					}
					break;
				case parse_state::waiting_for_attrib_closing_parentheses:
					if (it->m_str == ")"
						&& it.parentheses_count() == m_parentheses_count_before_parameters)
					{
						m_state = m_state_after_attributes_found;
						break;
					}

					*m_attributes_target += it->m_str;
					break;

				case parse_state::waiting_for_func_return_type_and_name:
				{
					auto function_start_it = it;
					auto function_name_it = it;
					for (; it != tokeniser.end(); ++it)
					{
						if (it->m_str == "("
							&& it.template_bracket_count() == 0)
						{
							m_state = parse_state::waiting_for_func_param_closing_parentheses;
							m_parentheses_count_before_parameters = it.parentheses_count() - 1;
							break;
						}

						if (it->m_flag != token::flag::valid_identifier)
						{
							continue;
						}

						m_currently_parsing_func->m_name = it->m_str;
						function_name_it = it;
					}

					for (auto current = function_start_it; current != function_name_it; ++current)
					{
						if (current->m_str == "inline")
						{
							m_currently_parsing_func->m_has_inline_keyword = true;
							break;
						}

						if (current->m_str == "static")
						{
							m_currently_parsing_func->m_has_static_keyword = true;
							break;
						}

						if (current->m_str == "virtual")
						{
							m_currently_parsing_func->m_has_virtual_keyword = true;
							break;
						}

						if (current->m_str != m_currently_parsing_func->m_name)
						{
							m_currently_parsing_func->m_return_type = parse_until_type_end(current);
						}
					}
					break;
				}
				case parse_state::waiting_for_func_param_closing_parentheses:

					if (it->m_str == ","
						&& it.parentheses_count() == m_parentheses_count_before_parameters + 1)
					{
						m_currently_parsing_func->m_parameters.emplace_back();
						break;
					}

					if (it->m_str == ")"
						&& it.parentheses_count() == m_parentheses_count_before_parameters)
					{
						m_state = parse_state::waiting_for_refl;

						parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
						parent.m_funcs.emplace_back(*std::move(m_currently_parsing_func));
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
