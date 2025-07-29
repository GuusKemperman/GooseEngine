export module logger;

import std;
import modules;

namespace ge
{
	export enum severity
	{
		verbose,
		message,
		warning,
		error,
		fatal,
	};

	export class logger :
		public modules::module<logger>
	{
	public:
		struct entry
		{
			severity m_severity{};
			std::string m_logged_text{};
			std::source_location m_src{};
		};

		template <typename... T>
		void log(severity a_severity,
			std::format_string<T...> a_format, 
			T&&... a_args, 
			std::source_location a_src = std::source_location::current());

		API void log(severity a_severity,
			std::string_view a_msg,
			std::source_location a_src = std::source_location::current());

		API auto get_logged_messages() const;

	private:
		mutable std::shared_mutex m_log_mut{};
		std::vector<entry> m_log{};
	};
}

namespace logger
{
	export class module :
		public ge::logger
	{
	public:
		using logger::logger;
	};
}

template <typename ... T>
void ge::logger::log(severity a_severity, std::format_string<T...> a_format, T&&... a_args, std::source_location a_src)
{
	std::string formatted = std::format(a_format, std::forward<T>(a_args)...);
	log(a_severity, 
		formatted,
		a_src);
}

void ge::logger::log(severity a_severity, std::string_view a_msg, std::source_location a_src)
{
	std::string logged_text = std::format("{}({}) - {}\n",
		a_src.file_name(),
		a_src.line(),
		a_msg);

	{
		std::unique_lock _{ m_log_mut };
		m_log.emplace_back(a_severity,
			logged_text,
			a_src
		).m_logged_text;
		std::cout << logged_text;
	}

	if (a_severity == severity::fatal)
	{
		throw std::runtime_error{ std::format("fatal message: {}", logged_text) };
	}
}

auto ge::logger::get_logged_messages() const
{
	struct
	{
		std::reference_wrapper<const std::vector<entry>> names;
		std::shared_lock<std::shared_mutex> lock{};

		API auto begin() const { return names.get().begin(); }
		API auto end() const { return names.get().end(); }
	} mut_view_range{ m_log, std::shared_lock{ m_log_mut } };
	return std::ranges::owning_view(std::move(mut_view_range));
}
