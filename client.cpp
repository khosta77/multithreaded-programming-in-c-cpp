#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <type_traits>
#include <iostream>
#include <exception>
#include <sstream>
#include <vector>
#include <cassert>
#include <string>

class Client
{
private:
    std::string _ip;
    int _port;
    int sock;
    struct sockaddr_in addr;    

    void connectToServer()
    {
        sock = socket( AF_INET, SOCK_STREAM, 0 );
        if( sock < 0 )
            std::cerr << std::format( "sock = {}!\n", sock );

        addr.sin_family = AF_INET;
        addr.sin_port = htons(_port);
        addr.sin_addr.s_addr = inet_addr(_ip.c_str());

        if( connect( sock, ( struct sockaddr* )&addr, sizeof(addr) ) < 0 )
            std::cerr << std::format( "connect!\n" );
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

    std::string readFromRecv()
    {
        const int BUFFER_SIZE = 1024;
        char buffer[BUFFER_SIZE];
        std::vector<char> message;
        size_t bytes_received;
        while( ( bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0) ) > 0 )
        {
            message.insert( message.end(), buffer, buffer + bytes_received );
            if ( message.back() == '\n' )
                break;
        }
        std::string complete_message( message.begin(), ( message.end() - 1 ) );
        return complete_message;
    }

public:
    Client( const std::string& IP, const int& PORT ) : _ip(IP), _port(PORT) {}
    ~Client() {}

    std::string send_to_server( const std::string& message )
    {
        connectToServer();
        sendToSock( ( message + "\n" ) );
        std::string ret = readFromRecv();
        close(sock);
        return ret;
    }
};

int main()
{
    Client user( "127.0.0.1", 8000 );
    assert( user.send_to_server("12345") == "12345" );
    assert( user.send_to_server("123 321") == "444" );
    assert( user.send_to_server("111 222 333") == "666" );
    assert( user.send_to_server("150 150 25 25 1 1 1 2 1 1") == "357" );
    return 0;
}

