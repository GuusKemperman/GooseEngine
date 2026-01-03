export module parser;

export import :tokeniser;
import utils;

namespace ge
{
	export enum class parsed_access_type
	{
		public_access,
		private_access,
		protected_access,
	};

	export enum class parsed_type_type
	{
		class_type,
		struct_type,
	};

	export struct parsed_type;

	export struct parsed_data
	{
		std::string m_attributes{};
		std::string m_name{};
		std::string m_type{};

		bool m_has_static_keyword{};
		bool m_has_inline_keyword{};

		parsed_access_type m_access{};
	};

	export struct parsed_func
	{
		std::string m_attributes{};
		std::string m_name{};
		std::string m_return_type{};
		std::vector<parsed_data> m_parameters{};
		
		bool m_has_static_keyword{};
		bool m_has_inline_keyword{};
		bool m_has_virtual_keyword{};
	
		parsed_access_type m_access{};
	};

	export struct parsed_scope
	{
		std::string m_name{};

		std::vector<parsed_func> m_funcs{};
		std::vector<parsed_data> m_data{};

		std::vector<unique_ref<parsed_scope>> m_namespaces{};
		std::vector<unique_ref<parsed_type>> m_types{};
	};

	struct parsed_base
	{
		std::string m_name{};
		parsed_access_type m_access{};
	};

	struct parsed_type : parsed_scope
	{
		std::string m_attributes{};
		parsed_type_type m_type{};
		std::vector<parsed_base> m_base_types{};
		parsed_access_type m_access{};
	};

	export struct parsed_file : parsed_scope
	{
	};

	export class parser
	{
	public:
		static constexpr std::string_view s_refl_func = "REFL_FUNC";
		static constexpr std::string_view s_refl_data = "REFL_DATA";
		static constexpr std::string_view s_refl_class = "REFL_TYPE";

		enum class parse_state
		{
			waiting_for_refl,
			waiting_for_attrib_opening_parentheses,
			waiting_for_attrib_closing_parentheses,
			waiting_for_data_type_and_name,
			waiting_for_func_return_type_and_name,
			waiting_for_func_param_closing_parentheses,
			waiting_for_namespace_opening_bracket,
			waiting_for_type_inheritance_list,
			waiting_for_type_opening_bracket,
		};
		parse_state m_state{};

		struct scope_stack_entry
		{
			std::reference_wrapper<parsed_scope> m_parsed_scope;
			int m_curly_brackets_count_before_scope{};
			parsed_access_type m_current_access_level{};
		};
		std::stack<scope_stack_entry> m_scope_stack{};

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
							&& (it->m_flag == token::flag::white_space || it->m_str == "," || it->m_str == ";" || it->m_str == "{"))
						{
							break;
						}

						type += it->m_str;
					}

					return type;
				};

			auto retrieve_name_and_previous_tokens =
				[&](std::string_view end_tokens, token_iterator& it, auto& target, const auto& receive_keyword_or_type) -> bool
				{
					bool success = false;

					auto function_start_it = it;
					auto function_name_it = it;
					for (; it != tokeniser.end(); ++it)
					{
						if (end_tokens.contains(it->m_str)
							&& it.template_bracket_count() == 0)
						{
							success = true;
							break;
						}

						if (it->m_flag != token::flag::valid_identifier)
						{
							continue;
						}

						target.m_name = it->m_str;
						function_name_it = it;
					}

					for (auto current = function_start_it; current != function_name_it; ++current)
					{
						receive_keyword_or_type(current);
					}

					return success;
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
						parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
						parsed_func& func = parent.m_funcs.emplace_back();

						m_state = parse_state::waiting_for_attrib_opening_parentheses;
						m_attributes_target = &func.m_attributes;
						m_state_after_attributes_found = parse_state::waiting_for_func_return_type_and_name;
						
						func.m_access = m_scope_stack.top().m_current_access_level;
						break;
					}

					if (it->m_str == s_refl_data)
					{
						parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
						parsed_data& data = parent.m_data.emplace_back();

						m_state = parse_state::waiting_for_attrib_opening_parentheses;
						m_attributes_target = &data.m_attributes;
						m_state_after_attributes_found = parse_state::waiting_for_data_type_and_name;

						data.m_access = m_scope_stack.top().m_current_access_level;
						break;
					}

					if (it->m_str == s_refl_class)
					{
						parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
						parsed_type& type = parent.m_types.emplace_back(make_unique_ref<parsed_type>());

						m_scope_stack.emplace(type, it.curly_bracket_count());
						m_state = parse_state::waiting_for_attrib_opening_parentheses;
						m_attributes_target = &type.m_attributes;
						m_state_after_attributes_found = parse_state::waiting_for_type_opening_bracket;

						type.m_access = m_scope_stack.top().m_current_access_level;
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
						break;
					}

					if (it->m_str == "private")
					{
						m_scope_stack.top().m_current_access_level = parsed_access_type::private_access;
						break;
					}

					if (it->m_str == "protected")
					{
						m_scope_stack.top().m_current_access_level = parsed_access_type::protected_access;
						break;
					}

					if (it->m_str == "public")
					{
						m_scope_stack.top().m_current_access_level = parsed_access_type::public_access;
						break;
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
					parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();
					if (retrieve_name_and_previous_tokens("(", it, func,
						[&](token_iterator& iterator)
						{
							if (iterator->m_str == "inline")
							{
								func.m_has_inline_keyword = true;
								return;
							}

							if (iterator->m_str == "static")
							{
								func.m_has_static_keyword = true;
								return;
							}

							if (iterator->m_str == "virtual")
							{
								func.m_has_virtual_keyword = true;
								return;
							}
							
							func.m_return_type = parse_until_type_end(iterator);
						}))
					{
						m_state = parse_state::waiting_for_func_param_closing_parentheses;
						m_parentheses_count_before_parameters = it.parentheses_count() - 1;
					}
					break;
				}
				case parse_state::waiting_for_func_param_closing_parentheses:
				{
					parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();

					if (it->m_str == ","
						&& it.parentheses_count() == m_parentheses_count_before_parameters + 1)
					{
						func.m_parameters.emplace_back();
						break;
					}

					if (it->m_str == ")"
						&& it.parentheses_count() == m_parentheses_count_before_parameters)
					{
						m_state = parse_state::waiting_for_refl;
						break;
					}

					if (func.m_parameters.empty())
					{
						func.m_parameters.emplace_back();
					}

					parsed_data& param = func.m_parameters.back();

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
				case parse_state::waiting_for_data_type_and_name:
				{
					parsed_data& data = m_scope_stack.top().m_parsed_scope.get().m_data.back();
					if (retrieve_name_and_previous_tokens(";={(", it, data,
						[&](token_iterator& iterator)
						{
							if (iterator->m_str == "inline")
							{
								data.m_has_inline_keyword = true;
								return;
							}

							if (iterator->m_str == "static")
							{
								data.m_has_static_keyword = true;
								return;
							}

							data.m_type = parse_until_type_end(iterator);
						}))
					{
						m_state = parse_state::waiting_for_refl;
					}
					break;
				}
				case parse_state::waiting_for_type_inheritance_list:
				{
					parsed_type& type = static_cast<parsed_type&>(m_scope_stack.top().m_parsed_scope.get());

					if (it->m_str == "," || type.m_base_types.empty() || !type.m_base_types.back().m_name.empty())
					{
						type.m_base_types.emplace_back();
					}

					parsed_base& base = type.m_base_types.back();

					if (it->m_str == "public")
					{
						base.m_access = parsed_access_type::public_access;
						break;
					}

					if (it->m_str == "protected")
					{
						base.m_access = parsed_access_type::protected_access;
						break;
					}

					if (it->m_str == "private")
					{
						base.m_access = parsed_access_type::private_access;
						break;
					}

					if (it->m_flag == token::flag::valid_identifier)
					{
						base.m_name = parse_until_type_end(it);
						break;
					}

					[[fallthrough]];
				}
				case parse_state::waiting_for_type_opening_bracket:
				{
					if (it->m_str == "{")
					{
						m_state = parse_state::waiting_for_refl;
						break;
					}

					if (it->m_str == ":")
					{
						m_state = parse_state::waiting_for_type_inheritance_list;
						break;
					}

					parsed_type& type = static_cast<parsed_type&>(m_scope_stack.top().m_parsed_scope.get());

					if (it->m_str == "struct")
					{
						type.m_type = parsed_type_type::struct_type;
						break;
					}

					if (it->m_str == "class")
					{
						type.m_type = parsed_type_type::class_type;
						m_scope_stack.top().m_current_access_level = parsed_access_type::private_access;
						break;
					}

					type.m_name = it->m_str;
					break;
				}
				}
			}

			return parsed_file;
		}
	};
}
