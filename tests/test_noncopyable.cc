// 测试 Noncopyable 基类
#include "Noncopyable.h"

class Foo : private lim::Noncopyable {
public:
    Foo() = default;
    int x = 42;
};

int main() {
    Foo a;
    // Foo b = a;  // 取消这行注释应该编译报错
    return a.x == 42 ? 0 : 1;
}