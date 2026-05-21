// MimeType.h
//
// 文件扩展名 → MIME 类型 映射
//不同扩展名对应不同 Content-Type,浏览器靠这个判断怎么处理
// 覆盖常见的 web 静态资源类型,够 Day 16 学习用

#pragma once

#include <string>
#include <unordered_map>

namespace lim
{

inline std::string getMimeType(const std::string& path)
{
    // 找最后一个 '.' 后面的扩展名
    auto dot = path.find_last_of('.');  //找到最后一个.所在位置
    if (dot == std::string::npos) return "application/octet-stream";

    std::string ext = path.substr(dot); //取出文件类型，带.

    static const std::unordered_map<std::string, std::string> mimeMap = {
        {".html", "text/html; charset=utf-8"},
        {".htm",  "text/html; charset=utf-8"},
        {".css",  "text/css"},
        {".js",   "application/javascript"},
        {".json", "application/json"},
        {".txt",  "text/plain; charset=utf-8"},
        {".png",  "image/png"},
        {".jpg",  "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif",  "image/gif"},
        {".svg",  "image/svg+xml"},
        {".ico",  "image/x-icon"},
        {".pdf",  "application/pdf"},
    };

    auto it = mimeMap.find(ext);
    return it != mimeMap.end() ? it->second : "application/octet-stream";
}

} // namespace lim