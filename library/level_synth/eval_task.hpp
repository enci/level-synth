#pragma once

#include <coroutine>
#include <string>
#include <optional>

namespace ls {

struct eval_step {
    std::string label;
    float progress = 0.f; // 0..1
};

class eval_task {
public:
    struct promise_type {
        std::optional<eval_step> current_step;

        eval_task get_return_object() {
            return eval_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }

        std::suspend_always yield_value(eval_step step) {
            current_step = std::move(step);
            return {};
        }

        void return_void() {}
        void unhandled_exception() { std::rethrow_exception(std::current_exception()); }
    };

    eval_task(std::coroutine_handle<promise_type> h) : m_handle(h) {}
    ~eval_task() { if (m_handle) m_handle.destroy(); }

    // Move only
    eval_task(const eval_task&) = delete;
    eval_task& operator=(const eval_task&) = delete;
    eval_task(eval_task&& o) noexcept : m_handle(o.m_handle) { o.m_handle = nullptr; }
    eval_task& operator=(eval_task&& o) noexcept {
        if (m_handle) m_handle.destroy();
        m_handle = o.m_handle;
        o.m_handle = nullptr;
        return *this;
    }

    bool done() const { return !m_handle || m_handle.done(); }
    void resume() { if (m_handle && !m_handle.done()) m_handle.resume(); }

    std::optional<eval_step> current_step() const {
        return m_handle ? m_handle.promise().current_step : std::nullopt;
    }

    // Run to completion (normal evaluation mode)
    void run() { while (!done()) resume(); }

private:
    std::coroutine_handle<promise_type> m_handle;
};

} // namespace ls