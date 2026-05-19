// test_httpcontext.cc —— 修订版
//
// 覆盖:
//   1. 完整 GET 一次到达
//   2. 带 query string,验证 KV 拆分
//   3. 半包(分两次到达)
//   4. reset 后能解析第二个请求(Keep-Alive)
//   5. HttpRequest 的 body 接口(单独单测,Day 12 状态机不解析)

#include "HttpContext.h"
#include "HttpRequest.h"
#include "Buffer.h"

#include <iostream>
#include <string>
#include <assert.h>

using namespace lim;

// ─────────────────────────────────────────
void testCase1_simpleGet()
{
    std::cout << "\n=== Test 1: 完整 GET 请求一次到达 ===\n";
    HttpContext context;
    Buffer input;
    std::string req =
        "GET /index.html HTTP/1.1\r\n"
        "Host: 127.0.0.1:8080\r\n"
        "User-Agent: curl/7.81\r\n"
        "\r\n";
    input.append(req);

    bool ok = context.parseRequest(&input, Timestamp::now());
    assert(ok);
    assert(context.gotAll());

    const HttpRequest& r = context.request();
    std::cout << "method  = " << r.methodString() << "\n";
    std::cout << "path    = " << r.path() << "\n";
    std::cout << "Host    = " << r.getHeader("Host") << "\n";
    std::cout << "UA      = " << r.getHeader("User-Agent") << "\n";

    assert(r.method() == HttpRequest::kGet);
    assert(r.path() == "/index.html");
    assert(r.queryParameters().empty());           // ← 没有 query
    assert(r.getHeader("Host") == "127.0.0.1:8080");
    assert(r.getVersion() == HttpRequest::kHttp11);
    std::cout << "✅ PASS\n";
}

// ─────────────────────────────────────────
void testCase2_withQuery()
{
    std::cout << "\n=== Test 2: 带 query string,验证 KV 拆分 ===\n";
    HttpContext context;
    Buffer input;
    input.append(std::string(
        "GET /search?q=hello&n=10&lang=zh HTTP/1.1\r\n"
        "Host: x\r\n"
        "\r\n"));

    assert(context.parseRequest(&input, Timestamp::now()));
    assert(context.gotAll());

    const HttpRequest& r = context.request();
    assert(r.path() == "/search");

    // 验证 query 被正确拆成 KV
    assert(r.getQueryParameter("q") == "hello");
    assert(r.getQueryParameter("n") == "10");
    assert(r.getQueryParameter("lang") == "zh");
    assert(r.getQueryParameter("notexist") == "");   // 不存在返回空

    std::cout << "path  = " << r.path() << "\n";
    std::cout << "q     = " << r.getQueryParameter("q") << "\n";
    std::cout << "n     = " << r.getQueryParameter("n") << "\n";
    std::cout << "lang  = " << r.getQueryParameter("lang") << "\n";
    std::cout << "✅ PASS\n";
}

// ─────────────────────────────────────────
void testCase3_halfPacket()
{
    std::cout << "\n=== Test 3: 半包(分两次到达)===\n";
    HttpContext context;
    Buffer input;

    // 第一次只到了一半请求行
    input.append(std::string("GET /hello HTTP/"));
    bool ok = context.parseRequest(&input, Timestamp::now());
    assert(ok);
    assert(!context.gotAll());
    std::cout << "第一次半包: gotAll = false ✓\n";

    // 第二次剩下的来了
    input.append(std::string("1.1\r\nHost: x\r\n\r\n"));
    ok = context.parseRequest(&input, Timestamp::now());
    assert(ok);
    assert(context.gotAll());
    assert(context.request().path() == "/hello");
    std::cout << "第二次拼上: gotAll = true, path = " << context.request().path() << "\n";
    std::cout << "✅ PASS\n";
}

// ─────────────────────────────────────────
void testCase4_reset()
{
    std::cout << "\n=== Test 4: reset 后能解析第二个请求(Keep-Alive)===\n";
    HttpContext context;
    Buffer input;

    input.append(std::string("GET /a HTTP/1.1\r\nHost: x\r\n\r\n"));
    context.parseRequest(&input, Timestamp::now());
    assert(context.gotAll());
    assert(context.request().path() == "/a");
    std::cout << "第一个请求: path = " << context.request().path() << "\n";

    context.reset();
    assert(!context.gotAll());
    assert(context.request().method() == HttpRequest::kInvalid);   // ← 验证彻底清空

    input.append(std::string("POST /b HTTP/1.1\r\nHost: y\r\n\r\n"));
    context.parseRequest(&input, Timestamp::now());
    assert(context.gotAll());
    assert(context.request().method() == HttpRequest::kPost);
    assert(context.request().path() == "/b");
    std::cout << "第二个请求: method = " << context.request().methodString()
              << ", path = " << context.request().path() << "\n";
    std::cout << "✅ PASS\n";
}

// ─────────────────────────────────────────
// Test 5 是「数据结构单测」,不走 HttpContext,直接测 HttpRequest 的 body 接口
// 这样确保 Day 12 留给 Day 13 用的字段接口齐全
void testCase5_bodyInterface()
{
    std::cout << "\n=== Test 5: HttpRequest body 字段接口 ===\n";
    HttpRequest req;

    // 初始状态
    assert(req.getBody() == "");
    assert(req.contentLength() == 0);

    // 用 string 设
    std::string body = "{\"username\":\"lim\",\"password\":\"123\"}";
    req.setBody(body);
    req.setContentLength(body.size());

    assert(req.getBody() == body);
    assert(req.contentLength() == body.size());
    std::cout << "body          = " << req.getBody() << "\n";
    std::cout << "contentLength = " << req.contentLength() << "\n";

    // 用 [start, end) 设(模拟 HttpContext 从 buffer 里切)
    HttpRequest req2;
    const char* s = body.data();
    req2.setBody(s, s + body.size());
    assert(req2.getBody() == body);
    std::cout << "(char*, char*) 版本也 OK\n";

    // 验证 swap 把 body 也带走了
    HttpRequest req3;
    req3.swap(req);
    assert(req3.getBody() == body);
    assert(req.getBody() == "");        // 被换空了
    std::cout << "swap 后 body 正确转移 ✓\n";
    std::cout << "✅ PASS\n";
}

// ─────────────────────────────────────────
int main()
{
    testCase1_simpleGet();
    testCase2_withQuery();
    testCase3_halfPacket();
    testCase4_reset();
    testCase5_bodyInterface();
    std::cout << "\n🎉 All tests passed!\n";
    return 0;
}