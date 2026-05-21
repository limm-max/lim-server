// JsonUtil.h
//json封装，后续可以替换集成库
//前后端用 JSON 字符串交换数据
// 把 nlohmann/json 引进 lim 命名空间,起个短别名
// 业务侧只 include 这个文件即可

#pragma once

#include <nlohmann/json.hpp>

namespace lim
{

// 给 nlohmann::json 起个别名,业务侧直接用 json 就行
using json = nlohmann::json;

} // namespace lim