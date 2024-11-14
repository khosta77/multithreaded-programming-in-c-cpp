#include <iostream>
#include <vector>
#include <chrono>
#include <string>
#include <thread>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

const int BUFFER_SIZE = 5;

class Server
{
private:
    int _listener;
    struct sockaddr_in _addr;

    timeval _timeout;

    int sock;

    std::string readFromRecv()
    {
        char buffer[BUFFER_SIZE];
        std::vector<char> message;
        size_t bytes_received;
        while( ( bytes_received = recv( sock, buffer, BUFFER_SIZE - 1, 0 ) ) > 0 )
        {
            message.insert(message.end(), buffer, buffer + bytes_received);
            if( message.back() == '\n' )
                break;
        }
        std::string complete_message(message.begin(), ( message.end() - 1 ) );
        return complete_message;
    }

    void sendToSock( const std::string& msg )
    {
        const char* dataPtr = msg.c_str();
        size_t dataSize = msg.length();
        size_t totalSent = 0;
        while( totalSent < dataSize )
        {
            int bytesSent = send( sock, ( dataPtr + totalSent ), ( dataSize - totalSent ), 0 );
            if( bytesSent == -1 )
                break;
            totalSent += bytesSent;
        }
    }

    void fillSockNew()
    {
        if( sock = accept( _listener, NULL, NULL ); sock >= 0 )
            std::cout << "new socket init: " << sock << std::endl;
    }

    void clearSocket()
    {
        std::cout << "socket " << sock << " free" << std::endl;
        close(sock);
    }

    void putIntToVector( std::vector<int>& values, const std::string& buffer )
    {
        try
        {
            values.push_back(std::stoi(buffer));
        }
        catch(...)
        {
            values.push_back(0);
        };
    }

    std::vector<int> split( const std::string &content, const char del )
    {
        std::vector<int> values;
        std::string buffer = "";
        for (size_t i = 0, I = content.size(); i < I; ++i) {
            if (content[i] != del)
                buffer += content[i];
            else {
                putIntToVector( values, buffer );
                buffer = "";
            }
        }
        putIntToVector( values, buffer );
        buffer.clear();
        return values;
    }

    std::string sum( const std::vector<int>& values )
    {
        int x = 0;
        for( const auto& value : values )
            x += value;
        return std::to_string(x);
    }

    void process()
    {
        std::string content = readFromRecv();
        if( content.empty() )
            clearSocket();
        else
        {
            auto values = split( content, ' ' ); 
            std::this_thread::sleep_for(std::chrono::seconds(1));
            sendToSock( ( sum( values ) + "\n" ) );
        }
    }

public:
    Server( const std::string& IP, const int& PORT )
    {
        _listener = socket( AF_INET, SOCK_STREAM, 0 );
        if( _listener < 0 )
            std::cerr << std::format("listener = {}!\n", _listener);

        _addr.sin_family = AF_INET;
        _addr.sin_port = htons(PORT);
        _addr.sin_addr.s_addr = inet_addr(IP.c_str());

        if( int _bind = bind( _listener, ( struct sockaddr* )&_addr, sizeof(_addr) ); _bind < 0 )
            std::cerr << std::format("bind = {}!\n", _bind);
        listen( _listener, 1 );

        std::cout << "server in system address: " << IP << ":" << PORT << std::endl;
        
        _timeout.tv_sec = 15;
        _timeout.tv_usec = 0;

    }

    int run()
    {
        while( true )
        {
            fillSockNew();
            if( sock > 0 )
            {
                process();
                clearSocket();
            }
        }
        return 0;
    }

};  // Server

int main()
{
    Server server( "127.0.0.1", 8000 );
    return server.run();
}
