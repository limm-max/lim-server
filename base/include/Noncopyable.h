// ============================================================
// Noncopyable.h
// 
// 禁止拷贝的基类。任何继承它的类都将无法被拷贝构造或拷贝赋值。
// 这是 RAII 资源管理类（如 EventLoop、Thread、Mutex）的标配基类。
// 
// 使用方式：
//     class MyClass : private Noncopyable { ... };
// 
// 设计灵感来源：muduo 网络库 / boost::noncopyable
// ============================================================

#pragma once

namespace lim {

class Noncopyable {
public:
    // 显式禁用拷贝构造和拷贝赋值
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;

protected:
    // 派生类可以正常构造/析构
    Noncopyable() = default;
    ~Noncopyable() = default;
};

} // namespace lim