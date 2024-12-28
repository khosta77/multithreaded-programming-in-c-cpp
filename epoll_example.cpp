#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <csignal>
#include <sys/eventfd.h>

#include <iostream>

#include <thread>

#include <map>
#include <mutex>
    
#define MAX_EVENTS 10

pthread_t MainThreadId = 0;
int evfd = 0;

std::map<int, std::string> clients_content;
std::mutex mutex; // Example for one client

// iovec

void signal_handler(int signal)
{
    std::cout << "signal_handler " << signal << " " << evfd << std::endl;
   
    uint64_t val = 1;
    int writeres = write(evfd, &val, sizeof(val));
    std::cout << "signal_handler no write: " << writeres << " " << strerror(errno) << " " << errno << std::endl;
        
}

int EventLoop(int epollfd, int listen_sock)
{
    struct epoll_event ev, events[MAX_EVENTS];
    int nfds, conn_sock = 0;

    for (;;) {
        std::cout << "Loop start " << std::this_thread::get_id() << std::endl;

        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror(">> epoll_wait");
            //exit(EXIT_FAILURE);
            return 0;
        }
    
	for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == evfd) {
                std::cout << ">> Event fd! " << std::this_thread::get_id() << std::endl;
                return 0;
	    }

            if (events[n].data.fd == listen_sock) {
        	std::cout << "Loop accept " << std::this_thread::get_id() << std::endl;
              
                struct sockaddr addr;
                socklen_t addrlen = sizeof(addr);
                conn_sock = accept(listen_sock, (struct sockaddr *) &addr, &addrlen); // LT ; ET
                std::cout << "Accepted: " << conn_sock << std::endl;
                if (conn_sock == -1) {
                    perror("accept");
                    continue; //exit(EXIT_FAILURE);
                }

                fcntl(conn_sock, F_SETFL, O_NONBLOCK);
                ev.events = EPOLLIN | EPOLLONESHOT;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else {
        	std::cout << "Loop Read: " << events[n].data.fd << " " << events[n].events << " " << std::this_thread::get_id() << std::endl;
		
		char buff[1024] = "";
	
		std::atomic_thread_fence(std::memory_order_aquire);

                //std::unique_lock<std::mutex> l(mutex);
		int bytescnt = read(events[n].data.fd, buff, 1);
                     //std::cout << buff << " " << bytescnt << std::endl;
                     clients_content[events[n].data.fd] += buff;
                     
		     if (bytescnt == 0) {
                   	 std::cout << clients_content[events[n].data.fd] << " " << std::this_thread::get_id() << std::endl;
   		         epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, nullptr);
                         close(events[n].data.fd);
			 break;
		     }
		     ev.events = EPOLLIN | EPOLLONESHOT;
                     ev.data.fd = events[n].data.fd;
		     epoll_ctl(epollfd, EPOLL_CTL_MOD, events[n].data.fd, &ev);
		
		std::atomic_thread_fence(std::memory_order_release);

            }
        }
    }

    return 0;
  }

int MainFunc()
{
    struct epoll_event ev, events[MAX_EVENTS];
    
    MainThreadId = pthread_self();
    evfd = eventfd(0, 0);

    int listen_sock, nfds, epollfd;
    
    listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    int reuse = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse));

    struct sockaddr_in addr_serv;
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port = htons(3425);
    addr_serv.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0 <-> 127.0.0.1 | 192.168.0.111
    
    if(bind(listen_sock, (struct sockaddr *)&addr_serv, sizeof(addr_serv)) < 0)
    {
        perror("bind");
        exit(2);
    }
    listen(listen_sock, 2);
    
    fcntl(listen_sock, F_SETFL, O_NONBLOCK);    
    //struct sockaddr addr;
    //socklen_t addrlen = sizeof(addr);
    //int acc_res = accept(listen_sock, (struct sockaddr *) &addr, &addrlen);
    //std::cout << ">> " << acc_res << " " << strerror(errno) << std::endl;
    //return 0;
    
    /* Code to set up listening socket, 'listen_sock',
       (socket(), bind(), listen()) omitted. */
    
    epollfd = epoll_create1(0);
    if (epollfd == -1) {
        perror("epoll_create1");
        exit(EXIT_FAILURE);
    }
    
    ev.events = EPOLLIN | EPOLLEXCLUSIVE;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    ev.events = EPOLLIN;
    ev.data.fd = evfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, evfd, &ev) == -1) {
        perror("epoll_ctl: evfd");
        exit(EXIT_FAILURE);
    }

    std::thread thr1( EventLoop, epollfd, listen_sock);
    std::thread thr2( EventLoop, epollfd, listen_sock);

    thr1.join();
    thr2.join();

    return 0;
}



int main()
{
    std::thread main_thread(MainFunc);
    std::cout << "> Main! Id = " << main_thread.native_handle() << std::endl;
    std::signal(SIGINT, signal_handler);
    sleep(5);
    
    std::cout << "Id from thread: " << MainThreadId << std::endl;

    //pthread_kill(MainThreadId, SIGINT);
    main_thread.join();

    std::cout << "Some message!" << std::endl;
}

