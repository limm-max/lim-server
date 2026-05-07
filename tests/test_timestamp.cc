// 测试 Timestamp 类的功能
#include "Timestamp.h"

#include <iostream>
#include <unistd.h>   // sleep

using namespace lim;

void test_basic() {
    std::cout << "=== test_basic ===" << std::endl;
    
    // 测试默认构造
    Timestamp t1;
    std::cout << "默认构造: valid=" << t1.valid() 
              << ", value=" << t1.microSecondsSinceEpoch() << std::endl;
    
    // 测试 now()
    Timestamp t2 = Timestamp::now();
    std::cout << "当前时间: " << t2.toString() << std::endl;
    std::cout << "格式化: " << t2.toFormattedString() << std::endl;
    
    // 测试 invalid()
    Timestamp t3 = Timestamp::invalid();
    std::cout << "无效时间戳 valid=" << t3.valid() << std::endl;
}

void test_compare() {
    std::cout << "\n=== test_compare ===" << std::endl;
    
    Timestamp t1 = Timestamp::now();
    sleep(1);  // 等 1 秒
    Timestamp t2 = Timestamp::now();
    
    std::cout << "t1 < t2: " << (t1 < t2) << std::endl;   // 应该为 true
    std::cout << "t1 == t2: " << (t1 == t2) << std::endl; // 应该为 false
    std::cout << "时间差(微秒): " 
              << (t2.microSecondsSinceEpoch() - t1.microSecondsSinceEpoch()) 
              << std::endl;
}

void test_addTime() {
    std::cout << "\n=== test_addTime ===" << std::endl;
    
    Timestamp now = Timestamp::now();
    Timestamp future = addTime(now, 5.0);  // 5 秒后
    
    std::cout << "现在: " << now.toFormattedString() << std::endl;
    std::cout << "5秒后: " << future.toFormattedString() << std::endl;
}

int main() {
    test_basic();
    test_compare();
    test_addTime();
    return 0;
}