//应用层缓冲区
/*
从内核read回来的数据先统一放在BUffer的buffer中，由BUffer处理
Buffer 解决 4 个问题:
  1. TCP 流式数据 → 业务消息边界    (peek / findCRLF / retrieveUntil)
  2. send 一次写不完 → 暂存等可写    (append + handleWrite 配合)
  3. read 系统调用怎么读最高效       (readFd 用 readv + 栈上 64KB 兜底)
  4. 协议头部反向插入                 (prepend, 默认预留 8 字节)
─── 三段式布局 ──────────────────────────────────────────────────────────────

  |<- prependable ->|<-- readable -->|<-- writable -->|
  +-----------------+----------------+----------------+
  |     8 字节      |   实际数据       |  剩余空间       |
  +-----------------+----------------+----------------+
  ↑                 ↑                ↑                ↑
  0           readerIndex_     writerIndex_      buffer_.size()

  prependable: 给业务"反向写"协议头部 (默认预留 8 字节)
  readable:    已 append 但未 retrieve 的数据 (业务真正关心的)
  writable:    可以继续 append 的剩余空间 (不够时自动扩容)
  */
 #pragma once

 #include<algorithm>
 #include<string>
 #include<vector>
 #include<stddef.h>
 #include<string.h> //memchr

 namespace lim
 {

class Buffer
{
public:
    static const size_t kCheapPrepend=8;    //预留的长度头
    static const size_t kInitialSize=1024;  //初始可写长度，大消息readv进行 scatter read + 扩容机制


    explicit Buffer(size_t initialSize=kInitialSize)
        :buffer_(kCheapPrepend+initialSize)
        ,readerIndex_(kCheapPrepend)
        ,writerIndex_(kCheapPrepend)
        {

        }

        
    //——————状态查询—————————————(返回的是buffer_d的索引)
    //随着读过的信息越来越多，prependableBytes越来越大
    //* | kCheapPrepend |xxx| reader           | writer |                     // xxx标示reader中已读的部分
    //                      ⬆readerIndex_     ⬆writerIndex_
    size_t readableBytes() const {return writerIndex_-readerIndex_;}
    size_t writableBytes() const {return buffer_.size()-writerIndex_;}
    size_t prependableBytes() const {return readerIndex_;}


    //返回真正的数据指针,对应其实就是beginread（）
    const char* peek() const {return begin()+readerIndex_;}


    // 在可读区找 "\r\n"(回车换行),返回指针,找不到返回 nullptr
    // 业务用于按行切分消息 (HTTP / Redis / 文本协议)
    const char* findCRLF() const
    {
        const char* crlf=std::search(peek(),beginWrite(),kCRLF,kCRLF+2); //查找刻度区域的kCRLF子串
        return crlf=beginWrite()? nullptr:crlf; //指针指向beginWrite()说明没找到
    }

    // 找单个字符 (如 '\n')
    const char* findEOL() const
    {
        const void* eol=memchr(peek(), '\n', readableBytes());  //在指定内存区域查找字符
        return static_cast<const char*>(eol);
    }


    // ─── 消费数据 (retrieve 系列) ,只移动index，不真正拷贝/返回数据────────────────
    void retrieve(size_t len)
    {
        if(len<readableBytes())
        {
            readerIndex_+=len;
        }
        else{
            retrieveAll();
        }
    }

    void retrieveAll()
    {
        readerIndex_=kCheapPrepend;
        writerIndex_=kCheapPrepend;

    }

    //使用指针指定读的位置，(end 必须在 readable 区内)
    void retrieveUntil(const char* end)
    {
        retrieve(end-peek());
    }

    //————消费数据，将数据获取出来作为string
    std::string retrieveAsString(size_t len)
    {
        std::string result(peek(),len);
        retrieve(len);
        return result;
    }
    // 把可读区的数据全部取出来变成字符串,然后清空
    // 业务常用: std::string msg = buf->retrieveAllAsString();
    std::string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }


    // ─── 写入数据 (append 系列) ─────────────
    // 追加写: 空间不够会自动扩容
    void append(const char* data,size_t len)
    {
        ensureWritableBytes(len);   //确保有够位置append，扩容发生在这里
        std::copy(data,data+len,beginWrite());
        writerIndex_+=len;
    }

    void append(const std::string& str)
    {
        append(str.data(),str.size());
    }

    //append协议头的时候用到
    void append(const void* data,size_t len)
    {
        append(static_cast<const char*>(data), len);
    }

    void ensureWritableBytes(size_t len)
    {
        if(writableBytes()<len)
        {
            makeSpace(len);
        }
    }


    //取写指针
    char* beginWrite()  {return begin()+writerIndex_;}
    const char* beginWrite() const {return begin()+writerIndex_;}
    //配合外部的writerIndex_推进撤销
    void hasWritten(size_t len) {writerIndex_+=len;}
    void unWritten(size_t len)  {writerIndex_-=len;}


    // ─── 反向插入 (prepend) ───────────
    // 在 readable 区的前面插入 len 字节数据，用于写入长度头
    void prepend(const void* data,size_t len)
    {
        readerIndex_-=len;
        const char* d=static_cast<const char*>(data);
        std::copy(d,d+len,begin()+readerIndex_);
    }


    // ─── 与 fd 交互 (核心!) ,真正读回来和写出去的方法──────────
    ssize_t readFd(int fd, int* savedErrno);
    ssize_t writeFd(int fd, int* savedErrno);

private:

    //取指针
    char* begin()   {return &*buffer_.begin();} //&*，迭代器行为像指针，但是是类对象，所以需要先解引用后取地址
    const char* begin() const {return &*buffer_.begin();}


    //buffer_扩容:如果前面已读的区域和可写区域加起来都不够len，就真的需要扩容，否则重新规划
    //* | kCheapPrepend |xxx| reader           | writer |                     // xxx标示reader中已读的部分
    //                      ⬆readerIndex_     ⬆writerIndex_
    //* | kCheapPrepend | reader           ｜          len          |
    void makeSpace(size_t len)
    {
        if(writableBytes()+prependableBytes()<len+kCheapPrepend)
        {
            buffer_.resize(writerIndex_+len);   //也就是将可写区域扩展为len长
        }
        else
        {   
            //* | kCheapPrepend | reader           | writer |
            size_t readable=readableBytes();
            std::copy(begin()+readerIndex_,
                      begin()+writerIndex_,
                      begin()+kCheapPrepend);
            readerIndex_=kCheapPrepend;
            writerIndex_=readerIndex_+readable;
            
        }
    }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

    static const char kCRLF[];
};
 }//namespace lim