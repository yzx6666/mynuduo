#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "Logger.h"

#include <functional>
#include <memory>

class EventLoop;

/**
 * Channel ���Ϊͨ������װ��sockfd�������Ȥ��event����EPOLLIN,EPOLLOUT�¼�
 * ������poller���صľ����¼�
*/
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;
    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd�õ�poller֪ͨ�Ժ󣬴����¼���
    void handleEvent(Timestamp receiveTime);

    // ���ûص���������
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // ��ֹ��channel���ֶ�remove�� channel����ִ�лص�
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = (int)revt; }

    // ����fd��Ӧ���¼�״̬
    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }

    // ����fd��ǰ���¼�״̬
    bool isNoneEvent() { return events_ == kNoneEvent; }
    bool isWriting() { return events_ & kWriteEvent; }
    bool isReading() { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    EventLoop *ownerLoop() { return loop_; }
    void remove();

private:

    void update();
    void handleEventWithGuard(Timestamp receiveTime); 

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // �¼�ѭ��
    const int fd_; // fd�� Poller�����Ķ���
    int events_; // ע��fd����Ȥ���¼�
    int revents_; // Poller���ؾ��巢�����¼�
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // ��ΪChannelͨ���ܹ���֪fa���շ����ľ�����¼�revents�����Ե��þ���Ļص�
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};