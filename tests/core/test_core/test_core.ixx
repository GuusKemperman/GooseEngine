export module test_core;

import stl;

namespace ge::test_core
{
	export class context
	{
	public:
	};

	export class test_exception
		: public std::exception
	{
	public:
		API test_exception(const std::source_location& a_src = std::source_location::current()) :
			m_src(a_src),
			m_what(std::format("{}({})",
				m_src.file_name(), 
				m_src.line()))
		{
		}

		API test_exception(std::string_view a_what, const std::source_location& a_src = std::source_location::current()) :
			test_exception(std::move(a_src))
		{
			m_what = std::format("{} - {}", m_what, a_what);
		}

		API const char* what() const override { return m_what.c_str(); }

		std::source_location m_src{};
		std::string m_what{};
	};

	export struct threading_test_arg
	{
		size_t m_iteration{};
		size_t m_thread_idx{};
	};

	export template<typename func_t>
	auto test_threading(func_t&& a_func, size_t num_iterations_per_thread = 10, size_t num_threads = 2)
		requires(std::is_invocable_v<func_t, const threading_test_arg&>)
	{
		auto work =
			[&](size_t a_thread_idx)
			{
				std::thread::id id = std::this_thread::get_id();
				std::hash<std::thread::id> hash{};
				size_t seed = hash(id);
				std::default_random_engine eng{ static_cast<std::uint32_t>(seed) };

				for (size_t i = 0; i < num_iterations_per_thread; i++)
				{
					auto start = std::chrono::high_resolution_clock::now();
					int num_ms_to_wait = std::uniform_int_distribution{ 1, 10 }(eng);
					auto expected_end = start + std::chrono::milliseconds{ num_ms_to_wait };

					while (std::chrono::high_resolution_clock::now() < expected_end)
					{
						
					}

					threading_test_arg arg{ i, a_thread_idx };
					std::invoke(a_func, arg);
				}
			};

		std::vector<std::thread> threads{};
		threads.reserve(num_threads);

		for (size_t i = 0; i < num_threads; i++)
		{
			threads.emplace_back(work, i);
		}

		for (std::thread& t : threads)
		{
			t.join();
		}

		struct
		{
			size_t m_num_threads{};
			size_t m_num_iterations_per_thread{};
		} params{ num_threads, num_iterations_per_thread };
		return params;
	}

	namespace assert
	{
		export [[noreturn]] API void failure(
			const std::source_location& src = std::source_location::current())
		{
			throw test_exception{ src };
		}

		export [[noreturn]] API void failure(
			std::string_view msg,
			const std::source_location& src = std::source_location::current())
		{
			throw test_exception{ msg, src };
		}

		export API void is_true(
			bool cond,
			const std::source_location& src = std::source_location::current())
		{
			if (!cond)
			{
				failure(src);
			}
		}

		export API void is_false(
			bool cond,
			const std::source_location& src = std::source_location::current())
		{
			return is_true(!cond, src);
		}

		export template<typename exception_t, typename func_t> requires std::invocable<func_t>
		exception_t expect_exception(
			func_t&& a_func,
			const std::source_location& src = std::source_location::current())
		{
			try
			{
				(void)std::invoke(a_func);
				failure(src);
			}
			catch (const exception_t& e)
			{
				return e;
			}
			catch (...)
			{
				throw test_exception{ "different exception type", src };
			}
		}

		export void is_null(
			const auto& ptr,
			const std::source_location& src = std::source_location::current()) requires requires
		{
			{ ptr == nullptr } -> std::same_as<bool>;
		}
		{
			return is_true(ptr == nullptr, src);
		}

		export void is_not_null(
			const auto& ptr,
			const std::source_location& src = std::source_location::current()) requires requires
		{
			{ ptr != nullptr } -> std::same_as<bool>;
		}
		{
			return is_true(ptr != nullptr, src);
		}

		template<typename t1, typename t2>
		void assert_template(std::string_view operator_as_txt,
			bool failed,
			const t1& lhs,
			const t2& rhs,
			const std::source_location& src)
		{
			if (failed)
			{
				return;
			}

			if constexpr (std::formattable<t1, char> && std::formattable<t2, char>)
			{
				failure(std::format("assert failed: {} {} {}", lhs, operator_as_txt, rhs), src);
			}
			else
			{
				failure(src);
			}
		}

		export template<typename t1, typename t2>
		void is_eq(
			const t1& lhs,
			const t2& rhs,
			const std::source_location& src = std::source_location::current()) requires requires
					{
						{ lhs == rhs } -> std::same_as<bool>;
					}
		{
			assert_template("==", lhs == rhs, lhs, rhs, src);
		}

		export void is_ne(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current()) requires requires
		{
			{ lhs != rhs } -> std::same_as<bool>;
		}
		{
			assert_template("!=", lhs != rhs, lhs, rhs, src);
		}

		export void is_lt(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current()) requires requires
		{
			{ lhs < rhs } -> std::same_as<bool>;
		}
		{
			assert_template("<", lhs < rhs, lhs, rhs, src);
		}

		export void is_gt(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current()) requires requires
		{
			{ lhs > rhs } -> std::same_as<bool>;
		}
		{
			assert_template(">", lhs > rhs, lhs, rhs, src);
		}

		export void is_le(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current()) requires requires
		{
			{ lhs <= rhs } -> std::same_as<bool>;
		}
		{
			assert_template("<=", lhs <= rhs, lhs, rhs, src);
		}

		export void is_ge(
			const auto& lhs,
			const auto& rhs,
			const std::source_location& src = std::source_location::current()) requires requires
		{
			{ lhs >= rhs } -> std::same_as<bool>;
		}
		{
			assert_template(">=", lhs >= rhs, lhs, rhs, src);
		}
	}


}
