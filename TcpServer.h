#pragma once
/**
 * �û�ʹ��muduo��д����������
*/
#include "noncopyable.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Callbacks.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include "Timer.h"
#include "TimerId.h"
#include "TimerQueue.h"

#include <atomic>
#include <unordered_map>

// ����ķ��������ʹ�õ���
class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop, const InetAddress &listenAddr,
                const std::string &nameArg, 
                Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitcallback(const ThreadInitCallback &cb) { threadInitCallback_ = cb; }
    void setConnectionCallback(const ConnectionCallback &cb) { connectionCallback_ = cb; }
    void setMessageCallback (const MessageCallback &cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback (const WriteCompleteCallback &cb) { writeCompleteCallback_ = cb; }

    void setThreadNum(int numThreads); // ����subLoop�ĸ���

    void start(); // ��������������
    
private:

    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // baseLoop���û������loop

    const std::string ipPort_;
    const std::string name_;

    std::unique_ptr<Acceptor> acceptor_;  // ������mainLoop�� �����Ǽ����������¼�
    std::shared_ptr<EventLoopThreadPool> threadPool_;

    ConnectionCallback connectionCallback_; // �������ӵĻص�
    MessageCallback messageCallback_; // �ж�д��Ϣ�Ļص�
    WriteCompleteCallback writeCompleteCallback_; // ��Ϣ��������Ժ�Ļص�

    ThreadInitCallback threadInitCallback_; // �̳߳�ʼ���Ļص�
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_;
};
