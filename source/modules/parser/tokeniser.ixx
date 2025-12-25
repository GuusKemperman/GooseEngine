export module parser:tokeniser;

export import std;

namespace ge
{
	export struct token
	{
		enum class flag
		{
			none,
			white_space,
			comment,
			attribute,
			valid_identifier,
		};

		std::string_view m_str{};
		flag m_flag{};

		API bool operator==(const token&) const = default;
	};

	export class token_iterator
	{
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = token;

		API token_iterator() = default;
		API token_iterator(std::string_view file) : m_remaining_file(file) {}

		API const token& operator*() const { return m_token.value(); }
		API const std::optional<token>& operator->() const { return m_token; }

		API token_iterator& operator++();

		API token_iterator operator++(int)
		{
			auto tmp = *this;
			++*this;
			return tmp;
		}

		API bool operator==(const token_iterator&) const = default;

	private:
		std::string_view m_remaining_file{};
		std::optional<token> m_token{};
	};


	export class tokeniser
	{
	public:
		API tokeniser(std::string_view file) : m_file(file) {}

		API token_iterator begin() const
		{
			token_iterator it{ m_file };
			++it;
			return it;
		}

		API token_iterator end() const
		{
			token_iterator it{};
			return it;
		}

	private:
		std::string_view m_file{};
	};
}

ge::token_iterator& ge::token_iterator::operator++()
{
	auto peek =
		[&](size_t index, unsigned char ch)
		{
			if (index >= m_remaining_file.size())
			{
				return false;
			}
			return m_remaining_file[index] == ch;
		};

	auto start_token =
		[&]
		{
			m_token.emplace();
			m_token->m_str = m_remaining_file.substr(0, 0);
		};

	auto discard_char =
		[&](size_t amount = 1)
		{
			m_remaining_file = m_remaining_file.substr(amount);
		};

	auto add_to_token =
		[&](size_t amount = 1)
		{
			m_token->m_str = { m_token->m_str.data(), std::min(m_token->m_str.data() + m_token->m_str.size() + amount,
				m_remaining_file.data() + m_remaining_file.size()) };
			discard_char(amount);
		};

	auto add_to_token_until_pred =
		[&](const auto& pred)
		{
			while (!m_remaining_file.empty())
			{
				if (pred())
				{
					break;
				}

				add_to_token();
			}
		};

	auto add_to_token_until_match =
		[&](std::string_view match)
		{
			add_to_token_until_pred(
				[&]
				{
					return m_remaining_file.starts_with(match);
				}
			);
		};

	m_token.reset();

	if (m_remaining_file.empty())
	{
		return *this;
	}

	bool white_space_found = false;
	while (!m_remaining_file.empty()
		&& std::isspace(static_cast<unsigned char>(m_remaining_file.front())))
	{
		white_space_found = true;
		discard_char();
	}

	if (white_space_found)
	{
		m_token.emplace();
		m_token->m_flag = token::flag::white_space;
		return *this;
	}

	if (m_remaining_file.starts_with("[["))
	{
		discard_char(2);

		start_token();
		m_token->m_flag = token::flag::attribute;
		add_to_token_until_match("]]");

		discard_char(2);
		return *this;
	}

	if (m_remaining_file.starts_with("/*"))
	{
		discard_char(2);
		start_token();
		m_token->m_flag = token::flag::comment;
		add_to_token_until_match("*/");
		discard_char(2);
		return *this;
	}

	if (m_remaining_file.starts_with("//"))
	{
		discard_char(2);
		start_token();
		m_token->m_flag = token::flag::comment;
		add_to_token_until_match("\n");
		discard_char();
		return *this;
	}

	if (m_remaining_file.starts_with("R\"(")
		|| m_remaining_file.starts_with("LR\"")
		|| m_remaining_file.starts_with("u8R\"")
		|| m_remaining_file.starts_with("uR\"")
		|| m_remaining_file.starts_with("UR\""))
	{
		start_token();
		add_to_token_until_match(")\"");
		add_to_token(2);
		return *this;
	}

	if (m_remaining_file.starts_with("\"")
		|| m_remaining_file.starts_with("L\"")
		|| m_remaining_file.starts_with("u8\"")
		|| m_remaining_file.starts_with("u\"")
		|| m_remaining_file.starts_with("U\""))
	{
		start_token();

		// Find the opening quote
		add_to_token_until_match("\"");
		add_to_token();

		while (!m_remaining_file.empty())
		{
			if (!peek(0, '\\') && peek(1, '"'))
			{
				add_to_token(2);
				break;
			}
			add_to_token();
		}

		return *this;
	}

	if (m_remaining_file.starts_with("::")
		|| m_remaining_file.starts_with("->")
		|| m_remaining_file.starts_with("&&"))
	{
		start_token();
		add_to_token(2);
		return *this;
	}

	static constexpr std::string_view identifier_start_characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
	static constexpr std::string_view identifier_characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";

	static constexpr std::string_view number_literal_start_characters = "0123456789.";
	static constexpr std::string_view number_literal_characters = "0123456789'.-eEb";
	static constexpr std::string_view hex_characters = "0123456789'abcdefABCDEF";

	if (m_remaining_file.starts_with("0x")
		|| m_remaining_file.starts_with("0X"))
	{
		start_token();
		add_to_token_until_pred(
			[&]
			{
				return !hex_characters.contains(m_remaining_file.front());
			});
		return *this;
	}

	if (number_literal_start_characters.contains(m_remaining_file.front()))
	{
		start_token();
		add_to_token_until_pred(
			[&]
			{
				return !number_literal_characters.contains(m_remaining_file.front());
			});
		return *this;
	}

	if (identifier_start_characters.contains(m_remaining_file.front()))
	{
		start_token();
		add_to_token_until_pred(
			[&]
			{
				return !identifier_characters.contains(m_remaining_file.front());
			});

		m_token->m_flag = token::flag::valid_identifier;
		return *this;
	}

	start_token();
	add_to_token();
	return *this;
}

