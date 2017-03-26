#include "Socket.hpp"

#include <iostream>
#include <thread>
#include <cassert>

#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

void die(const char * format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsprintf(buffer,format, args);
    cerr << buffer << endl;
    va_end(args);
    abort();
}

void send()
{
    std::string d;
    d.append(9216, '*');
    auto s = SocketFactory::createUdpSendSocket();
    for (int i = 0; i < 2; ++i)
    {
        s.sendTo("127.0.0.1", 7000, d);//std::string("Hello world ") + std::to_string(i));
    }
    s.sendTo("127.0.0.1", 7000, "");
}

void receive()
{
    auto s = SocketFactory::createUdpReceiveSocket("127.0.0.1", 7000);
    for (;;)
    {
        auto ret = s.receive();
        if (ret.first == -1 || ret.second.empty()) break;
        cout << ret.second.size() << endl;
    }
}

int main()
{
    std::thread sender([]{
        send();
    });
    receive();
    sender.join();
}