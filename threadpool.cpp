#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include <functional>
#include <exception>
#include <future>
#include <iomanip>
#include <type_traits>
#include <functional>

template<typename Func, typename... Args>
using result_of_t = std::invoke_result_t<Func, Args...>;

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
    explicit ThreadPool( const size_t& N ) : state_(State::OK), count_active_(N)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        for( size_t i = 0; i < N; ++i )
        {
            threads_.emplace_back(&ThreadPool::ThreadFunction, this);
            // void (ThreadPool::*tf)() = &ThreadFunction;
        }
    }
    // ...no copy, no move...

    ~ThreadPool()
    {
        Stop();
        for( std::thread& t : threads_ )
        {
            t.join();
        }
    }

    //template<typename R = void>
    //std::future<R>  // Получает результат
    //AddTask( std::function<R()> f )

    template<typename T>
    std::future<result_of_t<T>>  // Получает результат
    AddTask( T&& f )
    {
        using R = result_of_t<T>;
        // std::promise<R> promise;  // Позволяет создать результат
        std::unique_lock<std::mutex> lock(mutex_);
        if( state_ != State::OK )
            return std::future<R>();
        

        std::promise<R> prom;
        std::future<R> res = prom.get_future();

        tasks_.emplace_back(
                    [ f = std::move(f), p = std::move(prom) ]() mutable
                    {
                        try
                        {
                            p.set_value( f() );
                        }
                        catch( ... )
                        {
                            p.set_exception( std::current_exception() );
                        };
                    }
                );
        cond_var_.notify_one();
        return res;
    }

    void Stop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if( state_ != State::OK )
            return;

        state_ = State::STOPPING;
        cond_var_.notify_all();
    }

    // можно извлечь ошибку в отдельной функции, надо не забыть mutex

private:
    void ThreadFunction()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while( state_ == State::OK )
        {
            cond_var_.wait( lock, [&]() { return ( !tasks_.empty()
                        && state_ != State::OK ); } );
            while( !tasks_.empty() )
            {
                // std::function<void()> f = std::move(tasks_.front());
                std::packaged_task<void()> f = std::move(tasks_.front());

                tasks_.pop_front();

                lock.unlock();
                f();
/*
                try
                {
                    f();
                }
                catch( ... )
                {
                    eptr_ = std::current_exception();
                };
*/

                lock.lock();
            }
        }

        --count_active_;
        if( count_active_ == 0 )
            state_ = State::STOPPED;
    }

private:
    std::vector<std::thread> threads_;
    std::deque<std::packaged_task<void()>> tasks_;

    size_t count_active_;

    State state_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
    
    std::exception_ptr eptr_;  // ...vector...
};

bool task()
{
    int x = 1;
    while(x)
    {
        x -= 1; 
    }
    return true;
}

int main()
{
    const size_t I = 4;
    ThreadPool p(I);
    for( size_t i = 0; i < I; ++i )
    {
        p.AddTask([]() -> bool { return task(); });
    }

    std::cout << "next" << std::endl;
    auto res = p.AddTask([]() -> int { return ( task() ? 2 : 3 ); });
    if( !res.valid() )
    {
        std::cerr << "..." << std::endl;
    }
    else
    {
        std::cout << "success\n";
        res.wait();
        std::cout << "wait\n";
        res.get();
        std::cout << "get\n";
    }
    p.Stop();
    std::cout << "stop\n";
    return 0;
}
