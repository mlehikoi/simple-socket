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
        if (ownsM)
        {
            ::close(socketM);
        }
    }
protected:
    auto getSocket() { return socketM; }
};

class UdpSendSocket : public SocketAbs
{
public:
    UdpSendSocket()
      : SocketAbs{::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)}
    {
    }
    
    int sendTo(const IpAddress& rIpAddressP, uint16_t portP, const char* msgP, size_t lenP)
    {
        std::cout << "send " << lenP << " bytes"  << std::endl;
        auto s = getSocket();
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

class TcpSocket
{
};

class SocketFactory
{
public:
    static auto createUdpSendSocket()
    {
        return UdpSendSocket{};
    }
    
    static auto createUdpReceiveSocket(const IpAddress& rIpAddressP, uint16_t portP)
    {
        return UdpReceiveSocket{rIpAddressP, portP};
    }
};