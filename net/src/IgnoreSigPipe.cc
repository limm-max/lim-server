#include <signal.h>

namespace lim{
class IgnoreSigPipe
{
public:
    IgnoreSigPipe()
    {
        ::signal(SIGPIPE, SIG_IGN);
        // 一旦设置,这个进程内所有 SIGPIPE 都被忽略
        // ::write 失败时只会返回 -1 + errno=EPIPE,不会杀进程
    }
};
 
// 全局静态对象,程序加载时自动构造
IgnoreSigPipe initObj;
 
} // namespace lim