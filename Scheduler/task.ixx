export module scheduler.task.task;
import scheduler.task.tasktype;
import <coroutine>;
import <memory>;
import <vector>;
import <concepts>;
import <optional>;
import <stdexcept>;
import <chrono>;

template <typename T, TaskType task_type>
class promise;

export template <typename T, TaskType task_type>
class base_task
{
    template <typename U, TaskType tt2>
    friend class promise_type;

    friend class EventLoop;
public:
    using promise_type = ::promise<T, task_type>;
    explicit base_task(std::coroutine_handle<promise<T, task_type>> h) noexcept
        : coro_(h)
    {}

    base_task(base_task& t) = delete;
    base_task& operator=(base_task& t) = delete;
    base_task(base_task&& t) noexcept : coro_(std::exchange(t.coro_, {}))
    {

    }
    base_task& operator=(base_task&& t) noexcept
    {
        coro_ = std::exchange(t.coro_, {});
        return *this;
    }

    ~base_task()
    {
        if (coro_)
            coro_.destroy();
    }

    bool await_ready() noexcept
    {
        return false;
    }

    template <typename PROMISE> requires is_promise<PROMISE>
    std::coroutine_handle<> await_suspend(std::coroutine_handle<PROMISE> previous) noexcept
    {
        auto& previous_promise = previous.promise();
        auto& cur_promise = coro_.promise();

        void* prev_addr = previous.address();
        void* cur_addr = coro_.address();
        cur_promise.recursive_info = previous_promise.recursive_info;
        cur_promise.recursive_info->push_back({ cur_addr, task_type });

        if constexpr (task_type != TaskType::CPU)
            return std::noop_coroutine();
        return coro_;
    }

    T await_resume()
    {
        if constexpr (std::is_void_v<T>)
            return;
        else
        {
            if (!coro_.promise().value)
                throw std::runtime_error("Callee returned without yielding anything.");

            auto val = std::move(*coro_.promise().value);
            coro_.promise().value = std::nullopt;
            return val;
        }
    }

private:
    std::pair<std::coroutine_handle<>, TaskType> get_handle_to_resume()
    {
        auto& info = coro_.promise().recursive_info;
        return { std::coroutine_handle<>::from_address(info->back().first), info->back().second };
    }

    std::size_t get_handles_count()
    {
        return coro_.promise().recursive_info->size();
    }

    std::coroutine_handle<promise_type> coro_;
};

export template <typename T>
using task = base_task<T, TaskType::CPU>;

export template <typename T>
using io_task = base_task<T, TaskType::IO>;

export using sleep_task = base_task<std::chrono::milliseconds, TaskType::SLEEP>;