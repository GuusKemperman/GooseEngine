export module parser;

export import :tokeniser;
import utils;

// TODO: Consider removing the '{' counter in the scopestack entry, and instead infer it from the size of the stack
// TODO: Move comment/attribute logic from tokeniser to parser
// TODO: Error handling
// TODO: Member function 'this' type

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
		using key_t = std::underlying_type_t<parsed_keywords>;
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
		using enum_type = std::uint8_t;

		enum class reflect_bundle : enum_type
		{
			reflect_namespace,
			reflect_type_definition,
			reflect_base,
			reflect_data,
			reflect_func,
			reflect_parameter,
			reflect_type_specifier,
			reflect_attributes,
		};

		enum class token_consumer : enum_type
		{
			none,

			parse_attributes,
			parse_keywords,
			parse_identifier,
			parse_type_specifier_pre_identifier,
			parse_type_specifier_post_identifier,
			parse_access_specifier,
			parse_type_type, // check for the 'struct' or 'class' keyword

			check_for_next_base,
			check_for_next_parameter,

			skip_to_opening_parentheses,
		};

		enum class store_event : enum_type
		{
			store_reflected_namespace,
			store_reflected_type,
			store_base,
			store_reflected_data,
			store_reflected_func,
			store_parameter,
		};

		union parse_state
		{
			enum_type m_value{};
			token_consumer m_token_consumer;
			store_event m_store_event;
		};

		static constexpr enum_type s_next_item_is_store_event = std::numeric_limits<enum_type>::max();

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
			push_state(token_consumer::none);
		}

		void push_state(reflect_bundle bundle);
		void push_state(token_consumer consumer);
		void push_state(store_event event);

		bool receive_token(token_consumer consumer, token_iterator it);

		void store(store_event event);

		// Keyword being private, protected, or public
		static std::optional<parsed_access_specifier> get_access_specifier_from_string(std::string_view keyword);

		static bool is_type_qualifier_ish(std::string_view keyword);

		std::stack<parse_state> m_state_stack{};
		std::stack<scope_stack_entry> m_scope_stack{};

		struct
		{
			std::string m_attributes{};
			std::string m_type{};
			std::string m_identifier{};
			parsed_keywords m_keywords{};
			parsed_type_type m_type_type{};
			std::optional<parsed_access_specifier> m_access_specifier{};
		} m_most_recently_parsed{};
	};
}

ge::parsed_file ge::parser::parse(std::string_view file)
{
	tokeniser tokeniser{ file };
	parsed_file parsed_file{};

	m_scope_stack.emplace(parsed_file, -1);
	push_state(token_consumer::none);

	for (auto it = tokeniser.begin(); it != tokeniser.end(); ++it)
	{
		if (it->m_flag == token::flag::comment
			|| it->m_flag == token::flag::attribute)
		{
			continue;
		}

		while (true)
		{
			if (bool was_token_consumed = receive_token(m_state_stack.top().m_token_consumer, it))
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

void ge::parser::push_state(reflect_bundle bundle)
{
	auto queue_multi =
		[&](const auto& self, auto sub_state, auto... sub_states)
		{
			if constexpr (sizeof...(sub_states) == 0)
			{
				push_state(sub_state);
			}
			else
			{
				self(self, sub_states...);
				push_state(sub_state);
			}
		};

	auto queue =
		[&](auto... states)
		{
			queue_multi(queue_multi, states...);
		};

	switch (bundle)
	{
	case reflect_bundle::reflect_namespace:
		queue(token_consumer::parse_identifier,
			store_event::store_reflected_namespace);
		break;
	case reflect_bundle::reflect_type_definition:
		queue(reflect_bundle::reflect_attributes,
			token_consumer::parse_type_type,
			token_consumer::parse_identifier,
			store_event::store_reflected_type,
			token_consumer::check_for_next_base);
		break;
	case reflect_bundle::reflect_base:
		queue(token_consumer::parse_access_specifier,
			reflect_bundle::reflect_type_specifier,
			store_event::store_base);
		break;
	case reflect_bundle::reflect_data:
		queue(reflect_bundle::reflect_attributes,
			token_consumer::parse_keywords,
			reflect_bundle::reflect_type_specifier,
			token_consumer::parse_identifier,
			store_event::store_reflected_data);
		break;
	case reflect_bundle::reflect_func:
		queue(reflect_bundle::reflect_attributes,
			token_consumer::parse_keywords,
			reflect_bundle::reflect_type_specifier,
			token_consumer::parse_identifier,
			store_event::store_reflected_func,
			token_consumer::skip_to_opening_parentheses,
			token_consumer::check_for_next_parameter);
		break;
	case reflect_bundle::reflect_parameter:
		queue(reflect_bundle::reflect_type_specifier,
			token_consumer::parse_identifier,
			store_event::store_parameter);
		break;
	case reflect_bundle::reflect_type_specifier:
		queue(token_consumer::parse_type_specifier_pre_identifier,
			token_consumer::parse_type_specifier_post_identifier);
		break;
	case reflect_bundle::reflect_attributes:
		queue(token_consumer::skip_to_opening_parentheses,
			token_consumer::parse_attributes);
		break;
	default:
		std::unreachable();
	}
}

void ge::parser::push_state(token_consumer consumer)
{
	m_state_stack.emplace().m_token_consumer = consumer;
}

void ge::parser::push_state(store_event event)
{
	m_state_stack.emplace().m_store_event = event;
	m_state_stack.emplace(s_next_item_is_store_event);
}

void ge::parser::complete_state()
{
	m_state_stack.pop();

	if (m_state_stack.top().m_value == s_next_item_is_store_event)
	{
		m_state_stack.pop();

		store_event event = m_state_stack.top().m_store_event;
		complete_state();
		store(event);
	}
}

bool ge::parser::receive_token(token_consumer consumer, token_iterator it)
{
	switch (consumer)
	{
	case token_consumer::none:
	{
		if (it->m_str == s_refl_data)
		{
			push_state(reflect_bundle::reflect_data);
			return true;
		}

		if (it->m_str == s_refl_func)
		{
			push_state(reflect_bundle::reflect_func);
			return true;
		}

		if (it->m_str == s_refl_class)
		{
			push_state(reflect_bundle::reflect_type_definition);
			return true;
		}

		if (it->m_str == "namespace")
		{
			push_state(reflect_bundle::reflect_namespace);
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
	case token_consumer::parse_attributes:
	{
		if (it.parentheses_count() == 0
			&& it->m_str == ")")
		{
			complete_state();
			return true;
		}

		m_most_recently_parsed.m_attributes += it->m_str;
		return true;
	}
	case token_consumer::parse_keywords:
	{
		auto add_flag =
			[&](parsed_keywords keyword)
			{
				m_most_recently_parsed.m_keywords = m_most_recently_parsed.m_keywords | keyword;
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
		if (std::ranges::all_of(it->m_str, [](char ch) { return std::isupper(static_cast<unsigned char>(ch)); }))
		{
			return true;
		}

		complete_state();
		return false;
	}
	case token_consumer::parse_identifier:
	{
		if (it->m_flag == token::flag::valid_identifier)
		{
			m_most_recently_parsed.m_identifier = it->m_str;
			complete_state();
			return true;
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
	case token_consumer::parse_type_specifier_pre_identifier:
	{
		// Trim leading whitespace
		if (m_most_recently_parsed.m_type.empty()
			&& it->m_flag == token::flag::white_space)
		{
			return true;
		}

		if (is_type_qualifier_ish(it->m_str)
			|| it->m_flag == token::flag::white_space)
		{
			m_most_recently_parsed.m_type += it->m_str;
			return true;
		}

		if (it->m_flag != token::flag::valid_identifier)
		{
			report_error("No valid type found");
			return false;
		}

		m_most_recently_parsed.m_type += it->m_str;
		complete_state();
		return true;
	}
	case token_consumer::parse_type_specifier_post_identifier:
	{
		if (it.template_bracket_count() == 0
			&& it->m_str != ">"
			&& !is_type_qualifier_ish(it->m_str)
			&& it->m_flag != token::flag::white_space
			&& it->m_str != "::"
			&& (it->m_flag != token::flag::valid_identifier || !m_most_recently_parsed.m_type.ends_with("::")))
		{
			// Trim ending whitespace
			m_most_recently_parsed.m_type.erase(std::find_if(m_most_recently_parsed.m_type.rbegin(), m_most_recently_parsed.m_type.rend(),
				[](char ch)
				{
					return !std::isspace(static_cast<unsigned char>(ch));
				}).base(), m_most_recently_parsed.m_type.end());

			complete_state();
			return false;
		}

		m_most_recently_parsed.m_type += it->m_str;
		return true;
	}
	case token_consumer::parse_access_specifier:
	{
		std::optional<parsed_access_specifier> access = get_access_specifier_from_string(it->m_str);

		if (access.has_value())
		{
			m_most_recently_parsed.m_access_specifier = std::move(access);
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
	case token_consumer::parse_type_type:
	{
		if (it->m_str == "class")
		{
			m_most_recently_parsed.m_type_type = parsed_type_type::class_type;
			complete_state();
			return true;
		}

		if (it->m_str == "struct")
		{
			m_most_recently_parsed.m_type_type = parsed_type_type::struct_type;
			complete_state();
			return true;
		}

		return true;
	}
	case token_consumer::check_for_next_base:
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
			push_state(reflect_bundle::reflect_base);
			return true;
		}
		return true;
	}
	case token_consumer::check_for_next_parameter:
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
			push_state(reflect_bundle::reflect_parameter);
			return false;
		}

		if (it.parentheses_count() == 1
			&& it.template_bracket_count() == 0
			&& it.curly_bracket_count() == m_scope_stack.top().m_curly_brackets_count_before_scope + 1
			&& it->m_str == ",")
		{
			complete_state();
			push_state(reflect_bundle::reflect_parameter);
			return true;
		}

		return true;
	}
	case token_consumer::skip_to_opening_parentheses:
	{
		if (it->m_str == "(")
		{
			complete_state();
		}
		return true;
	}
	default:
		std::unreachable();
	}
}

void ge::parser::store(store_event event)
{
	switch (event)
	{
	case store_event::store_reflected_namespace:
	{
		parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
		parsed_scope& new_namespace = parent.m_namespaces.emplace_back(make_unique_ref<parsed_scope>());

		new_namespace.m_name = m_most_recently_parsed.m_identifier;

		m_scope_stack.emplace(new_namespace, m_scope_stack.top().m_curly_brackets_count_before_scope + 1);
		break;
	}
	case store_event::store_reflected_type:
	{
		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_error(std::format("{} requires a named type, but no valid identifier could be found.", s_refl_class));
			break;
		}

		parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
		parsed_type& new_type = parent.m_types.emplace_back(make_unique_ref<parsed_type>());

		new_type.m_name = m_most_recently_parsed.m_identifier;
		new_type.m_type = m_most_recently_parsed.m_type_type;
		new_type.m_attributes = m_most_recently_parsed.m_attributes;

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
	case store_event::store_base:
	{
		if (m_most_recently_parsed.m_type.empty())
		{
			report_error(std::format("{} has a base type, but no valid typename could be parsed.", s_refl_class));
			break;
		}

		parsed_type& type = static_cast<parsed_type&>(m_scope_stack.top().m_parsed_scope.get());
		parsed_base& base = type.m_base_types.emplace_back();

		base.m_name = m_most_recently_parsed.m_type;
		base.m_access = m_most_recently_parsed.m_access_specifier;

		push_state(token_consumer::check_for_next_base);
		break;
	}
	case store_event::store_reflected_data:
	{
		if (m_most_recently_parsed.m_type.empty())
		{
			report_error(std::format("{} was not followed by a type", s_refl_data));
			break;
		}

		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_error(std::format("{} requires a named variable, but no valid identifier could be found.", s_refl_data));
			break;
		}

		parsed_data& data = m_scope_stack.top().m_parsed_scope.get().m_data.emplace_back();
		data.m_access = m_scope_stack.top().m_current_access_level;
		data.m_name = m_most_recently_parsed.m_identifier;
		data.m_type = m_most_recently_parsed.m_type;
		data.m_attributes = m_most_recently_parsed.m_attributes;
		data.m_keywords = m_most_recently_parsed.m_keywords;
		break;
	}
	case store_event::store_reflected_func:
	{
		if (m_most_recently_parsed.m_type.empty())
		{
			report_error(std::format("{} was not followed by a return type", s_refl_func));
			break;
		}

		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_error(std::format("{} requires a named function, but no valid identifier could be found.", s_refl_func));
			break;
		}

		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.emplace_back();
		func.m_access = m_scope_stack.top().m_current_access_level;
		func.m_name = m_most_recently_parsed.m_identifier;
		func.m_return_type = m_most_recently_parsed.m_type;
		func.m_attributes = m_most_recently_parsed.m_attributes;
		func.m_keywords = m_most_recently_parsed.m_keywords;
		break;
	}
	case store_event::store_parameter:
	{
		if (m_most_recently_parsed.m_type.empty())
		{
			report_error(std::format("{} has a parameter without a type", s_refl_func));
			break;
		}

		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();
		parsed_parameter& param = func.m_parameters.emplace_back();

		param.m_type = m_most_recently_parsed.m_type;
		param.m_name = m_most_recently_parsed.m_identifier;

		push_state(token_consumer::check_for_next_parameter);
		break;
	}
	default:
		std::unreachable();
	}

	m_most_recently_parsed = {};
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
