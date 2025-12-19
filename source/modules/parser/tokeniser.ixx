export module parser:tokeniser;

export import std;

namespace ge
{
	export class tokeniser
	{
	public:
		std::string_view m_remaining_file{};

		struct token
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
		};

		API token consume_token()
		{
			token token{};

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
					token.m_str = m_remaining_file.substr(0, 0);
				};

			auto discard_char =
				[&](size_t amount = 1)
				{
					m_remaining_file = m_remaining_file.substr(amount);
				};

			auto add_to_token =
				[&](size_t amount = 1)
				{
					token.m_str = { token.m_str.data(), std::min(token.m_str.data() +  token.m_str.size() + amount, 
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

			while (!m_remaining_file.empty() 
				&& std::isspace(static_cast<unsigned char>(m_remaining_file.front())))
			{
				token.m_flag = token::flag::white_space;
				discard_char();
			}

			if (m_remaining_file.empty()
				|| token.m_flag == token::flag::white_space)
			{
				token.m_flag = token::flag::white_space;
				return token;
			}

			if (m_remaining_file.starts_with("[["))
			{
				discard_char(2);

				start_token();
				token.m_flag = token::flag::attribute;
				add_to_token_until_match("]]");

				discard_char(2);
				return token;
			}

			if (m_remaining_file.starts_with("/*"))
			{
				discard_char(2);
				start_token();
				token.m_flag = token::flag::comment;
				add_to_token_until_match("*/");
				discard_char(2);
				return token;
			}

			if (m_remaining_file.starts_with("//"))
			{
				discard_char(2);
				start_token();
				token.m_flag = token::flag::comment;
				add_to_token_until_match("\n");
				discard_char();
				return token;
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
				return token;
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

				return token;
			}

			if (m_remaining_file.starts_with("::")
				|| m_remaining_file.starts_with("->")
				|| m_remaining_file.starts_with("&&"))
			{
				start_token();
				add_to_token(2);
				return token;
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
				return token;
			}

			if (number_literal_start_characters.contains(m_remaining_file.front()))
			{
				start_token();
				add_to_token_until_pred(
					[&]
					{
						return !number_literal_characters.contains(m_remaining_file.front());
					});
				return token;
			}

			if (identifier_start_characters.contains(m_remaining_file.front()))
			{
				start_token();
				add_to_token_until_pred(
					[&]
					{
						return !identifier_characters.contains(m_remaining_file.front());
					});

				token.m_flag = token::flag::valid_identifier;
				return token;
			}

			start_token();
			add_to_token();
			return token;
		}
	};
}
