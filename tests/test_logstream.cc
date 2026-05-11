#include "LogStream.h"
#include <stdio.h>

using namespace lim;

void test_basic() {
    printf("\n=== Test 1: 基本类型 ===\n");

    LogStream stream;
    stream << "Hello " << 42 << ", pi=" << 3.14 << ", neg=" << -123;

    std::string result = stream.buffer().toString();
    printf("结果: [%s]\n", result.c_str());
    printf("长度: %d\n", stream.buffer().length());
}

void test_integer_types() {
    printf("\n=== Test 2: 各种整数类型 ===\n");

    LogStream stream;
    stream << "int=" << 2147483647 << " ";
    stream << "long=" << 9223372036854775807L << " ";
    stream << "unsigned=" << 4294967295U << " ";
    stream << "short=" << static_cast<short>(-32768) << " ";
    stream << "char=" << 'A';

    printf("结果: [%s]\n", stream.buffer().toString().c_str());
}

void test_string_pointer() {
    printf("\n=== Test 3: 字符串和指针 ===\n");

    LogStream stream;
    int x = 42;
    int* p = &x;
    const char* s = "hello";

    stream << "string=" << s << " ";
    stream << "std::string=" << std::string("world") << " ";
    stream << "pointer=" << p << " ";
    stream << "null=" << static_cast<const char*>(nullptr);

    printf("结果: [%s]\n", stream.buffer().toString().c_str());
}

void test_buffer_overflow() {
    printf("\n=== Test 4: 缓冲区溢出保护 ===\n");

    LogStream stream;
    // 试图写入超过 4000 字节的内容
    std::string huge(5000, 'x');
    stream << huge;

    printf("缓冲区 avail=%d (应为 4000,因为 huge 太大被丢弃)\n",
           stream.buffer().avail());
    printf("缓冲区 length=%d\n", stream.buffer().length());
}

void test_chain_call() {
    printf("\n=== Test 5: 链式调用 ===\n");

    LogStream stream;
    int userId = 12345;
    std::string action = "login";
    stream << "[INFO] User " << userId << " did " << action
           << " at timestamp=" << 1700000000L;

    printf("结果: [%s]\n", stream.buffer().toString().c_str());
}

int main() {
    test_basic();
    test_integer_types();
    test_string_pointer();
    test_buffer_overflow();
    test_chain_call();

    printf("\n所有测试完成 🎉\n");
    return 0;
}