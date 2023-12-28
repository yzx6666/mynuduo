#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo�ж�·�¼��ַ����ĺ���IOģ��
class Poller : noncopyable
{
private:
    EventLoop *ownerLoop_; // ����Poller�������¼�ѭ��EventLoop
protected:
    // map key : sockfd, value sockfd������channel
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller();

    // ������IO���ñ���ͳһ�Ľṹ
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel *channel) = 0;
    virtual void removeChannel(Channel *channel) = 0;

    //  �жϲ���channel�Ƿ��ڵ�ǰPoller����
    bool hasChannel(Channel *channel) const;

    // ����ͨ���ýӿڻ�ȡĬ�ϵ�IP���þ���ʵ��
    static Poller* newDefaultPoller(EventLoop *loop);
};
