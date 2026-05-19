// HttpResponse.cc
#include "HttpResponse.h"
#include "Buffer.h"

#include <stdio.h>     // snprintf

namespace lim
{

    void HttpResponse::appendToBuffer(Buffer* output)   const
    {
        // ── 1. 状态行:"HTTP/1.1 200 OK\r\n" ──
        char buf[32];
        snprintf(buf,sizeof buf,"%s %d ",httpVersion_.c_str(),statusCode_);
        output->append(buf);
        output->append(statusMessage_);
        output->append("\r\n");
        // ── 2. Connection 头(按 closeConnection_ 自动填)──
        //    注意:不直接塞进 headers_ map,而是放在这里,是为了让用户不用关心这事
        if(closeConnection_)
        {
            output->append("Connection:close\r\n");
        }
        else{
            output->append("Connection:Keep-Alive\r\n");
        }

        // ── 3. 其他 header ──
        for(const auto&[key,value]:headers_)
        {
            output->append(key);
            output->append(":");
            output->append(value);
            output->append("\r\n");
        }

        // ── 4. 空行分隔 + body ──
        output->append("\r\n");
        output->append(body_);
    }
}//namespace lim