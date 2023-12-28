#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo中多路事件分发器的核心IO模块
class Poller : noncopyable
{
private:
    EventLoop *ownerLoop_; // 定义Poller所属的事件循环EventLoop
protected:
    // map key : sockfd, value sockfd所属的channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller();

    // 给所有IO复用保留统一的结构
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    //  判断参数channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const;

    // 可以通过该接口获取默认的IP复用具体实现
    static Poller* newDefaultPoller(EventLoop *loop);
};
