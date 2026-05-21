// HttpContext.cc
#include "HttpContext.h"
#include "Buffer.h"

#include <algorithm>

namespace lim
{

// ─────────────────────────────────────────
// processRequestLine
//   解析 "GET /hello?name=lim HTTP/1.1"
//   切成 method / path / query / version 填进 request_
// ─────────────────────────────────────────
bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    // 找第一个空格,前面是 method
    const char* space = std::find(start, end, ' ');
    if (space != end && request_.setMethod(start, space))
    {
        start = space + 1;
        // 再找下一个空格,中间是 URL
        space = std::find(start, end, ' ');
        if (space != end)
        {
            // URL 里可能有 '?',要切出 path 和 query
            const char* question = std::find(start, space, '?');
            if (question != space)
            {
                request_.setPath(start, question);
                request_.setQueryParameters(question + 1, space);
            }
            else
            {
                request_.setPath(start, space);
                // 没有 query,query_ 保持空
            }
            // 最后是版本,只支持 HTTP/1.0 或 HTTP/1.1
            start = space + 1;
            succeed = (end - start == 8) && std::equal(start, end - 1, "HTTP/1.");
            if (succeed)
            {
                if (*(end - 1) == '1')      request_.setVersion(HttpRequest::kHttp11);
                else if (*(end - 1) == '0') request_.setVersion(HttpRequest::kHttp10);
                else                        succeed = false;
            }
        }
    }
    return succeed;
}

// ─────────────────────────────────────────
// parseRequest:主状态机驱动
//
// 核心思路:每次循环找一个 \r\n,处理一行,推进状态
//          如果找不到 \r\n,说明这行还没来全,break 等下次
// ─────────────────────────────────────────
bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
    bool ok = true;       // 解析过程是否成功
    bool hasMore = true;  // 是否还要继续解析(用来跳出循环)

    while (hasMore)
    {
        if (state_ == kExpectRequestLine)
        {
            // 在 Buffer 可读区找 \r\n
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                // peek() 是 Buffer 可读区起点
                ok = processRequestLine(buf->peek(), crlf);
                if (ok)
                {
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2);   // 消耗掉这一行 + \r\n
                    state_ = kExpectHeaders;
                }
                else
                {
                    hasMore = false;   // 请求行格式错,直接退
                }
            }
            else
            {
                hasMore = false;   // 请求行没来全,等下次
            }
        }
        else if (state_ == kExpectHeaders)
        {
            const char* crlf = buf->findCRLF();
            if (crlf)
            {
                // 在这一行里找 ':',形如 "Host: 127.0.0.1"
                const char* colon = std::find(buf->peek(), crlf, ':');
                if (colon != crlf)
                {
                    // 找到了 ':',是普通 header 行
                    request_.addHeader(buf->peek(), colon, crlf);
                }
                else
                {
                    // 空行,header 结束
                    //判断是否有body需要解析
                    if (request_.method() == HttpRequest::kPost ||
                        request_.method() == HttpRequest::kPut)
                    {
                        std::string len = request_.getHeader("Content-Length");
                        if (!len.empty())
                        {
                            request_.setContentLength(std::stoull(len));
                            if (request_.contentLength() > 0)
                            {
                                state_ = kExpectBody;   // 还要读 body
                            }
                            else
                            {
                                state_ = kGotAll;
                                hasMore = false;
                            }
                        }
                        else
                        {
                            // POST/PUT 没 Content-Length → 当作没 body,直接完成
                            state_ = kGotAll;
                            hasMore = false;
                        }
                    }
                    else
                    {
                        // GET/HEAD/DELETE 无 body,直接完成
                        state_ = kGotAll;
                        hasMore = false;
                    }
                }
                buf->retrieveUntil(crlf + 2);
            }
            else
            {
                hasMore = false;   // header 行没来全
            }
        }
        else if (state_ == kExpectBody)
        {
            
            if (buf->readableBytes() < request_.contentLength()) {
                hasMore = false;
                return true;
            }
            std::string body(buf->peek(), buf->peek() + request_.contentLength());
            request_.setBody(body);
            buf->retrieve(request_.contentLength());
            state_ = kGotAll;
            hasMore = false;
        }
    }

    return ok;
}

} // namespace lim