#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{
}

Poller::~Poller() = default;

//  �жϲ���channel�Ƿ��ڵ�ǰPoller����
bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
    // ���ҵ�channelList���Ƿ��и�fd��Ȼ���жϸ�fd������channel�Ƿ�һ��
}