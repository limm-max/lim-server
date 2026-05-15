// test_buffer.cc
//
// Buffer 单元测试: 覆盖 6 个核心场景
//   1. 基础读写
//   2. 三段式布局检验
//   3. retrieve 各种姿势
//   4. 扩容 (writable 不够)
//   5. prepend
//   6. readFd (用 pipe 模拟 fd)

#include "Buffer.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <string>

using namespace lim;

void test_basic()
{
    printf("\n=== Test 1: 基础读写 ===\n");

    Buffer buf;
    // 初始状态: prependable=8, readable=0, writable=1024
    assert(buf.readableBytes() == 0);
    assert(buf.writableBytes() == 1024);
    assert(buf.prependableBytes() == 8);
    printf("  初始: r=%zu w=%zu p=%zu ✅\n",
           buf.readableBytes(), buf.writableBytes(), buf.prependableBytes());

    // append 一个字符串
    const char* s = "Hello, Buffer!";
    buf.append(s, strlen(s));
    assert(buf.readableBytes() == strlen(s));
    assert(buf.writableBytes() == 1024 - strlen(s));
    printf("  写入 \"%s\" 后: r=%zu w=%zu ✅\n",
           s, buf.readableBytes(), buf.writableBytes());

    // peek 不消费
    assert(memcmp(buf.peek(), s, strlen(s)) == 0);
    assert(buf.readableBytes() == strlen(s));  // peek 后大小不变
    printf("  peek 不消费,r 仍然 %zu ✅\n", buf.readableBytes());

    // retrieveAllAsString
    std::string got = buf.retrieveAllAsString();
    assert(got == s);
    assert(buf.readableBytes() == 0);
    printf("  retrieveAllAsString = \"%s\" ✅\n", got.c_str());
}

void test_retrieve()
{
    printf("\n=== Test 2: retrieve 各种姿势 ===\n");

    Buffer buf;
    buf.append("abcdefghij", 10);
    printf("  初始: 写入 'abcdefghij' (10 字节)\n");

    // retrieve 3 字节
    buf.retrieve(3);
    assert(buf.readableBytes() == 7);
    assert(memcmp(buf.peek(), "defghij", 7) == 0);
    printf("  retrieve(3) 后: peek = \"%.7s\" ✅\n", buf.peek());

    // retrieveAsString 拿 4 字节
    std::string s = buf.retrieveAsString(4);
    assert(s == "defg");
    assert(buf.readableBytes() == 3);
    printf("  retrieveAsString(4) = \"%s\",剩 %zu 字节 ✅\n",
           s.c_str(), buf.readableBytes());

    // retrieveAll 后游标重置
    buf.retrieveAll();
    assert(buf.readableBytes() == 0);
    assert(buf.prependableBytes() == 8);  // 恢复到初始的 8
    printf("  retrieveAll 后 prependable 恢复 %zu ✅\n", buf.prependableBytes());
}

void test_grow()
{
    printf("\n=== Test 3: 扩容 ===\n");

    Buffer buf;
    // 一次性写超过初始的 1024
    std::string big(2000, 'X');
    buf.append(big);
    assert(buf.readableBytes() == 2000);
    printf("  写入 2000 字节,buffer 自动扩容 ✅\n");
    printf("  当前: r=%zu w=%zu p=%zu\n",
           buf.readableBytes(), buf.writableBytes(), buf.prependableBytes());

    // 验证数据正确
    std::string got = buf.retrieveAllAsString();
    assert(got == big);
    printf("  数据完整性 OK ✅\n");
}

void test_makespace_via_shifting()
{
    printf("\n=== Test 4: makeSpace 通过挪数据 (不扩容) ===\n");

    Buffer buf;
    // 写满 1000 字节,然后消费 800 字节
    std::string s(1000, 'A');
    buf.append(s);
    buf.retrieve(800);
    // 此时: prependable = 8+800 = 808, readable = 200, writable = 24
    printf("  写 1000 + retrieve 800 后:\n");
    printf("    r=%zu p=%zu w=%zu\n",
           buf.readableBytes(), buf.prependableBytes(), buf.writableBytes());

    // 再写 100 字节,writable 不够 (24 < 100),但 prependable+writable 够
    // 应该走 makeSpace 的"挪数据"路径,不扩容
    std::string more(100, 'B');
    buf.append(more);
    // 此时游标应该被重置,readable = 200+100 = 300
    assert(buf.readableBytes() == 300);
    printf("  再 append 100 字节后 (走挪数据路径):\n");
    printf("    r=%zu p=%zu w=%zu ✅\n",
           buf.readableBytes(), buf.prependableBytes(), buf.writableBytes());
    // prependable 应该恢复成 8
    assert(buf.prependableBytes() == 8);
}

void test_prepend()
{
    printf("\n=== Test 5: prepend 反向插入 ===\n");

    Buffer buf;
    // 写入"内容"
    const char* content = "HELLO";
    buf.append(content, 5);
    printf("  先 append \"HELLO\"\n");

    // 在前面 prepend 一个长度
    int32_t len = 5;
    buf.prepend(&len, sizeof len);
    printf("  prepend 4 字节长度 (int32_t = 5)\n");
    printf("  现在 readable = %zu 字节 (= 4 + 5)\n", buf.readableBytes());
    assert(buf.readableBytes() == 9);

    // 取出来验证
    int32_t gotLen = *reinterpret_cast<const int32_t*>(buf.peek());
    assert(gotLen == 5);
    buf.retrieve(4);
    std::string body = buf.retrieveAllAsString();
    assert(body == "HELLO");
    printf("  解析: len=%d, body=\"%s\" ✅\n", gotLen, body.c_str());
}

void test_readfd()
{
    printf("\n=== Test 6: readFd (用 pipe 模拟 fd) ===\n");

    // 用 pipe 制造一对 fd,写端模拟"客户端发数据",读端模拟"服务端接收"
    int fds[2];
    if (::pipe(fds) < 0)
    {
        printf("  pipe failed\n");
        return;
    }

    // 写端塞入数据 (一次塞很多,测大数据 readv 是否能装下)
    std::string sent(10000, 'Z');
    ssize_t wn = ::write(fds[1], sent.data(), sent.size());
    assert(wn == static_cast<ssize_t>(sent.size()));
    printf("  pipe 写端发出 %zu 字节\n", sent.size());

    // Buffer 从读端 readFd
    Buffer buf;
    int err = 0;
    ssize_t rn = buf.readFd(fds[0], &err);
    printf("  Buffer.readFd 读到 %zd 字节\n", rn);

    // 验证: 数据完整,且 buffer 内部已经 append 了所有数据
    assert(rn == static_cast<ssize_t>(sent.size()));
    assert(buf.readableBytes() == sent.size());
    std::string got = buf.retrieveAllAsString();
    assert(got == sent);
    printf("  数据完整性 OK ✅\n");

    ::close(fds[0]);
    ::close(fds[1]);
}

void test_findCRLF()
{
    printf("\n=== Test 7: findCRLF 找消息边界 ===\n");

    Buffer buf;
    buf.append("GET /index HTTP/1.1\r\nHost: localhost\r\n\r\n", 41);

    const char* crlf = buf.findCRLF();
    assert(crlf != nullptr);
    printf("  找到 CRLF,前面是: \"%.*s\"\n",
           static_cast<int>(crlf - buf.peek()), buf.peek());

    // 消费第一行 (含 \r\n)
    buf.retrieveUntil(crlf + 2);
    printf("  消费第一行后,剩余: \"%.*s\"\n",
           static_cast<int>(buf.readableBytes()), buf.peek());
}

int main()
{
    printf("=== Buffer 单元测试 ===\n");

    test_basic();
    test_retrieve();
    test_grow();
    test_makespace_via_shifting();
    test_prepend();
    test_readfd();
    test_findCRLF();

    printf("\n所有测试通过 🎉\n");
    return 0;
}