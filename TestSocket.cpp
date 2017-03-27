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
    d.append(20/*9216*/, '*');
    auto s = SocketFactory::createUdpSocket();
    for (int i = 0; i < 20; ++i)
    {
        cout << "send ret: " << s.sendTo("127.0.0.1", 7000, d) << endl;//std::string("Hello world ") + std::to_string(i));
    }
    s.sendTo("127.0.0.1", 7000, "");
}

void receive()
{
    auto s = SocketFactory::createUdpSocket();
    auto r = s.bind("0.0.0.0", 7000);
    for (;;)
    {
        auto ret = r.receive();
        if (ret.first == -1 || ret.second.empty()) break;
        cout << ret.second.size() << endl;
    }
}

void receiveTcp()
{
    auto s = SocketFactory::createTcpSocket();
    auto l = s.listen("127.0.0.1", 7001);
    
    // Accept two connections
    auto c1 = l.accept();
    auto c2 = l.accept();
    
    for (;;)
    {
        auto ret1 = c1.receive();
        if (ret1.first > 0)
        {
            cout << "1 " << ret1.first << " " << ret1.second << endl;
            c1.send("ack");
        }
        
        auto ret2 = c2.receive();
        if (ret2.first > 0)
        {
            cout << "2 " << ret2.first << " " << ret2.second << endl;
            c2.send("ack");
        }

        if (ret1.first <= 0 && ret2.first <= 0) break;
    }
    cout << "Done" << endl;
}

void sendTcp(int indexP)
{
    cout << "sendTcp" << endl;
    auto s = SocketFactory::createTcpSocket();
    auto conn = s.connect("127.0.0.1", 7001);
    //cout << "Conn " << ret << endl;
    
    for (int i = 0; i < 10; ++i)
    {
        std::string str = "Message " + std::to_string(i) + " from " + std::to_string(indexP);
        conn.send(str);
        
        auto resp = conn.receive();
        //cout << "Response to " << indexP << " " << resp.second << endl;
    }
    cout << "Closing..." << endl;
}

void testCreate()
{
    UdpSocket s;
}

void sendUdp(int indexP, int portP, int timeoutP)
{
    UdpSocket s;
    for (int i = 0; i < 5; ++i)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(timeoutP));
        s.sendTo("127.0.0.1", portP, "msg " + std::to_string(indexP));
    }
    s.sendTo("127.0.0.1", portP, "");
}

void testPoll()
{
    std::thread t1([]{ sendUdp(1, 7002, 100); });
    std::thread t2([]{ sendUdp(2, 7003, 300); });
    
    UdpSocket s1;
    auto rec1 = s1.bind("0.0.0.0", 7002);
    UdpSocket s2;
    auto rec2 = s2.bind("0.0.0.0", 7003);
    //start = true;
    
    std::vector<UdpReceiver*> sockets{ &rec1, &rec2 };
    std::this_thread::sleep_for(333ms);
    while (!sockets.empty())
    {
        auto pollResults = sspoll(sockets.begin(), sockets.end(), 5000);
        cout << "pollResults: " << pollResults.size() << endl;
        if (pollResults.empty()) continue;
        for (auto pr : pollResults)
        {
            auto ret = pr->receive();
            if (ret.first <= 0)
            {
                sockets.erase(std::remove(sockets.begin(), sockets.end(), pr), sockets.end());
            }
            else
            {
                cout << ret.second << endl;
            }
        }
        //std::this_thread::sleep_for(100ms);
        //auto ret = rec.receive();
        //if (ret.first <= 0) break;
        //cout << ret.second << endl;
    }
    
    t1.join();
    t2.join();
}

int main()
{
    //testCreate();
    testPoll();
    return 0;
    std::thread sender([]{ send();});
    receive();
    sender.join();
    
    cout << "Starting TCP test" << endl;
    
    std::thread receiver([] { receiveTcp(); });
    std::this_thread::sleep_for(100ms);
    
    std::thread sender1([]{ sendTcp(1); });
    //
    std::thread sender2([]{ sendTcp(2); });
    
    
    
    
    sender1.join();
    sender2.join();
    receiver.join();
}
