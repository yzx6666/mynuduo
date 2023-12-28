#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"
#include "TimerId.h"
#include "TimerQueue.h"

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class Channel;
class Poller;

// 事件循环类，主要 Channel 和 Poller(epoll的抽象)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollRetuenTime() const { return pollReturnTime_; }

    // 在当前loop中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop所在的线程，执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在线程
    void wakeup();

    // EventLoop的方法 =》 Poller的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);
    
    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    using TimerCallback = std::function<void()>;
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runAt(Timestamp time, TimerCallback cb);


private:
    void handleRead(); // wake up
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel*>;
    
    std::atomic_bool looping_; // 原子操作
    std::atomic_bool quit_; // 退出loop循环
    
    const pid_t threadId_; // 记录当前loop所在线程的id
    
    Timestamp pollReturnTime_; // poller返回发生事件的channels是时间点
    std::unique_ptr<Poller> poller_;
    
    int wakeupFd_; // 主要作用是当mainLoop获取一个新用户的channel，通过轮询算法选择一个subLoop，通过该成员唤醒
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有回调操作
    std::mutex mutex_; // 用来保证上面vector容器的线程安全操作

    std::unique_ptr<TimerQueue> timerQueue_;

};
