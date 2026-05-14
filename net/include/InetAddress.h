// InetAddress.h
//网络地址封装
#pragma once
 
#include <arpa/inet.h>      // sockaddr_in / htons / inet_pton
#include <netinet/in.h>
#include <string>

namespace lim
{
class InetAddress
{
public:
    //禁止隐式转换
    //Linux地址结构是C风格，所以需要C++包装
    /*
    通用socket地址——struct sockaddr
    IPv4
        struct sockaddr_in{
            sa_family_t sin_family;     
            u_int16_t sin_port;         
            struct in_addr sin_addr;
        }
        struct in_addr{
            u_int32_t s_addr;       
        }
    */
    //两种构造：端口+ip，或者直接传入sockaddr_in通信地址结构
    explicit InetAddress(uint16_t port=0,std::string ip="0.0.0.0");
    explicit InetAddress(const sockaddr_in& addr): addr_(addr){}

    //读取方法
    std::string toIp() const;   // ip字符串："192.168.1.5"
    std::string toIpPort() const;   // ip端口字符串"192.168.1.5:8080"
    uint16_t toPort() const;    // // 8080 (主机字节序)

    // ─── 给系统调用 ───
    //因为bind等函数要求传进去的是sockaddr*通用指针，所以需要进行类型转换
    const sockaddr* getSockAddr() const{
        return reinterpret_cast<const sockaddr*>(&addr_);
    }

    //accept时候将系统提供的对端地址写进来
    void setSockAddr(const sockaddr_in& addr){addr_=addr;}

    private:
        sockaddr_in addr_;
};
}//namespace lim