// SocketsOps.cc
#include "SocketsOps.h"
#include "Logger.h"
 
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
 
namespace lim
{
namespace sockets
{

int createNonblockingOrDie(sa_family_t family)
{
    int sockfd = ::socket(family,
                          SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,   //流服务+非阻塞+fork/exec 时自动 close,防止子进程泄漏
                          IPPROTO_TCP); //显性标明为TCP
    if(sockfd<0)
    {
        LOG_FATAL << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}

void close(int sockfd)
{
    if (::close(sockfd) < 0)
    {
        LOG_ERROR << "sockets::close errno=" << errno;
    }
}

void shutdownWrite(int sockfd)
{
    if (::shutdown(sockfd, SHUT_WR) < 0)
    {
        LOG_ERROR << "sockets::shutdownWrite errno=" << errno;
    }
}

}//namespace sockets
}//namespace lim