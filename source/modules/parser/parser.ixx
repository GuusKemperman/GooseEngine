export module parser;

export import :tokeniser;
import utils;

// TODO: Consider splitting enums into separate ones
// TODO: Consider removing the '{' counter in the scopestack entry, and instead infer it from the size of the stack

namespace ge
{
	export enum class parsed_keywords
	{
		inline_keyword = 1 << 1,
		static_keyword = 1 << 2,
		virtual_keyword = 1 << 3,
		export_keyword = 1 << 4,
	};

	export API constexpr parsed_keywords operator|(parsed_keywords lhs, parsed_keywords rhs)
	{
		using key_t = std::underlying_type_t<parsed_keywords> ;
		lhs = static_cast<parsed_keywords>(
			static_cast<key_t>(lhs) |
			static_cast<key_t>(rhs));
		return lhs;
	}

	export API constexpr parsed_keywords operator&(parsed_keywords lhs, parsed_keywords rhs)
	{
		using key_t = std::underlying_type_t<parsed_keywords>;
		lhs = static_cast<parsed_keywords>(
			static_cast<key_t>(lhs) &
			static_cast<key_t>(rhs));
		return lhs;
	}

	export enum class parsed_access_specifier
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

		parsed_keywords m_keywords{};
		parsed_access_specifier m_access{};
	};

	export struct parsed_parameter
	{
		std::string m_type{};
		std::string m_name{};
	};

	export struct parsed_func
	{
		std::string m_attributes{};
		std::string m_name{};
		std::string m_return_type{};
		std::vector<parsed_parameter> m_parameters{};
		
		parsed_keywords m_keywords{};
		parsed_access_specifier m_access{};
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
		std::optional<parsed_access_specifier> m_access{};
	};

	struct parsed_type : parsed_scope
	{
		std::string m_attributes{};
		parsed_type_type m_type{};
		std::vector<parsed_base> m_base_types{};
		parsed_access_specifier m_access{};
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

		API parsed_file parse(std::string_view file);

	private:
		enum class parse_state : std::uint32_t
		{
			none,
			complete_next_state_immediately,

			reflect_namespace,
			reflect_type,
			reflect_base,
			reflect_data,
			reflect_func,
			reflect_parameter,

			check_for_next_parameter,
			check_for_next_base,

			skip_to_opening_parentheses,

			parse_type,
			parse_type_after_identifier_start_found,
			parse_type_type, // check for the 'struct' or 'class' keyword
			parse_keywords,
			parse_identifier,
			parse_attributes,
			parse_access_specifier,

			store_reflected_namespace,
			store_reflected_type,
			store_base,
			store_reflected_data,
			store_reflected_func,
			store_parameter,
		};

		struct scope_stack_entry
		{
			std::reference_wrapper<parsed_scope> m_parsed_scope;
			int m_curly_brackets_count_before_scope{};
			parsed_access_specifier m_current_access_level{};
		};

		void complete_state();

		void report_error(std::string reason) 
		{
			std::print("error: {}\n", reason);
			
			m_state_stack = {};
			m_state_stack.emplace(parse_state::none);
		}

		void push_state(parse_state state);
		void switch_state(parse_state state);

		void on_event(parse_state state);
		
		void on_state_push_requested(parse_state state);
		bool on_state_receive_token(parse_state state, token_iterator it);
		void on_state_completed(parse_state state);

		// Keyword being private, protected, or public
		static std::optional<parsed_access_specifier> get_access_specifier_from_string(std::string_view keyword);

		static bool is_type_qualifier_ish(std::string_view keyword);

		std::stack<parse_state> m_state_stack{};
		std::stack<scope_stack_entry> m_scope_stack{};

		std::string m_most_recently_parsed_attributes{};
		std::string m_most_recently_parsed_type{};
		std::string m_most_recently_parsed_identifier{};
		parsed_keywords m_most_recently_parsed_keywords{};
		parsed_type_type m_most_recently_parsed_type_type{};
		std::optional<parsed_access_specifier> m_most_recently_parsed_access_specifier{};
	};
}

ge::parsed_file ge::parser::parse(std::string_view file)
{
	tokeniser tokeniser{ file };
	parsed_file parsed_file{};

	m_scope_stack.emplace(parsed_file, -1);
	push_state(parse_state::none);

	for (auto it = tokeniser.begin(); it != tokeniser.end(); ++it)
	{
		if (it->m_flag == token::flag::comment
			|| it->m_flag == token::flag::attribute)
		{
			continue;
		}

		while (true)
		{
			if (bool was_token_consumed = on_state_receive_token(m_state_stack.top(), it))
			{
				break;
			}
		}
	}

	if (m_scope_stack.size() > 1ull)
	{
		report_error("Found '{' with no matching '}'");
	}

	if (m_state_stack.size() > 1ull)
	{
		report_error("Parser did not fully exit all of it's states before reaching the end of the file.");
	}

	return parsed_file;
}

void ge::parser::on_state_push_requested(parse_state state)
{
	auto queue_single =
		[&](parse_state sub_state)
		{
			if (sub_state == state)
			{
				m_state_stack.push(state);
			}
			else
			{
				push_state(sub_state);
			}
		};

	auto queue_multi =
		[&](const auto& self, auto sub_state, auto... sub_states)
		{
			if constexpr (sizeof...(sub_states) == 0)
			{
				queue_single(sub_state);
			}
			else
			{
				self(self, sub_states...);
				queue_single(sub_state);
			}
		};

	auto queue = 
		[&](auto... states)
		{
			queue_multi(queue_multi, states...);
		};

	switch (state)
	{
	case parse_state::reflect_namespace:
	{
		queue(parse_state::parse_identifier,
			parse_state::complete_next_state_immediately,
			parse_state::store_reflected_namespace);
		break;
	}
	case parse_state::reflect_data:
	{
		queue(parse_state::parse_attributes, 
			parse_state::parse_keywords, 
			parse_state::parse_type,
			parse_state::parse_identifier,
			parse_state::complete_next_state_immediately,
			parse_state::store_reflected_data);
		break;
	}
	case parse_state::reflect_type:
	{
		queue(parse_state::parse_attributes,
			parse_state::parse_type_type,
			parse_state::parse_identifier,
			parse_state::complete_next_state_immediately,
			parse_state::store_reflected_type,
			parse_state::check_for_next_base);
		break;
	}
	case parse_state::reflect_func:
	{
		queue(parse_state::parse_attributes,
			parse_state::parse_keywords,
			parse_state::parse_type,
			parse_state::parse_identifier,
			parse_state::complete_next_state_immediately,
			parse_state::store_reflected_func,
			parse_state::skip_to_opening_parentheses,
			parse_state::check_for_next_parameter
		);
		break;
	}
	case parse_state::reflect_parameter:
	{
		queue(parse_state::parse_type,
			parse_state::parse_identifier,
			parse_state::complete_next_state_immediately,
			parse_state::store_parameter);
		break;
	}
	case parse_state::reflect_base:
	{
		queue(parse_state::parse_access_specifier,
			parse_state::parse_type,
			parse_state::complete_next_state_immediately,
			parse_state::store_base);
		break;
	}
	case parse_state::parse_attributes:
	{
		m_most_recently_parsed_attributes.clear();
		queue(parse_state::skip_to_opening_parentheses,
			parse_state::parse_attributes);
		break;
	}
	case parse_state::parse_type:
	{
		m_most_recently_parsed_type.clear();
		queue(parse_state::parse_type);
		break;
	}
	case parse_state::parse_type_type:
	{
		m_most_recently_parsed_type_type = {};
		queue(parse_state::parse_type_type);
		break;
	}
	case parse_state::parse_access_specifier:
	{
		m_most_recently_parsed_access_specifier.reset();
		queue(parse_state::parse_access_specifier);
		break;
	}
	case parse_state::parse_keywords:
	{
		m_most_recently_parsed_keywords = {};
		queue(parse_state::parse_keywords);
		break;
	}
	case parse_state::parse_identifier:
	{
		m_most_recently_parsed_identifier.clear();
		queue(parse_state::parse_identifier);
		break;
	}
	case parse_state::none:
	case parse_state::check_for_next_parameter:
	case parse_state::check_for_next_base:
	case parse_state::skip_to_opening_parentheses:
	case parse_state::parse_type_after_identifier_start_found:
	case parse_state::complete_next_state_immediately:
	case parse_state::store_reflected_namespace:
	case parse_state::store_reflected_type:
	case parse_state::store_base:
	case parse_state::store_reflected_data:
	case parse_state::store_reflected_func:
	case parse_state::store_parameter:
		queue(state);
		break;
	default:
		std::unreachable();
	}
}

void ge::parser::complete_state()
{
	parse_state completed = m_state_stack.top();
	m_state_stack.pop();
	on_state_completed(completed);

	if (m_state_stack.top() == parse_state::complete_next_state_immediately)
	{
		m_state_stack.pop();
		complete_state();
	}
}

void ge::parser::push_state(parse_state state)
{
	on_state_push_requested(state);
}

void ge::parser::switch_state(parse_state state)
{
	complete_state();
	on_state_push_requested(state);
}

bool ge::parser::on_state_receive_token(parse_state state, token_iterator it)
{
	switch (state)
	{
	case parse_state::parse_keywords:
	{
		auto add_flag =
			[&](parsed_keywords keyword)
			{
				m_most_recently_parsed_keywords = m_most_recently_parsed_keywords | keyword;
			};

		if (it->m_flag == token::flag::white_space ||
			it->m_flag == token::flag::attribute ||
			it->m_flag == token::flag::comment)
		{
			return true;
		}

		if (it->m_str == "static")
		{
			add_flag(parsed_keywords::static_keyword);
			return true;
		}

		if (it->m_str == "inline")
		{
			add_flag(parsed_keywords::inline_keyword);
			return true;
		}

		if (it->m_str == "virtual")
		{
			add_flag(parsed_keywords::virtual_keyword);
			return true;
		}

		if (it->m_str == "export")
		{
			add_flag(parsed_keywords::export_keyword);
			return true;
		}

		// Assume it's a macro if it's all caps
		if (std::ranges::all_of(it->m_str, [](char ch){ return std::isupper(static_cast<unsigned char>(ch)); }))
		{
			return true;
		}

		complete_state();
		return false;
	}
	case parse_state::skip_to_opening_parentheses:
	{
		if (it->m_str == "(")
		{
			complete_state();
		}
		return true;
	}
	case parse_state::parse_identifier:
	{
		if (it->m_flag == token::flag::valid_identifier)
		{
			m_most_recently_parsed_identifier = it->m_str;
			complete_state();
		}
	
		if (it->m_str == "{"
			|| it->m_str == "}"
			|| it->m_str == "("
			|| it->m_str == ")"
			|| it->m_str == ";")
		{
			complete_state();
			return false;
		}

		return true;
	}
	case parse_state::parse_attributes:
	{
		if (it.parentheses_count() == 0
			&& it->m_str == ")")
		{
			complete_state();
			return true;
		}

		m_most_recently_parsed_attributes += it->m_str;
		return true;
	}
	case parse_state::parse_type:
	{
		// Trim leading whitespace
		if (m_most_recently_parsed_type.empty()
			&& it->m_flag == token::flag::white_space)
		{
			return true;
		}

		if (is_type_qualifier_ish(it->m_str)
			|| it->m_flag == token::flag::white_space)
		{
			m_most_recently_parsed_type += it->m_str;
			return true;
		}

		if (it->m_flag != token::flag::valid_identifier)
		{
			report_error("No valid type found");
			return false;
		}

		m_most_recently_parsed_type += it->m_str;
		complete_state();
		push_state(parse_state::parse_type_after_identifier_start_found);
		return true;
	}
	case parse_state::parse_type_after_identifier_start_found:
	{
		// Check keywords
		if (it.template_bracket_count() == 0
			&& it->m_str != ">"
			&& !is_type_qualifier_ish(it->m_str)
			&& it->m_flag != token::flag::white_space
			&& it->m_str != "::"
			&& (it->m_flag != token::flag::valid_identifier || !m_most_recently_parsed_type.ends_with("::")))
		{
			// Trim ending whitespace
			m_most_recently_parsed_type.erase(std::find_if(m_most_recently_parsed_type.rbegin(), m_most_recently_parsed_type.rend(),
				[](char ch)
				{
					return !std::isspace(static_cast<unsigned char>(ch));
				}).base(), m_most_recently_parsed_type.end());

			complete_state();
			return false;
		}

		m_most_recently_parsed_type += it->m_str;
		return true;
	}
	case parse_state::parse_type_type:
	{
		if (it->m_str == "class")
		{
			m_most_recently_parsed_type_type = parsed_type_type::class_type;
			complete_state();
			return true;
		}

		if (it->m_str == "struct")
		{
			m_most_recently_parsed_type_type = parsed_type_type::struct_type;
			complete_state();
			return true;
		}

		return true;
	}
	case parse_state::parse_access_specifier:
	{
		std::optional<parsed_access_specifier> access = get_access_specifier_from_string(it->m_str);

		if (access.has_value())
		{
			m_most_recently_parsed_access_specifier = std::move(access);
			complete_state();
			return true;
		}

		if (it->m_flag == token::flag::white_space)
		{
			return true;
		}

		complete_state();
		return false;
	}
	case parse_state::none:
	{
		if (it->m_str == s_refl_data)
		{
			push_state(parse_state::reflect_data);
			return true;
		}

		if (it->m_str == s_refl_func)
		{
			push_state(parse_state::reflect_func);
			return true;
		}

		if (it->m_str == s_refl_class)
		{
			push_state(parse_state::reflect_type);
			return true;
		}

		if (it->m_str == "namespace")
		{
			push_state(parse_state::reflect_namespace);
			return true;
		}

		if (std::optional<parsed_access_specifier> access = get_access_specifier_from_string(it->m_str))
		{
			m_scope_stack.top().m_current_access_level = *access;
			return true;
		}

		if (it->m_str == "}")
		{
			if (it.curly_bracket_count() != m_scope_stack.top().m_curly_brackets_count_before_scope)
			{
				return true;
			}

			if (m_scope_stack.size() <= 1ull)
			{
				report_error("Unexpected token: '}'. No matching '{'");
				return true;
			}

			m_scope_stack.pop();
			return true;
		}

		return true;
	}
	case parse_state::check_for_next_parameter:
	{
		// TODO check for curly/template bracket
		if (it.parentheses_count() == 0
			&& it->m_str == ")")
		{
			complete_state();
			return true;
		}

		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();

		if (func.m_parameters.empty())
		{
			complete_state();
			push_state(parse_state::reflect_parameter);
			return false;
		}

		if (it.parentheses_count() == 1
			&& it.template_bracket_count() == 0
			&& it.curly_bracket_count() == m_scope_stack.top().m_curly_brackets_count_before_scope + 1
			&& it->m_str == ",")
		{
			complete_state();
			push_state(parse_state::reflect_parameter);
			return true;
		}

		return true;
	}
	case parse_state::check_for_next_base:
	{
		if (it.template_bracket_count() != 0)
		{
			return true;
		}

		if (it->m_str == "{")
		{
			complete_state();
			return true;
		}

		if (it->m_str == ":"
			|| it->m_str == ",")
		{
			complete_state();
			push_state(parse_state::reflect_base);
			return true;
		}
		return true;
	}

	default:
	case parse_state::reflect_namespace:
	case parse_state::reflect_type:
	case parse_state::reflect_data:
	case parse_state::reflect_func:
	case parse_state::reflect_parameter:
	case parse_state::reflect_base:
	case parse_state::complete_next_state_immediately:
	case parse_state::store_reflected_namespace:
	case parse_state::store_reflected_type:
	case parse_state::store_base:
	case parse_state::store_reflected_data:
	case parse_state::store_reflected_func:
	case parse_state::store_parameter:
		std::unreachable();
	}
}

void ge::parser::on_state_completed(parse_state state)
{
	switch (state)
	{
	case parse_state::store_reflected_namespace:
	{
		parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
		parsed_scope& new_namespace = parent.m_namespaces.emplace_back(make_unique_ref<parsed_scope>());

		new_namespace.m_name = m_most_recently_parsed_identifier;

		m_scope_stack.emplace(new_namespace, m_scope_stack.top().m_curly_brackets_count_before_scope + 1);
		break;
	}
	case parse_state::store_reflected_type:
	{
		if (m_most_recently_parsed_identifier.empty())
		{
			report_error(std::format("{} requires a named type, but no valid identifier could be found.", s_refl_class));
			break;
		}

		parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
		parsed_type& new_type = parent.m_types.emplace_back(make_unique_ref<parsed_type>());

		new_type.m_name = m_most_recently_parsed_identifier;
		new_type.m_type = m_most_recently_parsed_type_type;
		new_type.m_attributes = m_most_recently_parsed_attributes;

		scope_stack_entry& new_entry = m_scope_stack.emplace(new_type, m_scope_stack.top().m_curly_brackets_count_before_scope + 1);

		switch (new_type.m_type)
		{
		case parsed_type_type::class_type:
			new_entry.m_current_access_level = parsed_access_specifier::private_access;
			break;
		case parsed_type_type::struct_type:
			new_entry.m_current_access_level = parsed_access_specifier::public_access;
			break;
		default:
			std::unreachable();
		}
		break;
	}
	case parse_state::store_base:
	{
		if (m_most_recently_parsed_type.empty())
		{
			report_error(std::format("{} has a base type, but no valid typename could be parsed.", s_refl_class));
			break;
		}

		parsed_type& type = static_cast<parsed_type&>(m_scope_stack.top().m_parsed_scope.get());
		parsed_base& base = type.m_base_types.emplace_back();

		base.m_name = m_most_recently_parsed_type;
		base.m_access = m_most_recently_parsed_access_specifier;

		push_state(parse_state::check_for_next_base);
		break;
	}
	case parse_state::store_reflected_data:
	{
		if (m_most_recently_parsed_type.empty())
		{
			report_error(std::format("{} was not followed by a type", s_refl_data));
			break;
		}

		if (m_most_recently_parsed_identifier.empty())
		{
			report_error(std::format("{} requires a named variable, but no valid identifier could be found.", s_refl_data));
			break;
		}

		parsed_data& data = m_scope_stack.top().m_parsed_scope.get().m_data.emplace_back();
		data.m_access = m_scope_stack.top().m_current_access_level;
		data.m_name = m_most_recently_parsed_identifier;
		data.m_type = m_most_recently_parsed_type;
		data.m_attributes = m_most_recently_parsed_attributes;
		data.m_keywords = m_most_recently_parsed_keywords;
		break;
	}
	case parse_state::store_reflected_func:
	{
		if (m_most_recently_parsed_type.empty())
		{
			report_error(std::format("{} was not followed by a return type", s_refl_func));
			break;
		}

		if (m_most_recently_parsed_identifier.empty())
		{
			report_error(std::format("{} requires a named function, but no valid identifier could be found.", s_refl_func));
			break;
		}

		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.emplace_back();
		func.m_access = m_scope_stack.top().m_current_access_level;
		func.m_name = m_most_recently_parsed_identifier;
		func.m_return_type = m_most_recently_parsed_type;
		func.m_attributes = m_most_recently_parsed_attributes;
		func.m_keywords = m_most_recently_parsed_keywords;
		break;
	}
	case parse_state::store_parameter:
	{
		if (m_most_recently_parsed_type.empty())
		{
			report_error(std::format("{} has a parameter without a type", s_refl_func));
			break;
		}
	
		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();
		parsed_parameter& param = func.m_parameters.emplace_back();
		
		param.m_type = m_most_recently_parsed_type;
		param.m_name = m_most_recently_parsed_identifier;

		push_state(parse_state::check_for_next_parameter);
		break;
	}
	case parse_state::parse_type:
	case parse_state::parse_type_after_identifier_start_found:
	case parse_state::parse_keywords:
	case parse_state::skip_to_opening_parentheses:
	case parse_state::parse_identifier:
	case parse_state::parse_type_type:
	case parse_state::parse_access_specifier:
	case parse_state::parse_attributes:
	case parse_state::check_for_next_parameter:
	case parse_state::check_for_next_base:
	default:
		break;
	case parse_state::none:
	case parse_state::complete_next_state_immediately:
	case parse_state::reflect_namespace:
	case parse_state::reflect_data:
	case parse_state::reflect_type:
	case parse_state::reflect_base:
	case parse_state::reflect_func:
	case parse_state::reflect_parameter:
		std::unreachable();
	}
}

std::optional<ge::parsed_access_specifier> ge::parser::get_access_specifier_from_string(std::string_view keyword)
{
	if (keyword == "private")
	{
		return parsed_access_specifier::private_access;
	}
	
	if (keyword == "protected")
	{
		return parsed_access_specifier::protected_access;
	}

	if (keyword == "public")
	{
		return parsed_access_specifier::public_access;
	}

	return std::nullopt;
}

bool ge::parser::is_type_qualifier_ish(std::string_view keyword)
{
	return keyword == "&"
		|| keyword == "&&"
		|| keyword == "*"
		|| keyword == "const"
		|| keyword == "volatile";
}
