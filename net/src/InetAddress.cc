#include "InetAddress.h"

#include<string.h>
#include<stdio.h>

namespace lim
{

    InetAddress::InetAddress(uint16_t port,std::string ip)
    {
        ::bzero(&addr_,sizeof addr_);   //addr_清零
        addr_.sin_family=AF_INET;
        addr_.sin_port=::htons(port);   //小端（主机端）到大端（网络端）转换
        ::inet_pton(AF_INET,ip.c_str(),&addr_.sin_addr);    //(presentation to network)两种ip协议通用的字符串ip转大端整数函数
    }

    std::string InetAddress::toIp() const
    {
        char buf[64]={0};
        ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);
        return buf;
    }

    std::string InetAddress::toIpPort() const
    {
        char buf[64]={0};
        ::inet_ntop(AF_INET,&addr_.sin_addr,buf,sizeof buf);
        size_t end=::strlen(buf);
        uint16_t port=::ntohs(addr_.sin_port);
        snprintf(buf+end,sizeof buf-end,":%u",port);    //buf+end,指针偏移
        return buf;   
    }

    uint16_t InetAddress::toPort() const
    {
        return ::ntohs(addr_.sin_port);
    }

}//namespace lim