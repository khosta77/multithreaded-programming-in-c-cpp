#include <thread>
#include <mutex>
#include <condition_variable>

#include <iostream>

size_t count = 10'000;
std::mutex m;
std::condition_variable cv;

void ping()
{
    while(true)
    {
        std::unique_lock<std::mutex> lock(m);

        cv.wait( lock, [&](){ return ( count % 2 == 0 || count == 0 ); });
        if( count == 0 )
            break;

        // if( count % 2 == 0 )
        {
            std::cout << "\tping" << std::endl;
            count--;
            cv.notify_all();
        }
        std::cout << "ping>> " << count << std::endl;
    }
}

void pong()
{
    while(true)
    {
        std::unique_lock<std::mutex> lock(m);

        cv.wait( lock, [&](){ return ( count % 2 == 1 || count == 0 ); });
        if( count == 0 )
            break;

        // if( count % 2 == 1 )
        {
            std::cout << "pong" << std::endl;
            count--;
            cv.notify_all();
        }
        std::cout << "pong>> " << count << std::endl;
    }
}

int main()
{
    std::thread ping_t(ping);
    std::thread pong_t(pong);

    ping_t.join();
    pong_t.join();
    return 0;
}
