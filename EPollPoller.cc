#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <strings.h>
#include <unistd.h>

// channel未添加到poller中
const int KNew = -1; // channel的成员index_ = -1
// channel已添加到poller中
const int KAdded = 1;
// channel从poller中删除
const int KDelete = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop)
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) // vector<epoll_event>
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error: %d\n", errno);
    }
}

EPollPoller::~EPollPoller()
{
    ::close(epollfd_);
}

// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        activeChannels->push_back(channel); 
        // EventLoop就拿到了他的Poller给他返回的所有时间发生的channel列表了
    }
}

// 更新channel通道 epoll_stl add/mod/del
void EPollPoller::update(int operation, Channel *channel)
{
    epoll_event event;
    int fd = channel->fd();
    bzero(&event, sizeof event);
    
    event.events = channel->events();
    
    event.data.ptr = channel;
    //event.data.fd = fd;


    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error:%d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}

// 重写基类Poller的抽象方法
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
    // 用DEBUG更好
    LOG_INFO("func %s => fd total count:%d\n", __FUNCTION__, channels_.size());

    int numEvents = ::epoll_wait(epollfd_, &*events_.begin()
            , static_cast<int>(events_.size()), timeoutMs);

    int saveError = errno;

    Timestamp now(Timestamp::now());

    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("%s timeout! \n", __FUNCTION__);
    }
    else
    {
        if (saveError != EINTR)
        {
            errno = saveError;
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }

    return now;
}

// channel update remove => EventLoop updateChannel removeChannel => Poller
/**
 *          EventLoop
 *    Channel     Poller
 *              ChannelMap <fd, channel*>
*/
void EPollPoller::updateChannel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func = %s fd = %d events = %d index = %d\n", 
            __FUNCTION__, channel->fd(), channel->events(), channel->index());

    if (index == KDelete || index == KNew)
    {
        if (index == KNew)
        {
            int fd = channel->fd();
            channels_[fd] = channel;
        }

        channel->set_index(KAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else // channel 已经注册过
    {
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(KDelete);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

// 从poller中删除channel
void EPollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func = %s fd = %d\n", 
            __FUNCTION__, channel->fd());

    int index = channel->index();
    if (index == KAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }

    channel->set_index(KNew);
}