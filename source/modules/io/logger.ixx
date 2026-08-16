export module io:logger;

import stl;

namespace ge
{
	export enum severity
	{
		verbose,
		message,
		warning,
		error
	};

	export template<typename... types_t>
	struct basic_format_with_location
	{
		template<typename arg_t> requires( std::convertible_to< const arg_t&, std::format_string< types_t... > > )
		consteval basic_format_with_location(
			const arg_t& a_str,
			std::source_location a_src = std::source_location::current() )
			: m_str( a_str ),
			  m_src( std::move( a_src ) )
		{
		}

		std::format_string< types_t... > m_str;
		std::source_location m_src{};
	};

	export template<typename... types_t>
	using format_with_location = basic_format_with_location< std::type_identity_t< types_t >... >;

	export class logger
	{
	public:
		struct entry
		{
			severity m_severity{};
			std::string m_logged_text{};
			std::source_location m_src{};
		};

		API logger(
			std::ostream& a_output_stream = std::cout,
			std::ostream& a_err_stream = std::cerr );

		template<typename... T>
		void log(
			severity a_severity,
			format_with_location< T... > a_format,
			T&&... a_args );

		API void log_raw(
			severity a_severity,
			std::string_view a_msg,
			const std::source_location& a_src = std::source_location::current() );

		API auto get_logged_messages() const;

		API void clear();

		API void set_severity( severity a_severity );

		API severity get_severity() const;

		API static constexpr bool should_display( severity a_current_severity_level, severity a_message_severity );

		API void set_max_num_characters_stored( size_t a_max );

		API size_t get_max_num_characters_stored() const;

	private:
		void clear_excess_messages();

		std::reference_wrapper< std::ostream > m_output_stream;
		std::reference_wrapper< std::ostream > m_err_stream;
		mutable std::shared_mutex m_log_mut{};
		severity m_severity{};
		std::list< entry > m_log{};

		size_t m_num_characters_logged{};
		size_t m_max_num_characters_logged = 2097152; // 2mb
	};
}

ge::logger::logger( std::ostream& a_output_stream, std::ostream& a_err_stream )
	: m_output_stream( a_output_stream ),
	  m_err_stream( a_err_stream )
{
}

template<typename... T>
void ge::logger::log(
	severity a_severity,
	format_with_location< T... > a_format,
	T&&... a_args )
{
	std::string formatted = std::format( a_format.m_str, std::forward< T >( a_args )... );
	log_raw(
		a_severity,
		formatted,
		a_format.m_src );
}

void ge::logger::log_raw( severity a_severity, std::string_view a_msg, const std::source_location& a_src )
{
	std::string logged_text = std::format(
		"{}({}): '{}'\n",
		std::filesystem::path{ a_src.file_name() }.filename().string(),
		a_src.line(),
		a_msg );

	std::unique_lock _{ m_log_mut };

	m_num_characters_logged += logged_text.size();
	m_log.emplace_back(
		a_severity,
		logged_text,
		a_src
		).m_logged_text;

	clear_excess_messages();

	if( should_display( m_severity, a_severity ) )
	{
		std::ostream& stream = ( a_severity >= severity::warning ) ? m_err_stream : m_output_stream;
		stream << logged_text;
	}
}

auto ge::logger::get_logged_messages() const
{
	struct
	{
		std::reference_wrapper< const std::list< entry > > names;
		std::shared_lock< std::shared_mutex > lock{};

		API auto begin() const
		{
			return names.get().begin();
		}

		API auto end() const
		{
			return names.get().end();
		}

		API auto size() const
		{
			return names.get().size();
		}
	} mut_view_range{ m_log, std::shared_lock{ m_log_mut } };
	return std::ranges::owning_view( std::move( mut_view_range ) );
}

void ge::logger::clear()
{
	std::unique_lock _{ m_log_mut };
	m_log.clear();
	m_num_characters_logged = 0;
}

void ge::logger::set_severity( severity a_severity )
{
	std::unique_lock _{ m_log_mut };
	m_severity = a_severity;
}

ge::severity ge::logger::get_severity() const
{
	std::shared_lock _{ m_log_mut };
	return m_severity;
}

constexpr bool ge::logger::should_display( severity a_current_severity_level, severity a_message_severity )
{
	return a_message_severity >= a_current_severity_level;
}

void ge::logger::set_max_num_characters_stored( size_t a_max )
{
	std::unique_lock _{ m_log_mut };
	m_max_num_characters_logged = a_max;
	clear_excess_messages();
}

size_t ge::logger::get_max_num_characters_stored() const
{
	std::shared_lock _{ m_log_mut };
	return m_max_num_characters_logged;
}

void ge::logger::clear_excess_messages()
{
	while( m_num_characters_logged > m_max_num_characters_logged )
	{
		m_num_characters_logged -= m_log.front().m_logged_text.size();
		m_log.pop_front();
	}
}
