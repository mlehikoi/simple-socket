#pragma once

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>


class IpAddress
{
public:
    uint32_t ipM;

    IpAddress(const char* pStrP)
    {
        in_addr addr;
        assert(inet_aton(pStrP, &addr));
        ipM = addr.s_addr;
    }
    auto fromString(const char* pStrP)
    {
        return IpAddress{pStrP};
    }
};

class SocketAbs;
class Receiver
{
    SocketAbs& rSocketM;
public:
    explicit Receiver(SocketAbs& rSocketP)
      : rSocketM{rSocketP}
    {
        
    }
    std::pair<int, std::string> receive();
};

class SocketAbs
{
    bool ownsM;
    int socketM;
public:
    SocketAbs(int socketP)
      : ownsM{true},
        socketM{socketP}
    {        
    }

    SocketAbs(SocketAbs&& rOtherP)
      : ownsM{rOtherP.ownsM},
        socketM{rOtherP.socketM}
    {
        rOtherP.ownsM = false;
        rOtherP.socketM = -1;
    }
    
    ~SocketAbs()
    {
        std::cout << "~SocketAbs " << socketM << std::endl;
        if (socketM >= 0)
        {
            ::close(socketM);
        }
    }
    
    auto bind(const IpAddress& rIpAddressP, uint16_t portP)
    {
        sockaddr_in si_me = {};
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(portP);
        si_me.sin_addr.s_addr = rIpAddressP.ipM;
        assert(::bind(socketM, reinterpret_cast<const sockaddr*>(&si_me), sizeof(si_me)) != -1);
        
        return Receiver{*this};
    }
    
    int sendTo(const IpAddress& rIpAddressP, uint16_t portP, const char* msgP, size_t lenP)
    {
        auto s = socketM;
        assert(s != -1);
        sockaddr_in si_other = {};
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(portP);     
        si_other.sin_addr.s_addr = rIpAddressP.ipM;
        return ::sendto(s, msgP, lenP, 0,
            reinterpret_cast<sockaddr*>(&si_other), sizeof(si_other));
    }

    int sendTo(const IpAddress& rIpAddressP, uint16_t portP, const std::string& msgP)
    {
        return sendTo(rIpAddressP, portP, msgP.data(), msgP.size());
    }
    

    auto getSocket() const { return socketM; }
};

class UdpSocket : public SocketAbs
{
public:
    UdpSocket()
      : SocketAbs{::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)}
    {
    }
};

class UdpReceiveSocket : public SocketAbs
{
public:
    UdpReceiveSocket(const IpAddress& rIpAddressP, uint16_t portP)
      : SocketAbs{::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)}
    {
        sockaddr_in si_me = {};
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(portP);
        si_me.sin_addr.s_addr = rIpAddressP.ipM;
        assert(::bind(getSocket(), reinterpret_cast<const sockaddr*>(&si_me), sizeof(si_me)) != -1);
    }
    
    auto receive()
    {
        auto s = getSocket();
        
        static const int BUFLEN = 64 * 1024;
        char buf[BUFLEN] = {};
        {
            const auto rv = recv(s, buf, 2, MSG_PEEK);
            std::cout << "Peak: " << rv << std::endl;
        }
        const auto rv = recv(s, buf, BUFLEN, 0);
        return std::make_pair(rv, std::string(buf, rv));
    }
};

class TcpSocket;
class TcpConnection
{
    const TcpSocket* pSocketM;
    int socketM;
public:
    explicit TcpConnection(const TcpSocket& rSocketP);
    explicit TcpConnection(int socketP);
    ~TcpConnection()
    {
        std::cout << "**** Closing TcpConnection " << socketM << std::endl;
        if (socketM >= 0)
        {
            
            ::close(socketM);
        }
    }
    int send(const std::string& rMsgP);
    std::pair<int, std::string> receive();

private:
    int getSocket() const;
};

class TcpSocket : public SocketAbs
{
public:
    TcpSocket()
      : SocketAbs{::socket(AF_INET, SOCK_STREAM, 0)}
    {
    }
    
    int listen(const IpAddress& rIpAddressP, uint16_t portP)
    {
        int s = getSocket();
        std::cout << "s=" << s << std::endl;
        // Bind
        sockaddr_in si_me = {};
        si_me.sin_family = AF_INET;
        si_me.sin_port = htons(portP);
        si_me.sin_addr.s_addr = rIpAddressP.ipM;
        auto r = ::bind(s, reinterpret_cast<const sockaddr*>(&si_me), sizeof(si_me));
        std::cout << "r=" << r << std::endl;
        
        // listen
        auto ret = ::listen(s, 5);
        std::cout << "listen " << ret << std::endl;
        return 0;
    }
    
    auto accept()
    {
        int s = getSocket();
        
        sockaddr_in clientAddr = {};
        socklen_t clilen = sizeof(clientAddr);
        std::cout << "accept" << std::endl;
        auto conn = ::accept(s, reinterpret_cast<sockaddr *>(&clientAddr), &clilen);
        std::cout << "accepted " << conn << std::endl;
        assert(conn != -1);
        return TcpConnection{conn};
    }
    
    auto connect(const IpAddress& rIpAddressP, uint16_t portP)
    {
        int s = getSocket();

        sockaddr_in dest = {};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(portP);
        dest.sin_addr.s_addr = rIpAddressP.ipM;
        auto ret = ::connect(s, reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
        std::cout << "ret: " << ret << std::endl;
        //assert(ret == 0);
        return TcpConnection{*this};
    }
    
    int send(std::string& rMsgP)
    {
        auto s = getSocket();
        return ::send(s, rMsgP.data(), rMsgP.size(), 0);
    }

    std::pair<int, std::string> receive()
    {
        auto s = getSocket();

        static const int BUFLEN = 64 * 1024;
        char buf[BUFLEN] = {};
        {
            const auto rv = recv(s, buf, 2, MSG_PEEK);
            std::cout << "Peak: " << rv << std::endl;
        }
        const auto rv = recv(s, buf, BUFLEN, 0);
        return std::make_pair(rv, std::string(buf, rv));
    }
};

inline std::pair<int, std::string> Receiver::receive()
{
    auto s = rSocketM.getSocket();
    
    static const int BUFLEN = 64 * 1024;
    char buf[BUFLEN] = {};
    {
        const auto rv = recv(s, buf, 2, MSG_PEEK);
        std::cout << "Peak: " << rv << std::endl;
    }
    const auto rv = recv(s, buf, BUFLEN, 0);
    return std::make_pair(rv, std::string(buf, rv));
}

// TcpConnection
TcpConnection::TcpConnection(const TcpSocket& rSocketP)
  : pSocketM{&rSocketP},
    socketM{-1}
{
}

TcpConnection::TcpConnection(int socketP)
  : pSocketM{},
    socketM{socketP}
{
}

inline int TcpConnection::send(const std::string& rMsgP)
{
    auto s = getSocket();
    return ::send(s, rMsgP.data(), rMsgP.size(), 0);
}

inline std::pair<int, std::string> TcpConnection::receive()
{
    auto s = getSocket();

    static const int BUFLEN = 1024;
    char buf[BUFLEN] = {};
    const auto rv = recv(s, buf, BUFLEN, 0);
    return std::make_pair(rv, std::string(buf, rv));
}

inline int TcpConnection::getSocket() const
{
    return pSocketM ? pSocketM->getSocket() : socketM;
}

class SocketFactory
{
public:
    static auto createUdpSocket()
    {
        return UdpSocket{};
    }
    
    static auto createTcpSocket()
    {
        return TcpSocket{};
    }
};
