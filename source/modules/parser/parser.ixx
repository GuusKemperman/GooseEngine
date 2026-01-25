export module parser;

export import :tokeniser;

namespace ge
{
	export enum class parsed_keywords : std::uint8_t
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

	export enum class parsed_access_specifier : std::uint8_t
	{
		public_access,
		private_access,
		protected_access,
	};

	export enum class parsed_type_key : std::uint8_t
	{
		class_type,
		struct_type,
	};

	export enum class parsed_enum_key : std::uint8_t
	{
		enum_class_key,
		enum_struct_key,
		enum_key,
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
		std::string m_trailing_qualifiers{};
		std::vector<parsed_parameter> m_parameters{};

		parsed_keywords m_keywords{};
		parsed_access_specifier m_access{};
	};

	export struct parsed_enum
	{
		std::string m_attributes{};
		std::string m_name{};
		std::vector<std::string> m_entries{};
		parsed_enum_key m_key{};
	};

	export struct parsed_scope
	{
		std::string m_name{};

		std::vector<parsed_func> m_funcs{};
		std::vector<parsed_data> m_data{};
		std::vector<parsed_enum> m_enums{};

		std::list<parsed_scope> m_namespaces{};
		std::list<parsed_type> m_types{};
	};

	export struct parsed_base
	{
		std::string m_name{};
		std::optional<parsed_access_specifier> m_access{};
	};

	export struct parsed_type : parsed_scope
	{
		std::string m_attributes{};
		parsed_type_key m_key{};
		std::vector<parsed_base> m_base_types{};
		parsed_access_specifier m_access{};
	};

	export struct parsed_file : parsed_scope
	{
	};

	export API std::expected<parsed_file, std::string> parse(std::string_view file);

	using namespace std::string_view_literals;

	class parser
	{
	public:
		static constexpr std::string_view s_refl_data = "REFL_DATA"sv;
		static constexpr std::string_view s_refl_func = "REFL_FUNC"sv;
		static constexpr std::string_view s_refl_enum = "REFL_ENUM"sv;
		static constexpr std::string_view s_refl_class = "REFL_TYPE"sv;

		std::expected<parsed_file, std::string> parse(std::string_view file);

	private:
		using enum_type = std::uint8_t;

		enum class reflect_bundle : enum_type
		{
			reflect_namespace,
			reflect_type_definition,
			reflect_base,
			reflect_enum,
			reflect_enum_entry,
			reflect_data,
			reflect_func,
			reflect_parameter,
			reflect_trailing_qualifiers,
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
			parse_trailing_qualifiers,
			parse_access_specifier,
			parse_type_key,
			parse_enum_key,

			check_for_next_base,
			check_for_next_parameter,
			check_for_next_enum_entry,

			skip_to_opening_parentheses,
			skip_to_opening_curly_bracket,
		};

		enum class store_event : enum_type
		{
			store_reflected_namespace,
			store_reflected_type,
			store_base,
			store_reflected_enum,
			store_enum_entry,
			store_reflected_data,
			store_reflected_func,
			store_parameter,
			store_trailing_qualifiers,
		};
		static constexpr enum_type s_next_item_is_store_event = std::numeric_limits<enum_type>::max();

		union parse_state
		{
			enum_type m_value{};
			token_consumer m_token_consumer;
			store_event m_store_event;
		};

		struct scope_stack_entry
		{
			std::reference_wrapper<parsed_scope> m_parsed_scope;
			std::int64_t m_curly_brackets_count_before_scope{};
			parsed_access_specifier m_current_access_level{};
		};

		void complete_state();

		void push_state(reflect_bundle bundle);
		void push_state(token_consumer consumer);
		void push_state(store_event event);

		bool receive_token(token_consumer consumer, token_iterator it);

		void store(store_event event);

		[[noreturn]] static void report_failure(std::string_view reason);
		static std::string format_error(std::string_view file, token_iterator error_location, std::string_view reason);

		static std::optional<parsed_access_specifier> get_access_specifier_from_string(std::string_view keyword);
		static bool is_type_qualifier_ish(std::string_view keyword);
		static void trim_trailing_whitespace(std::string& str);

		std::stack<parse_state> m_state_stack{};
		std::stack<scope_stack_entry> m_scope_stack{};

		struct
		{
			std::string m_attributes{};
			std::string m_type_specifier{};
			std::string m_identifier{};
			parsed_keywords m_keywords{};
			parsed_type_key m_type_key{};
			parsed_enum_key m_enum_key{};
			std::optional<parsed_access_specifier> m_access_specifier{};
		} m_most_recently_parsed{};
	};
}

std::expected<ge::parsed_file, std::string> ge::parse(std::string_view file)
{
	return parser{}.parse(file);
}

std::expected<ge::parsed_file, std::string> ge::parser::parse(std::string_view file)
{
	tokeniser tokeniser{ file };
	parsed_file parsed_file{};

	m_scope_stack.emplace(parsed_file, -1ll);
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
			try
			{
				if (bool was_token_consumed = receive_token(m_state_stack.top().m_token_consumer, it))
				{
					break;
				}
			}
			catch (const std::exception& e)
			{
				return std::unexpected(format_error(file, it, e.what()));
			}
		}
	}

	if (m_scope_stack.size() > 1ull)
	{
		return std::unexpected("Found '{' with no matching '}'");
	}

	if (m_state_stack.size() > 1ull)
	{
		return std::unexpected("Parser did not fully exit all of it's states before reaching the end of the file.");
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
			token_consumer::parse_type_key,
			token_consumer::parse_identifier,
			store_event::store_reflected_type,
			token_consumer::check_for_next_base);
		break;
	case reflect_bundle::reflect_base:
		queue(token_consumer::parse_access_specifier,
			reflect_bundle::reflect_type_specifier,
			store_event::store_base,
			token_consumer::check_for_next_base);
		break;
	case reflect_bundle::reflect_enum:
		queue(reflect_bundle::reflect_attributes,
			token_consumer::parse_enum_key,
			token_consumer::parse_identifier,
			store_event::store_reflected_enum,
			token_consumer::skip_to_opening_curly_bracket,
			token_consumer::check_for_next_enum_entry);
		break;
	case reflect_bundle::reflect_enum_entry:
		queue(token_consumer::parse_identifier,
			store_event::store_enum_entry,
			token_consumer::check_for_next_enum_entry);
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
			token_consumer::check_for_next_parameter,
			reflect_bundle::reflect_trailing_qualifiers);
		break;
	case reflect_bundle::reflect_parameter:
		queue(reflect_bundle::reflect_type_specifier,
			token_consumer::parse_identifier,
			store_event::store_parameter,
			token_consumer::check_for_next_parameter);
		break;
	case reflect_bundle::reflect_trailing_qualifiers:
		queue(token_consumer::parse_trailing_qualifiers,
			store_event::store_trailing_qualifiers);
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

		if (it->m_str == s_refl_enum)
		{
			push_state(reflect_bundle::reflect_enum);
			return true;
		}

		if (it->m_str == s_refl_class)
		{
			push_state(reflect_bundle::reflect_type_definition);
			return true;
		}

		if (it->m_str == "namespace"sv)
		{
			push_state(reflect_bundle::reflect_namespace);
			return true;
		}

		if (std::optional<parsed_access_specifier> access = get_access_specifier_from_string(it->m_str))
		{
			m_scope_stack.top().m_current_access_level = *access;
			return true;
		}

		if (it->m_str == "}"sv)
		{
			if (it.curly_bracket_count() != m_scope_stack.top().m_curly_brackets_count_before_scope)
			{
				return true;
			}

			if (m_scope_stack.size() <= 1ull)
			{
				report_failure("Unexpected token: '}'. No matching '{'"sv);
			}

			m_scope_stack.pop();
			return true;
		}

		return true;
	}
	case token_consumer::parse_attributes:
	{
		if (it.parentheses_count() == 0
			&& it->m_str == ")"sv)
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

		if (it->m_str == "static"sv)
		{
			add_flag(parsed_keywords::static_keyword);
			return true;
		}

		if (it->m_str == "inline"sv)
		{
			add_flag(parsed_keywords::inline_keyword);
			return true;
		}

		if (it->m_str == "virtual"sv)
		{
			add_flag(parsed_keywords::virtual_keyword);
			return true;
		}

		if (it->m_str == "export"sv)
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

		if (it->m_str == "{"sv
			|| it->m_str == "}"sv
			|| it->m_str == "("sv
			|| it->m_str == ")"sv
			|| it->m_str == ";"sv)
		{
			complete_state();
			return false;
		}

		return true;
	}
	case token_consumer::parse_type_specifier_pre_identifier:
	{
		if (m_most_recently_parsed.m_type_specifier.empty()
			&& it->m_flag == token::flag::white_space)
		{
			return true;
		}

		if (is_type_qualifier_ish(it->m_str)
			|| it->m_flag == token::flag::white_space)
		{
			m_most_recently_parsed.m_type_specifier += it->m_str;
			return true;
		}

		if (it->m_flag != token::flag::valid_identifier)
		{
			report_failure("No valid type found"sv);
		}

		m_most_recently_parsed.m_type_specifier += it->m_str;
		complete_state();
		return true;
	}
	case token_consumer::parse_type_specifier_post_identifier:
	{
		if (it.template_bracket_count() == 0
			&& it->m_str != ">"sv
			&& !is_type_qualifier_ish(it->m_str)
			&& it->m_flag != token::flag::white_space
			&& it->m_str != "::"sv
			&& (it->m_flag != token::flag::valid_identifier || !m_most_recently_parsed.m_type_specifier.ends_with("::"sv)))
		{
			trim_trailing_whitespace(m_most_recently_parsed.m_type_specifier);
			complete_state();
			return false;
		}

		m_most_recently_parsed.m_type_specifier += it->m_str;
		return true;
	}
	case token_consumer::parse_trailing_qualifiers:
	{
		if (m_most_recently_parsed.m_type_specifier.empty()
			&& it->m_flag == token::flag::white_space)
		{
			return true;
		}

		if (it->m_flag != token::flag::white_space
			&& !is_type_qualifier_ish(it->m_str))
		{
			trim_trailing_whitespace(m_most_recently_parsed.m_type_specifier);
			complete_state();
			return false;
		}

		m_most_recently_parsed.m_type_specifier += it->m_str;
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
	case token_consumer::parse_type_key:
	{
		if (it->m_str == "class"sv)
		{
			m_most_recently_parsed.m_type_key = parsed_type_key::class_type;
			complete_state();
			return true;
		}

		if (it->m_str == "struct"sv)
		{
			m_most_recently_parsed.m_type_key = parsed_type_key::struct_type;
			complete_state();
			return true;
		}

		return true;
	}
	case token_consumer::parse_enum_key:
	{
		if (it->m_str == "enum"sv)
		{
			m_most_recently_parsed.m_enum_key = parsed_enum_key::enum_key;
			return true;
		}

		if (it->m_str == "class"sv)
		{
			m_most_recently_parsed.m_enum_key = parsed_enum_key::enum_class_key;
			complete_state();
			return true;
		}

		if (it->m_str == "struct"sv)
		{
			m_most_recently_parsed.m_enum_key = parsed_enum_key::enum_struct_key;
			complete_state();
			return true;
		}

		if (it->m_flag != token::flag::white_space
			&& m_most_recently_parsed.m_enum_key == parsed_enum_key::enum_key)
		{
			complete_state();
			return false;
		}

		return true;
	}
	case token_consumer::check_for_next_base:
	{
		if (it.template_bracket_count() != 0)
		{
			return true;
		}

		if (it->m_str == "{"sv)
		{
			complete_state();
			return true;
		}

		if (it->m_str == ":"sv
			|| it->m_str == ","sv)
		{
			complete_state();
			push_state(reflect_bundle::reflect_base);
			return true;
		}
		return true;
	}
	case token_consumer::check_for_next_parameter:
	{
		if (it.parentheses_count() == 0
			&& it.template_bracket_count() == 0
			&& it.curly_bracket_count() == m_scope_stack.top().m_curly_brackets_count_before_scope + 1
			&& it->m_str == ")"sv)
		{
			complete_state();
			return true;
		}

		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();

		if (func.m_parameters.empty()
			&& it->m_flag == token::flag::valid_identifier)
		{
			complete_state();
			push_state(reflect_bundle::reflect_parameter);
			return false;
		}

		if (it.parentheses_count() == 1
			&& it.template_bracket_count() == 0
			&& it.curly_bracket_count() == m_scope_stack.top().m_curly_brackets_count_before_scope + 1
			&& it->m_str == ","sv)
		{
			complete_state();
			push_state(reflect_bundle::reflect_parameter);
		}

		return true;
	}
	case token_consumer::check_for_next_enum_entry:
	{
		if (it.parentheses_count() == 0
			&& it.template_bracket_count() == 0
			&& it.curly_bracket_count() == m_scope_stack.top().m_curly_brackets_count_before_scope + 1
			&& it->m_str == "}"sv)
		{
			complete_state();
			return true;
		}

		parsed_enum& parsed_enum = m_scope_stack.top().m_parsed_scope.get().m_enums.back();

		if (parsed_enum.m_entries.empty()
			&& it->m_flag == token::flag::valid_identifier)
		{
			complete_state();
			push_state(reflect_bundle::reflect_enum_entry);
			return false;
		}

		if (it.parentheses_count() == 0
			&& it.template_bracket_count() == 0
			&& it.curly_bracket_count() == m_scope_stack.top().m_curly_brackets_count_before_scope + 2
			&& it->m_str == ","sv)
		{
			complete_state();
			push_state(reflect_bundle::reflect_enum_entry);
		}

		return true;
	}
	case token_consumer::skip_to_opening_parentheses:
	{
		if (it->m_str == "("sv)
		{
			complete_state();
		}
		return true;
	}
	case token_consumer::skip_to_opening_curly_bracket:
	{
		if (it->m_str == "{"sv)
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
		parsed_scope& new_namespace = parent.m_namespaces.emplace_back();

		new_namespace.m_name = std::move(m_most_recently_parsed.m_identifier);

		m_scope_stack.emplace(new_namespace, m_scope_stack.top().m_curly_brackets_count_before_scope + 1);
		break;
	}
	case store_event::store_reflected_type:
	{
		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_failure("Expected identifier"sv);
		}

		parsed_scope& parent = m_scope_stack.top().m_parsed_scope;
		parsed_type& new_type = parent.m_types.emplace_back();

		new_type.m_name = std::move(m_most_recently_parsed.m_identifier);
		new_type.m_attributes = std::move(m_most_recently_parsed.m_attributes);
		new_type.m_key = m_most_recently_parsed.m_type_key;

		scope_stack_entry& new_entry = m_scope_stack.emplace(new_type, m_scope_stack.top().m_curly_brackets_count_before_scope + 1);

		switch (new_type.m_key)
		{
		case parsed_type_key::class_type:
			new_entry.m_current_access_level = parsed_access_specifier::private_access;
			break;
		case parsed_type_key::struct_type:
			new_entry.m_current_access_level = parsed_access_specifier::public_access;
			break;
		default:
			std::unreachable();
		}
		break;
	}
	case store_event::store_base:
	{
		if (m_most_recently_parsed.m_type_specifier.empty())
		{
			report_failure("Expected base type"sv);
		}

		parsed_type& type = static_cast<parsed_type&>(m_scope_stack.top().m_parsed_scope.get());
		parsed_base& base = type.m_base_types.emplace_back();

		base.m_name = std::move(m_most_recently_parsed.m_type_specifier);
		base.m_access = std::move(m_most_recently_parsed.m_access_specifier);
		break;
	}
	case store_event::store_reflected_enum:
	{
		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_failure("Expected identifier"sv);
		}

		parsed_enum& parsed_enum = m_scope_stack.top().m_parsed_scope.get().m_enums.emplace_back();
		parsed_enum.m_attributes = std::move(m_most_recently_parsed.m_attributes);
		parsed_enum.m_name = std::move(m_most_recently_parsed.m_identifier);
		break;
	}	
	case store_event::store_enum_entry:
	{
		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_failure("Expected identifier"sv);
		}

		parsed_enum& parsed_enum = m_scope_stack.top().m_parsed_scope.get().m_enums.back();
		parsed_enum.m_entries.emplace_back(std::move(m_most_recently_parsed.m_identifier));
		break;
	}
	case store_event::store_reflected_data:
	{
		if (m_most_recently_parsed.m_type_specifier.empty())
		{
			report_failure("Expected valid type specifier"sv);
		}

		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_failure("Expected identifier"sv);
		}

		parsed_data& data = m_scope_stack.top().m_parsed_scope.get().m_data.emplace_back();
		data.m_attributes = std::move(m_most_recently_parsed.m_attributes);
		data.m_name = std::move(m_most_recently_parsed.m_identifier);
		data.m_type = std::move(m_most_recently_parsed.m_type_specifier);
		data.m_keywords = m_most_recently_parsed.m_keywords;
		data.m_access = m_scope_stack.top().m_current_access_level;
		break;
	}
	case store_event::store_reflected_func:
	{
		if (m_most_recently_parsed.m_type_specifier.empty())
		{
			report_failure("Expected return type"sv);
		}

		if (m_most_recently_parsed.m_identifier.empty())
		{
			report_failure("Expected identifier"sv);
		}

		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.emplace_back();
		func.m_attributes = std::move(m_most_recently_parsed.m_attributes);
		func.m_name = std::move(m_most_recently_parsed.m_identifier);
		func.m_return_type = std::move(m_most_recently_parsed.m_type_specifier);
		func.m_keywords = m_most_recently_parsed.m_keywords;
		func.m_access = m_scope_stack.top().m_current_access_level;
		break;
	}
	case store_event::store_parameter:
	{
		if (m_most_recently_parsed.m_type_specifier.empty())
		{
			report_failure("Expected parameter type"sv);
		}

		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();
		parsed_parameter& param = func.m_parameters.emplace_back();

		param.m_type = std::move(m_most_recently_parsed.m_type_specifier);
		param.m_name = std::move(m_most_recently_parsed.m_identifier);
		break;
	}
	case store_event::store_trailing_qualifiers:
	{
		parsed_func& func = m_scope_stack.top().m_parsed_scope.get().m_funcs.back();
		func.m_trailing_qualifiers = std::move(m_most_recently_parsed.m_type_specifier);
		break;
	}
	default:
		std::unreachable();
	}

	m_most_recently_parsed = {};
}

void ge::parser::report_failure(std::string_view reason)
{
	throw std::exception{ reason.data() };
}

std::string ge::parser::format_error(std::string_view file, token_iterator error_location, std::string_view reason)
{
	std::string_view parsed_file{ file.data(), file.data() + error_location.num_characters_parsed() };
	std::int64_t character_index_in_line = std::distance(parsed_file.rbegin(), std::ranges::find(parsed_file.rbegin(), parsed_file.rend(), '\n'));

	auto lines = std::views::split(parsed_file, "\n"sv);
	std::int64_t error_line_number = std::distance(lines.begin(), lines.end());
	std::int64_t start_line = std::max(0ll, error_line_number - 3ll);

	std::string error = "\n";

	for (auto [index, line] : lines 
		| std::views::drop(start_line) 
		| std::views::enumerate)
	{
		error += std::format("{:6} | {}\n"sv, start_line + index, 
			std::string_view{ line.begin(), line.end() });
	}

	// + to account for us inserting the line number, and - since that spot will be taken by the '^'
	error.insert(error.end(), static_cast<size_t>(character_index_in_line + 9 - 1), ' ');
	error.append("^\n"sv);

	error.append(reason);
	return error;
}

std::optional<ge::parsed_access_specifier> ge::parser::get_access_specifier_from_string(std::string_view keyword)
{
	if (keyword == "private"sv)
	{
		return parsed_access_specifier::private_access;
	}

	if (keyword == "protected"sv)
	{
		return parsed_access_specifier::protected_access;
	}

	if (keyword == "public"sv)
	{
		return parsed_access_specifier::public_access;
	}

	return std::nullopt;
}

bool ge::parser::is_type_qualifier_ish(std::string_view keyword)
{
	return keyword == "&"sv
		|| keyword == "&&"sv
		|| keyword == "*"sv
		|| keyword == "const"sv
		|| keyword == "volatile"sv;
}

void ge::parser::trim_trailing_whitespace(std::string& str)
{
	str.erase(std::find_if(str.rbegin(), str.rend(),
		[](char ch)
		{
			return !std::isspace(static_cast<unsigned char>(ch));
		}).base(), str.end());
}
