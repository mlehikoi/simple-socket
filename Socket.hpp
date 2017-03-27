#pragma once

// #include <cassert>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// #include <iostream>
// #include <string>
//
// #include <arpa/inet.h>
// #include <netinet/in.h>
// #include <sys/socket.h>
// #include <unistd.h>

class IpAddress
{
public:
    uint32_t ipM;

    IpAddress(const char* pStrP);
    IpAddress fromString(const char* pStrP);
};

class TcpSocket;
class UdpSocket;

class SocketAbs
{
    bool ownsM;
    int socketM;
public:
    SocketAbs(int socketP);
    SocketAbs(SocketAbs&& rOtherP);
    ~SocketAbs();
    int getSocket() const;
};

class UdpReceiver;
class UdpSocket : public SocketAbs
{
    std::unique_ptr<UdpReceiver> receiverM;
public:
    UdpSocket();
    UdpReceiver& bind(const IpAddress& rIpAddressP, uint16_t portP);
    UdpReceiver& getReceiver();
    int sendTo(const IpAddress& rIpAddressP, uint16_t portP, const char* msgP, size_t lenP);
    int sendTo(const IpAddress& rIpAddressP, uint16_t portP, const std::string& msgP);
};

class UdpReceiver
{
    SocketAbs& rSocketM;
public:
    explicit UdpReceiver(SocketAbs& rSocketP);
    std::pair<int, std::string> receive();
    int getSocket() const;
};

class TcpListener;
class TcpConnection;
class TcpSocket : public SocketAbs
{
public:
    TcpSocket();
    TcpListener listen(const IpAddress& rIpAddressP, uint16_t portP);
    TcpConnection connect(const IpAddress& rIpAddressP, uint16_t portP);
};

class TcpListener
{
    const TcpSocket& rSocketM;
public:
    TcpListener(const TcpSocket& rSocketP);
    TcpConnection accept();
};

class TcpConnection
{
    const TcpSocket* pSocketM;
    int socketM;
public:
    explicit TcpConnection(const TcpSocket& rSocketP);
    explicit TcpConnection(int socketP);
    ~TcpConnection();
    int send(const std::string& rMsgP);
    std::pair<int, std::string> receive();

private:
    int getSocket() const;
};

class SocketFactory
{
public:
    static UdpSocket createUdpSocket();
    static TcpSocket createTcpSocket();
};

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <iostream>

template<typename Iterator>
auto sspoll(Iterator beginP, Iterator endP, int timeoutP)
{
    using ValueT = typename std::iterator_traits<Iterator>::value_type;
    std::vector<ValueT> sockets{beginP, endP};
    std::vector<pollfd> fds;
    std::vector<ValueT> ret;
    for (auto s : sockets)
    {
        fds.push_back({s->getSocket(), POLLIN});
    }
    const int r = poll(fds.data(), fds.size(), timeoutP);
    if (r < 0)
    {
    }
    else
    {
        int i = 0;
        for (auto& fd : fds)
        {
            if (fd.revents & POLLIN)
            {
                ret.push_back(sockets[i]);
            }
            ++i;
        }
    }
    return ret;
}

#include "Socket.cpp"
