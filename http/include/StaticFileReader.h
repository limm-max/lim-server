// FileUtil.h
//
// 简单的文件读取工具:
//   • 打开文件 (二进制模式)
//   • 获取文件大小
//   • 一次性读到内存
//   • 文件不存在时,自动指向 404 默认页
//
// 用法:
//   FileUtil f("./resource/index.html");
//   if (!f.isValid()) { ... }
//   std::vector<char> buf(f.size());
//   f.readFile(buf);

#pragma once

#include "Logger.h"

#include <fstream>
#include <string>
#include <vector>

namespace lim
{

class StaticFileReader
{
public:
    explicit StaticFileReader(const std::string& filePath)
        : filePath_(filePath)
        , file_(filePath, std::ios::binary)   // 二进制模式,避免文本模式自动转换 \r\n
    {}

    ~StaticFileReader()
    {
        if (file_.is_open()) file_.close();
    }

    // 文件是否成功打开
    bool isValid() const { return file_.is_open(); }

    // 切到 NotFound 默认页 (打开失败时调)
    // 这条路径业务可以改,我们写死方便演示
    void resetDefaultFile(const std::string& defaultPath = "./resource/NotFound.html")
    {
        file_.close();
        file_.open(defaultPath, std::ios::binary);
    }

    // 文件大小 (字节)
    uint64_t size()
    {
        file_.seekg(0, std::ios::end);
        uint64_t sz = file_.tellg();
        file_.seekg(0, std::ios::beg);
        return sz;
    }

    // 把文件内容读到 buffer
    void readFile(std::vector<char>& buffer)
    {
        if (file_.read(buffer.data(), buffer.size()))
        {
            LOG_INFO << "File " << filePath_ << " loaded (" << buffer.size() << " bytes)";
        }
        else
        {
            LOG_ERROR << "File read failed: " << filePath_;
        }
    }

private:
    std::string   filePath_;
    std::ifstream file_;
};

} // namespace lim