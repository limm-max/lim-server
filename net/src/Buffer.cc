// Buffer.cc
#include "Buffer.h"
 
#include <sys/uio.h>     // readv, iovec
#include <unistd.h>      // read, write
#include <errno.h>
 
namespace lim
{

    // "\r\n" 常量 (用 const char[] 而非 const char*,findCRLF 里 std::search 需要)
    const char Buffer::kCRLF[] = "\r\n";

    // readFd:
    //vec[0]: Buffer 现有 writable 区 (~1KB)
    //vec[1]: 临时栈上 64KB extrabuf。
    //当数据大于buffer的容量，溢出部分存进函数栈extrabuf，然后再带哦用Buffer方法append回Buffer
    //savedErrno,输出型参数
    ssize_t Buffer::readFd(int fd,int *savedErrno)
    {
        //额外的buf直接设置在函数栈上，函数返回就释放
        char extrabuf[65536];
        struct iovec vec[2];    //readv要传入的分散内存块结构
        const size_t writable=writableBytes();
        vec[0].iov_base=beginWrite();
        vec[0].iov_len=writable;
        vec[1].iov_base=extrabuf;
        vec[1].iov_len=sizeof extrabuf;

        // 小优化: 如果 Buffer 的 writable 已经 >= 64KB,就不需要 extrabuf 兜底了
        // 用一个 iovec 即可,避免 readv 把 64KB 数据塞到根本用不上的 extrabuf
        const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;

        const ssize_t n=::readv(fd,vec,iovcnt);

        if(n<0)
        {
            *savedErrno=errno;
        }
        else if(static_cast<size_t>(n)<=writable)
        {
            // 数据没超过 vec[0] 的容量,只动了 Buffer 自己
            writerIndex_ += n;
        }
        else
        {
            // 数据超过 vec[0],溢出部分在 extrabuf 里
            //要先修改writeIndex_再append在extrabuf，可能设计扩容，append函数自己会处理
            writerIndex_=buffer_.size();
            append(extrabuf,n-writable);    //void append(const char* data,size_t len)函数
        }
        return n;

    }


    // writeFd - 把 Buffer 可读区的数据写到 fd
    // 实际项目里 TcpConnection 通常自己直接 ::write,不会走这个方法
    // 这里提供只是为了对称
    ssize_t Buffer::writeFd(int fd, int* savedErrno)
    {
        ssize_t n = ::write(fd, peek(), readableBytes());
        if (n < 0)
        {
            *savedErrno = errno;
        }
        return n;
    }
}//namespace lim