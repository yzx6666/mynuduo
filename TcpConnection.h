#pragma once

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"
#include "Buffer.h"

#include <memory>
#include <atomic>

class Channel;
class EventLoop;
class Socket;

/**
 *TcpServer => Acceptor => ��һ�����û����ӣ� ͨ��accept�����õ�connfd
    =�� TcpConnection ���ûص� -��Channel -�� poller -��channel�Ļص�����
*/
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop *loop, const std::string &name
                , int sockfd
                , const InetAddress &localAddr
                , const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string &name() const { return name_; }
    const InetAddress &localAddr() { return localAddr_; }
    const InetAddress &peerAddr() { return peerAddr_; }

    bool connected() const { return state_ == kConnected; }

    // ��������
    void send(const std::string &message);

    // �ر�����
    void shutdown();

    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb) { highWaterMarkCallback_ = cb; }
    void setCloseCallback(const CloseCallback &cb) { closeCallback_ = cb; }

    // ���ӽ���
    void connectEstablished();
    // ��������
    void connectDestroyed();

    void forceClose();
    void forceCloseInLoop();

private:
    enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting};
    void setState(StateE state) { state_ = state; }

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void *messgae, size_t len);
    void shutdownInLoop();

    EventLoop *loop_; // ���ﲻ��baseLoop����ΪTcpConnection����subLoop�е�
    const std::string  name_;
    std::atomic_int state_;
    bool reading_;

    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; // �������ӵĻص�
    MessageCallback messageCallback_; // �ж�д��Ϣ�Ļص�
    WriteCompleteCallback writeCompleteCallback_; // ��Ϣ��������Ժ�Ļص�
    HighWaterMarkCallback highWaterMarkCallback_;
    CloseCallback closeCallback_;
    size_t highWaterMark_;

    Buffer inputBuffer_; // �������ݵĻ�����
    Buffer outputBuffer_; // �������ݵĻ�����
};