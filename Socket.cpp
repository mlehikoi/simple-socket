#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>


// class IpAddress
inline IpAddress::IpAddress(const char* pStrP)
{
    in_addr addr;
    assert(inet_aton(pStrP, &addr));
    ipM = addr.s_addr;
}

inline IpAddress IpAddress::fromString(const char* pStrP)
{
    return IpAddress{pStrP};
}

// class SocketAbs
inline SocketAbs::SocketAbs(int socketP)
  : socketM{socketP}
{
    std::cout << "SocketAbs: "<< "'" << (int)socketM << "'" << std::endl;
}

inline SocketAbs::SocketAbs(SocketAbs&& rOtherP)
  : socketM{rOtherP.socketM}
{
    rOtherP.socketM = -1;
}

inline SocketAbs::~SocketAbs()
{
    std::cout << "~SocketAbs: " << "'" << (int)socketM << "'" << std::endl;
    if (socketM >= 0)
    {
        ::close(socketM);
    }
}

inline int SocketAbs::getSocket() const
{
    return socketM;
}

// class UdpSocket
inline UdpSocket::UdpSocket()
  : SocketAbs{::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)},
    receiverM{}
{
}

inline UdpReceiver& UdpSocket::bind(const IpAddress& rIpAddressP, uint16_t portP)
{
    sockaddr_in si_me = {};
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(portP);
    si_me.sin_addr.s_addr = rIpAddressP.ipM;
    assert(::bind(getSocket(), reinterpret_cast<const sockaddr*>(&si_me), sizeof(si_me)) != -1);
    
    receiverM.reset(new UdpReceiver{*this});
    return *receiverM;
}

inline UdpReceiver& UdpSocket::getReceiver()
{
    return *receiverM;
}

inline int UdpSocket::sendTo(const IpAddress& rIpAddressP, uint16_t portP, const char* msgP, size_t lenP)
{
    auto s = getSocket();
    assert(s != -1);
    sockaddr_in si_other = {};
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(portP);     
    si_other.sin_addr.s_addr = rIpAddressP.ipM;
    return ::sendto(s, msgP, lenP, 0,
        reinterpret_cast<sockaddr*>(&si_other), sizeof(si_other));
}

inline int UdpSocket::sendTo(const IpAddress& rIpAddressP, uint16_t portP, const std::string& msgP)
{
    return sendTo(rIpAddressP, portP, msgP.data(), msgP.size());
}

// class UdpReceiver
inline UdpReceiver::UdpReceiver(SocketAbs& rSocketP)
  : rSocketM{rSocketP}
{
}

inline std::pair<int, std::string> UdpReceiver::receive()
{
    auto s = rSocketM.getSocket();
    
    static const int BUFLEN = 64 * 1024;
    char buf[BUFLEN] = {};
    // {
    //     const auto rv = recv(s, buf, 2, MSG_PEEK);
    //     std::cout << "Peak: " << rv << std::endl;
    //     int count = 0;
    //     ::ioctl(s, FIONREAD, &count);
    //     std::cout << "ioctl:" << count << std::endl;
    // }
    const auto rv = recv(s, buf, BUFLEN, 0);
    return std::make_pair(rv, std::string(buf, rv));
}

int UdpReceiver::getSocket() const
{
    return rSocketM.getSocket();
}

// class TcpSocket
inline TcpSocket::TcpSocket()
  : SocketAbs{::socket(AF_INET, SOCK_STREAM, 0)}
{
}
    
inline TcpListener TcpSocket::listen(const IpAddress& rIpAddressP, uint16_t portP)
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
    return TcpListener{*this};
}

inline TcpConnection TcpSocket::connect(const IpAddress& rIpAddressP, uint16_t portP)
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

// class TcpListener
inline TcpListener::TcpListener(const TcpSocket& rSocketP)
  : rSocketM{rSocketP}
{
}

inline TcpConnection TcpListener::accept()
{
    int s = rSocketM.getSocket();
    
    sockaddr_in clientAddr = {};
    socklen_t clilen = sizeof(clientAddr);
    std::cout << "accept" << std::endl;
    auto conn = ::accept(s, reinterpret_cast<sockaddr *>(&clientAddr), &clilen);
    std::cout << "accepted " << conn << std::endl;
    assert(conn != -1);
    return TcpConnection{conn};
}

// class TcpConnection
inline TcpConnection::TcpConnection(const TcpSocket& rSocketP)
  : pSocketM{&rSocketP},
    socketM{-1}
{
}

inline TcpConnection::TcpConnection(int socketP)
  : pSocketM{},
    socketM{socketP}
{
}

inline TcpConnection::~TcpConnection()
{
    std::cout << "**** Closing TcpConnection " << socketM << std::endl;
    if (socketM >= 0)
    {
        
        ::close(socketM);
    }
}

inline int TcpConnection::send(const std::string& rMsgP)
{
    auto s = getSocket();
    return ::send(s, rMsgP.data(), rMsgP.size(), 0);
}

inline std::pair<int, std::string> TcpConnection::receive()
{
    auto s = getSocket();

    std::stringstream ss;
    static const int BUFLEN = 2;
    char buf[BUFLEN] = {};
    auto bytes = recv(s, buf, BUFLEN, 0);
    if (bytes > 0)
    {
        ss.write(buf, bytes);
        int count = 0;
        ::ioctl(s, FIONREAD, &count);
        for (int i = 0; i < count; i += BUFLEN)
        {
            const int left = count - i;
            const int n = left > BUFLEN ? BUFLEN : left; 
            bytes = recv(s, buf, n, 0);
            ss.write(buf, bytes);
        }
    }
    return std::make_pair(bytes, ss.str());
}

inline int TcpConnection::getSocket() const
{
    return pSocketM ? pSocketM->getSocket() : socketM;
}

// class SocketFactory
inline UdpSocket SocketFactory::createUdpSocket()
{
    return UdpSocket{};
}

inline TcpSocket SocketFactory::createTcpSocket()
{
    return TcpSocket{};
}