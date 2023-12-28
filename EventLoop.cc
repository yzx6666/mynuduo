#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>

// ��ֹһ���̴߳������EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// ����Ĭ��IO�ӿڳ�ʱʱ��
const int kPollTimeMs = 10000;

// ����wakeupfd����������subReactor������������channel
int createEventfd()
{
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error : %d\n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurrentThread::tid())
    , poller_(Poller::newDefaultPoller(this))
    , wakeupFd_(createEventfd())
    , wakeupChannel_(new Channel(this, wakeupFd_))
{
    LOG_DEBUG("EventLoop created %p : in thread %d\n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Anthor EventLoop %p exists in this thread %d\n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }

    // ����wakeupFd ���¼����ͣ��ͻص�����
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // ÿһ��eventLoop��������wakeupchannel��EPOLLIN���¼�
    wakeupChannel_->enableReading();
}                                                  

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while(!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

        for (Channel *channel : activeChannels_)
        {
            // Poller������Щchannel�����¼��ˣ�Ȼ���ϱ���EventLoop��֪ͨchannel������Ӧ���¼�
            channel->handleEvent(pollReturnTime_);
        }

        // ִ�е�ǰEventLoop�¼�ѭ����Ҫ����Ļص�����
        /*
        * IO�߳� main accept fd =�� channel subloop
        */
        doPendingFunctors();
    }
    LOG_INFO("EventLoop %p stop looping\n", this);
    looping_ = false;
}

// loop���Լ����߳���ִ��
void EventLoop::quit()
{
    quit_ = true;

    // ������������߳��У�quit����һ��subloop�е���mainLoop��quit
    if (!isInLoopThread())
    {
        wakeup();
    }
}

// �ڵ�ǰloop��ִ��cb
void EventLoop::runInLoop(Functor cb)
{
    // �ڵ�ǰ��loop�߳��У�ִ��cb
    if (isInLoopThread())
    {
        cb();
    }
    else // �ǵ�ǰ�߳�ִ��cb, ��Ҫ����loop�����߳�ִ��cb
    {
        queueInLoop(cb);
    }
}

// ��cb��������У�����loop���ڵ��̣߳�ִ��cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }

    // ������Ӧ����Ҫִ�лص��Ĳ���
    // || callingPendingFunctors_ ����˼�ǣ���ǰloop����ִ�лص��������������µĻص�
    if (!isInLoopThread() || callingPendingFunctors_) 
    {
        wakeup(); // ����loop�����߳�
    }
}

 // ����loop�����߳�, ��wakeupfdд����, wakeupchannel�������¼�����ǰloop�߳̾ͻᱻ����
void EventLoop::wakeup()
{
    uint64_t one = 1;
    size_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup writes %lu bytes instead of 8\n", n);
    }
}

// EventLoop�ķ��� =�� Poller�ķ���
void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}
    
void EventLoop::handleRead()
{
    uint64_t one = 1;
    size_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %d bytes instead of 8", n);
    }
}

void EventLoop::doPendingFunctors()// ִ�лص�
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        std::swap(pendingFunctors_, functors);
    }

    for (const Functor &functor : functors)
    {
        functor(); // ִ�е�ǰloop��Ҫִ�еĻص�����
    }

    callingPendingFunctors_ = false;
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
  return timerQueue_->addTimer(std::move(cb), time, 0.0);
}