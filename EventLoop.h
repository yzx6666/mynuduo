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

// �¼�ѭ���࣬��Ҫ Channel �� Poller(epoll�ĳ���)
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // �����¼�ѭ��
    void loop();
    // �˳��¼�ѭ��
    void quit();

    Timestamp pollRetuenTime() const { return pollReturnTime_; }

    // �ڵ�ǰloop��ִ��cb
    void runInLoop(Functor cb);
    // ��cb��������У�����loop���ڵ��̣߳�ִ��cb
    void queueInLoop(Functor cb);

    // ����loop�����߳�
    void wakeup();

    // EventLoop�ķ��� =�� Poller�ķ���
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel);
    
    // �ж�EventLoop�����Ƿ����Լ����߳�����
    bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

    using TimerCallback = std::function<void()>;
    TimerId runAfter(double delay, TimerCallback cb);
    TimerId runAt(Timestamp time, TimerCallback cb);


private:
    void handleRead(); // wake up
    void doPendingFunctors(); // ִ�лص�

    using ChannelList = std::vector<Channel*>;
    
    std::atomic_bool looping_; // ԭ�Ӳ���
    std::atomic_bool quit_; // �˳�loopѭ��
    
    const pid_t threadId_; // ��¼��ǰloop�����̵߳�id
    
    Timestamp pollReturnTime_; // poller���ط����¼���channels��ʱ���
    std::unique_ptr<Poller> poller_;
    
    int wakeupFd_; // ��Ҫ�����ǵ�mainLoop��ȡһ�����û���channel��ͨ����ѯ�㷨ѡ��һ��subLoop��ͨ���ó�Ա����
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    std::atomic_bool callingPendingFunctors_; // ��ʶ��ǰloop�Ƿ�����Ҫִ�еĻص�
    std::vector<Functor> pendingFunctors_; // �洢loop��Ҫִ�е����лص�����
    std::mutex mutex_; // ������֤����vector�������̰߳�ȫ����

    std::unique_ptr<TimerQueue> timerQueue_;

};
