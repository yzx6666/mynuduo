#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <string.h>
#include <functional>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainloop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
}

TcpServer::TcpServer(EventLoop *loop, const InetAddress &listenAddr,
            const std::string &nameArg,
            Option option)
            : loop_(loop)
            , ipPort_(listenAddr.toIpPort())
            , name_(nameArg)
            , acceptor_(new Acceptor(loop, listenAddr, option == kReusePort))
            , threadPool_(new EventLoopThreadPool(loop, name_))
            , connectionCallback_()
            , messageCallback_()
            , nextConnId_(1)
            , started_(0)
{
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this,
                                        std::placeholders::_1, std::placeholders::_2));
}
TcpServer::~TcpServer()
{
    for (auto &it : connections_)
    {
        TcpConnectionPtr conn(it.second); // ����ֲ���shared_ptr�����Զ��ͷţ�new������TcpConnection�Ķ�����Դ
        it.second.reset();
        conn->getLoop()->runInLoop(
            std::bind(&TcpConnection::connectDestroyed, conn)
        );
    }
}

void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_++ == 0) // ��ֹһ��TcpServer��start���
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// ��һ���µĿͻ��˵����ӣ���ִ������ص�
void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr)
{
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);

    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n", 
                name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());
    
    // ͨ��sockfd��ȡ��󶨵ı�����ip��ַ�Ͷ˿���Ϣ
    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr\n");
    }

    InetAddress localAddr(local);

    // �������ӳɹ���sockfd������TcpConnection���Ӷ���
    TcpConnectionPtr conn(new TcpConnection(
                                ioLoop,
                                connName,
                                sockfd,
                                localAddr,
                                peerAddr
    ));

    connections_[connName] = conn;

    // ����Ļص������û����ø�TcpServer=��TcpConnection=��Channel=��Poller=��notify channel���ûص�
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // ��������ιرյĻص�
    conn->setCloseCallback(
        std::bind(&TcpServer::removeConnection, this, std::placeholders::_1)
    );

    // ֱ�ӵ���TcpConnection����connectEstablished
    ioLoop->runInLoop(
        std::bind(&TcpConnection::connectEstablished, conn)
    );
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop(
        std::bind(&TcpServer::removeConnectionInLoop, this, conn)
    );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", 
            name_.c_str(),conn->name().c_str());
    
    connections_.erase(conn->name());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        std::bind(&TcpConnection::connectDestroyed, conn)
    );
}