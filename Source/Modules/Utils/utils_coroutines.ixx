export module utils:coroutines;

import std;
import modules;
import :memory;

namespace ge
{
	struct coroutine_handle
	{
        API constexpr auto operator<=>(const coroutine_handle& other) const;

        API static constexpr bool await_ready() { return false; }

        API void await_suspend(std::coroutine_handle<> a_handle);

        API static constexpr void await_resume() {}

        float m_continue_at_time{};
		shared_ptr<std::coroutine_handle<>> m_handle = ge::make_shared_ptr<std::coroutine_handle<>>();
	};

	export struct coroutine
    {
        struct promise_type
        {
            API static constexpr coroutine get_return_object() { return {}; }
            API static constexpr std::suspend_never initial_suspend() { return {}; }
            API static constexpr std::suspend_never final_suspend() noexcept { return {}; }
            API static constexpr void return_void() {}
            API static constexpr void unhandled_exception() {}
        };
    };

    export class coroutine_context
    {
    public:
        API void tick(float a_dt);

        template<typename... args_t, std::invocable<coroutine_context&, args_t...> func_t>
        void start_coroutine(func_t&& a_func, args_t&&... a_args) requires(std::same_as<std::invoke_result_t<func_t, coroutine_context&, args_t...>, coroutine>);

        API coroutine_handle wait_for(float a_seconds);

    private:
        float m_current_time{};
        std::priority_queue<coroutine_handle> m_queue{};
    };
}

constexpr auto ge::coroutine_handle::operator<=>(const coroutine_handle& other) const
{
    return m_continue_at_time <=> other.m_continue_at_time;
}

void ge::coroutine_handle::await_suspend(std::coroutine_handle<> a_handle)
{
	*m_handle = std::move(a_handle);
}

template <typename ... args_t, std::invocable<ge::coroutine_context&, args_t...> func_t>
void ge::coroutine_context::start_coroutine(func_t&& a_func, args_t&&... a_args) requires (std::same_as<std::invoke_result_t<func_t,
    coroutine_context&, args_t...>, coroutine>)
{
    (void)std::invoke(a_func, *this, std::forward<args_t>(a_args)...);
}

void ge::coroutine_context::tick(float a_dt)
{
    m_current_time += a_dt;

    while (!m_queue.empty()
        && m_queue.top().m_continue_at_time <= m_current_time)
    {
        std::coroutine_handle<> handle = std::move(*m_queue.top().m_handle);
        m_queue.pop();

    	if (handle)
        {
            handle.resume();
        }
    }
}

ge::coroutine_handle ge::coroutine_context::wait_for(float a_seconds)
{
    m_queue.emplace(m_current_time + a_seconds);
    return m_queue.top();
}

