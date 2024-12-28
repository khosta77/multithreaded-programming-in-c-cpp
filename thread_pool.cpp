#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include <functional>
#include <iostream>
#include <exception>
#include <future>

class ThreadPool
{
private:
    enum class State
    {
        OK,
        STOPPING,
        STOPPED
    };

public:
    explicit ThreadPool(std::size_t N)
        : state_(State::OK)
        , count_threads_(N)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        for (std::size_t i = 0; i < N; i++)
        {
            threads_.emplace_back(&ThreadPool::ThreadFunction, this);
            //void (ThreadPool::*tf)() = &ThreadFunction;
        }
    }
    ~ThreadPool()
    {
        Stop();
        for (std::thread& t : threads_)
            t.join();
    }
    // ...no copy, no move...

    template<typename T>
    std::future<std::result_of_t<T()>> AddTask(T&& f)
    {
        using R = std::result_of_t<T()>;

        std::unique_lock<std::mutex> lock(mutex_);
        if (state_ != State::OK)
            return std::future<R>();

        std::promise<R> prom;
        std::future<R> res = prom.get_future();

        tasks_.emplace_back(
            [f = std::move(f), p = std::move(prom)]() mutable
            {
                try
                {
                    if constexpr (!std::is_same_v<R, void>)
                        p.set_value(f());
                    else
                        f();
                }
                catch (...)
                {
                    p.set_exception(std::current_exception());
                }
            }
        );
        cond_var_.notify_one();
        return res;
    }

    void Stop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (state_ != State::OK)
            return;

        state_ = State::STOPPING;
        cond_var_.notify_all();
    }

    /*std::exception_ptr ExtractException()
    {
        std::unique_lock<std::mutex> lock(mutex_);

        std::exception_ptr result = std::move(eptr_);
        eptr_ = nullptr;
        return result;
    }*/

private:
    void ThreadFunction()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (state_ == State::OK)
        {
            // while (!pred())
            cond_var_.wait(lock, [&](){ return state_ != State::OK || !tasks_.empty(); });
            while (!tasks_.empty())
            {
                std::packaged_task<void()> f = std::move(tasks_.front());
                tasks_.pop_front();

                lock.unlock();
                f();
                lock.lock();
            }
        }

        count_threads_--;
        if (count_threads_ == 0)
            state_ = State::STOPPED;
    }

private:
    std::vector<std::thread> threads_;
    std::deque<std::packaged_task<void()>> tasks_;

    std::size_t count_threads_;

    State state_;
    std::mutex mutex_;
    std::condition_variable cond_var_;

    //std::exception_ptr eptr_; // ...vector...
};


int main()
{
    ThreadPool p(23);

    std::cout << ">!> " << std::this_thread::get_id() << std::endl;
    auto res = p.AddTask([]() -> int { std::cout << ">> " << std::this_thread::get_id() << std::endl; return 2+3; });
    if (!res.valid()) { /* */ }
    else
    {
        res.wait();
        std::cout << res.get() << std::endl;
    }

    using namespace std::chrono_literals;
    p.AddTask([]() { std::this_thread::sleep_for(2s); std::cout << "!>> >>> " << std::this_thread::get_id() << std::endl; });

    p.Stop();


    auto async_res = std::async(std::launch::deferred, []() { std::cout << ">!!!> " << std::this_thread::get_id() << std::endl; });
    async_res.get();
}


/*
// 4 * 32
union m128_union {
    __m128 content;
    struct {
        int x;
        int y;
        int z;
        int a;
    };
};*/