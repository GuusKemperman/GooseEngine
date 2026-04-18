export module error_handling;

import stl;

import logger;

namespace ge
{
	export template<typename... types_t>
	struct basic_format_with_trace :
		format_with_location<types_t...>
	{
		template<typename arg_t> requires(std::is_constructible_v<format_with_location<types_t...>,
			const arg_t, const std::source_location&>)
		consteval basic_format_with_trace(const arg_t& a_str,
			const std::source_location& a_src = std::source_location::current(),
			std::stacktrace a_stacktrace = std::stacktrace::current()) :
			format_with_location<types_t...>(a_str, a_src),
			m_stack_trace(std::move(a_stacktrace))
		{
		}

		std::stacktrace m_stack_trace{};
	};

	export template<typename... types_t>
	using format_with_trace = basic_format_with_trace<std::type_identity_t<types_t>...>;

	export class exception
		: public std::exception
	{
	public:
		API exception(logger* a_logger,
			std::string_view a_msg,
			const std::source_location& a_src = std::source_location::current(),
			std::stacktrace a_stacktrace = std::stacktrace::current());

		template<typename... args_t>
		exception(logger* a_logger, format_with_trace<args_t...> a_format, args_t&&... a_args);

		API char const* what() const override;

		std::stacktrace m_stack_trace{};
		std::string m_msg{};
	};

	class logged_unexpected
		: public std::unexpected<std::string>
	{
	public:
		API logged_unexpected(logger& a_logger,
			std::string_view a_msg,
			const std::source_location& a_src = std::source_location::current());

		template<typename... args_t>
		logged_unexpected(logger& a_logger, format_with_location<args_t...> a_format, args_t&&... a_args);
	};
}

ge::exception::exception(logger* a_logger, std::string_view a_msg, const std::source_location& a_src,
	std::stacktrace a_stacktrace) :
	m_stack_trace(std::move(a_stacktrace)),
	m_msg(std::format("exception thrown: {}", a_msg))
{
	if (a_logger == nullptr)
	{
		return;
	}

	a_logger->log_raw(error, m_msg, a_src);
}

template <typename... args_t>
ge::exception::exception(logger* a_logger, format_with_trace<args_t...> a_format, args_t&&... a_args) :
	exception(a_logger, 
		std::format(a_format, std::forward<args_t>(a_args)...),
		a_format.m_src,
		a_format.m_stacktrace)
{
}

char const* ge::exception::what() const
{
	return m_msg.c_str();
}

ge::logged_unexpected::logged_unexpected(logger& a_logger, 
		std::string_view a_msg,
		const std::source_location& a_src) :
	std::unexpected<std::string>(std::format("unexpected: {}", a_msg))
{
	a_logger.log_raw(warning, error(), a_src);
}

template <typename ... args_t>
ge::logged_unexpected::logged_unexpected(logger& a_logger, format_with_location<args_t...> a_format, args_t&&... a_args) :
	logged_unexpected(a_logger, std::format(a_format, std::forward<args_t>(a_args)..., a_format.m_src))
{
}

