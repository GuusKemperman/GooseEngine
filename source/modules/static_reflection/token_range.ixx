export module static_reflection:tokeniser;

import stl;
import :source_error;

namespace ge
{
	using namespace std::string_view_literals;

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

		API bool operator==( const token& ) const = default;
	};

	export class token_iterator
	{
	public:
		using difference_type = std::ptrdiff_t;
		using value_type = token;

		API token_iterator() = default;
		API token_iterator( std::string_view file, size_t num_characters_to_skip = 0ull )
			: m_file( file ), m_num_characters_parsed( num_characters_to_skip )
		{
		}

		API const token& operator*() const
		{
			return m_token;
		}

		API const token* operator->() const
		{
			return &m_token;
		}

		API token_iterator& operator++();

		API token_iterator operator++( int )
		{
			auto tmp = *this;
			++*this;
			return tmp;
		}

		API bool operator==( const token_iterator& other ) const
		{
			return m_num_characters_parsed == other.m_num_characters_parsed
			       && m_file == other.m_file;
		}

		API bool operator!=( const token_iterator& other ) const
		{
			return m_num_characters_parsed != other.m_num_characters_parsed
			       || m_file != other.m_file;
		}

		API int parentheses_count() const
		{
			return m_parentheses_count;
		}

		API int curly_bracket_count() const
		{
			return m_curly_brackets_count;
		}

		API size_t num_characters_parsed() const
		{
			return m_num_characters_parsed;
		}

		API source_location get_source() const
		{
			return m_location;
		}

	private:
		std::string_view m_file{};
		size_t m_num_characters_parsed{};
		token m_token{};
		source_location m_location{ 1u, 1u };
		int m_parentheses_count{};
		int m_curly_brackets_count{};
	};

	export class token_range
	{
	public:
		API token_range( std::string_view file )
			: m_file( file )
		{
		}

		API token_iterator begin() const
		{
			token_iterator it{ m_file };
			++it;
			return it;
		}

		API token_iterator end() const
		{
			token_iterator it{ m_file, m_file.size() + 1 };
			return it;
		}

	private:
		std::string_view m_file{};
	};
}

ge::token_iterator& ge::token_iterator::operator++()
{
	auto remaining_file = [&]
	{
		return m_file.substr( m_num_characters_parsed );
	};

	auto peek =
		[&]( size_t index, unsigned char ch )
	{
		if( m_num_characters_parsed + index >= m_file.size() )
		{
			return false;
		}
		return m_file[ index + m_num_characters_parsed ] == ch;
	};

	auto start_token =
		[&]
	{
		m_token = {};
		m_token.m_str = m_file.substr( m_num_characters_parsed, 0 );
	};

	auto discard_char =
		[&]( size_t amount = 1 )
	{
		// This is the only place where m_num_characters_parsed is incremented, 
		// so we also update the location count here
		for( size_t i = 0; i < amount; i++ )
		{
			bool is_new_line = peek( i, '\n' );

			m_location.m_line_number += is_new_line;
			m_location.m_column_number++;

			if( is_new_line )
			{
				m_location.m_column_number = 1u;
			}
		}

		m_num_characters_parsed += amount;
	};

	auto add_to_token =
		[&]( size_t amount = 1 )
	{
		m_token.m_str = { m_token.m_str.data(), std::min(
			                  m_token.m_str.data() + m_token.m_str.size() + amount,
			                  m_file.data() + m_file.size() ) };
		discard_char( amount );
	};

	auto add_to_token_until_pred =
		[&]( const auto& pred )
	{
		while( m_num_characters_parsed < m_file.size() )
		{
			if( pred() )
			{
				break;
			}

			add_to_token();
		}
	};

	auto add_to_token_until_match =
		[&]( std::string_view match )
	{
		add_to_token_until_pred(
			[&]
			{
				return m_file.substr( m_num_characters_parsed ).starts_with( match );
			}
			);
	};

	m_token = {};

	if( m_num_characters_parsed == m_file.size() )
	{
		m_num_characters_parsed = m_file.size() + 1;
		return *this;
	}

	bool white_space_found = false;
	while( m_num_characters_parsed < m_file.size()
	       && std::isspace( static_cast< unsigned char >( m_file[ m_num_characters_parsed ] ) ) )
	{
		white_space_found = true;
		discard_char();
	}

	if( white_space_found )
	{
		m_token = {};
		m_token.m_str = " "sv;
		m_token.m_flag = token::flag::white_space;
		return *this;
	}

	if( remaining_file().starts_with( "[["sv ) )
	{
		discard_char( 2 );

		start_token();
		m_token.m_flag = token::flag::attribute;
		add_to_token_until_match( "]]"sv );

		discard_char( 2 );
		return *this;
	}

	if( remaining_file().starts_with( "/*"sv ) )
	{
		discard_char( 2 );
		start_token();
		m_token.m_flag = token::flag::comment;
		add_to_token_until_match( "*/"sv );
		discard_char( 2 );
		return *this;
	}

	if( remaining_file().starts_with( "//"sv ) )
	{
		discard_char( 2 );
		start_token();
		m_token.m_flag = token::flag::comment;
		add_to_token_until_match( "\n"sv );
		discard_char();
		return *this;
	}

	if( remaining_file().starts_with( "R\"("sv )
	    || remaining_file().starts_with( "LR\""sv )
	    || remaining_file().starts_with( "u8R\""sv )
	    || remaining_file().starts_with( "uR\""sv )
	    || remaining_file().starts_with( "UR\""sv ) )
	{
		start_token();
		add_to_token_until_match( ")\""sv );
		add_to_token( 2 );
		return *this;
	}

	if( remaining_file().starts_with( '\"' )
	    || remaining_file().starts_with( "L\""sv )
	    || remaining_file().starts_with( "u8\""sv )
	    || remaining_file().starts_with( "u\""sv )
	    || remaining_file().starts_with( "U\""sv ) )
	{
		start_token();

		// Find the opening quote
		add_to_token_until_match( "\""sv );

		while( !remaining_file().empty() )
		{
			if( !peek( 0, '\\' ) && peek( 1, '"' ) )
			{
				add_to_token( 2 );
				break;
			}
			add_to_token();
		}

		return *this;
	}

	if( remaining_file().starts_with( '\'' ) )
	{
		start_token();
		add_to_token();

		if( remaining_file().starts_with( R"(\'')" ) )
		{
			add_to_token( 3 );
		}
		else
		{
			add_to_token( 2 );
		}

		return *this;
	}

	if( remaining_file().starts_with( "::"sv )
	    || remaining_file().starts_with( "->"sv )
	    || remaining_file().starts_with( "&&"sv ) )
	{
		start_token();
		add_to_token( 2 );
		return *this;
	}

	static constexpr std::string_view identifier_start_characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_"sv;
	static constexpr std::string_view identifier_characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"sv;

	static constexpr std::string_view number_literal_start_characters = "0123456789."sv;
	static constexpr std::string_view number_literal_characters = "0123456789'.-eEb"sv;
	static constexpr std::string_view hex_characters = "0123456789'abcdefABCDEF"sv;

	if( remaining_file().starts_with( "0x"sv )
	    || remaining_file().starts_with( "0X"sv ) )
	{
		start_token();
		add_to_token( 2 );
		add_to_token_until_pred(
			[&]
			{
				return !hex_characters.contains( remaining_file().front() );
			} );
		return *this;
	}

	if( number_literal_start_characters.contains( remaining_file().front() ) )
	{
		start_token();
		add_to_token_until_pred(
			[&]
			{
				return !number_literal_characters.contains( remaining_file().front() );
			} );
		return *this;
	}

	if( identifier_start_characters.contains( remaining_file().front() ) )
	{
		start_token();
		add_to_token_until_pred(
			[&]
			{
				return !identifier_characters.contains( remaining_file().front() );
			} );

		m_token.m_flag = token::flag::valid_identifier;
		return *this;
	}

	start_token();
	add_to_token();

	if( m_token.m_str == "("sv )
	{
		m_parentheses_count++;
	}
	else if( m_token.m_str == ")"sv )
	{
		m_parentheses_count--;
	}
	else if( m_token.m_str == "{"sv )
	{
		m_curly_brackets_count++;
	}
	else if( m_token.m_str == "}"sv )
	{
		m_curly_brackets_count--;
	}
	return *this;
}
