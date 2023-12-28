#include "Poller.h"
#include "Channel.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{
}

Poller::~Poller() = default;

//  判断参数channel是否在当前Poller当中
bool Poller::hasChannel(Channel *channel) const
{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
    // 先找到channelList中是否有该fd，然后判断该fd所属的channel是否一样
}