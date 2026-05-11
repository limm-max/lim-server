#include "AsyncLogging.h"
#include "Logger.h"
#include "Timestamp.h"

#include <stdio.h>
#include <unistd.h>
#include <memory>

using namespace lim;
std::unique_ptr<AsyncLogging> g_asyncLog;

// 工作线程——输出钩子:把日志塞给 AsyncLogging
void asyncOutput(const char* msg,int len){
    g_asyncLog->append(msg,len);
}

void bench(bool longLog){
    Logger::setOutput(asyncOutput);

    const int kBatch = 1000;
    std::string longStr(3000, 'X');
    longStr += " ";

    int cnt = 0;
    Timestamp start = Timestamp::now();

    //测试业务线程每1ms发送一条日志，整个测试周期发送30*1000条
    for (int t = 0; t < 30; ++t) {
        for (int i = 0; i < kBatch; ++i) {
            LOG_INFO << "Hello 0123456789 abcdefghijklmnopqrstuvwxyz "
                     << (longLog ? longStr : "") << cnt;
            ++cnt;
        }
        usleep(1000);   // 1ms,模拟真实业务节奏
    }

    Timestamp end = Timestamp::now();
    double seconds = (end.microSecondsSinceEpoch() -
                      start.microSecondsSinceEpoch()) / 1e6;
    printf("写 %d 条日志耗时 %.3f 秒, %.0f 条/秒\n",
           cnt, seconds, cnt / seconds);
}

int main(int argc, char* argv[]) {
    int rollSize = 500 * 1024 * 1024;  // 500MB 滚动

    g_asyncLog.reset(new AsyncLogging("./async_test", rollSize));
    g_asyncLog->start();

    bool longLog = (argc > 1);
    bench(longLog);

    g_asyncLog->stop();
    printf("测试完成 🎉\n");
    return 0;
}

    
