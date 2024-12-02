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

    bool AddTask( std::function<void()> f )
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if( state_ != State::OK )
            return false;
        tasks_.push_back(f);
        cond_var_.notify_one();
        return true;
    }

    void Stop()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if( state_ != State::OK )
            return;

        state_ = State::STOPPING;
        cond_var_.notify_all();
    }

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
                std::function<void()> f = std::move(tasks_.front());
                tasks_.pop_front();

                lock.unlock();
                try
                {
                    f();
                }
                catch( std::exception& exc )
                {
                    std::cerr << "!> Exception! " << exc.what() << std::endl;
                    // Надо в метрику записывать
                }
                catch( ... )
                {
                    std::cerr << "!> Exception! Unknown!" << std::endl;
                };
                lock.lock();
            }
        }

        --count_active_;
        if( count_active_ == 0 )
            state_ = State::STOPPED;
    }

private:
    std::vector<std::thread> threads_;
    std::deque<std::function<void()>> tasks_;

    size_t count_active_;

    State state_;
    std::mutex mutex_;
    std::condition_variable cond_var_;
};

int main()
{
    const size_t I = 8;
    ThreadPool p(I);
    for( size_t i = 0; i < I; ++i )
    {
        std::cout << i << ") " <<  std::boolalpha << p.AddTask([](){while(1) { 2 + 3 / 5 * 10;}}) << std::endl;
    }
    return 0;
}
