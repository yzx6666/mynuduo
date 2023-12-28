#include "TcpConnection.h"
#include "Logger.h"
#include "Timestamp.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Callbacks.h"

#include <errno.h>
#include <memory>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <string.h>


static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop, const std::string &name
                , int sockfd
                , const InetAddress &localAddr
                , const InetAddress &peerAddr)
        : loop_(CheckLoopNotNull(loop))
        , name_(name)
        , state_(kConnecting)
        , reading_(true)
        , socket_(new Socket(sockfd))
        , channel_(new Channel(loop, sockfd))
        , localAddr_(localAddr)
        , peerAddr_(peerAddr)
        , highWaterMark_(64*1024*1024) // 64M
{
    // ��channel������Ӧ�Ļص���poller��channel֪ͨ����Ȥ���¼������ˣ�channel��ص���Ӧ�Ĳ�������
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );

    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );

    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );

    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );

    LOG_INFO("TcpConnection::ctor[%s] at fd = %d\n", name_.c_str(), sockfd);
    socket_->setKeepAlive(true);
}
TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::dtor[%s] at fd = %d state = %d\n", name_.c_str(), channel_->fd(), (int)state_);
}


// ��������
void TcpConnection::send(const std::string &buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf.c_str(), buf.size());
        }
        else 
        {
            loop_->runInLoop(
                std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size())
            );
        }
    }
}


// �ر�����
void TcpConnection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this)
        );
    }
}


// ���ӽ���
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); // ע��epollin�¼�

    // �����ӽ���ִ�лص�
    connectionCallback_(shared_from_this());
}

// ��������
void TcpConnection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }

    channel_->remove(); // ��channel��poller��ɾ��
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(), &saveErrno);
    if (n > 0)
    {
        // �ѽ������û����пɶ��¼������ˣ������û�����Ļص�����onMessage
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = saveErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if (channel_->isWriting())
    {
        int saveErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                channel_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // ����loop_��Ӧ��thread�̣߳�ִ�лص�
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                if (state_ == kDisconnecting)
                {
                    shutdownInLoop();
                }
            }
        }
        else
        {
            LOG_ERROR("TcpConnection::handleWrite error!\n");
        }
    }
    else
    {
        LOG_ERROR("TcpConnection fd = %d is down, no more writing\n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd = %d state = %d\n", channel_->fd(), (int)state_);

    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr); // ִ�йرյĻص�
    closeCallback_(connPtr); // �ر����ӵĻص� ִ�е���TcpServer::removeConnection
}

void TcpConnection::handleError()
{
    int optval;
    int err = 0;
    socklen_t optlen = sizeof optval;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else 
    {
        err = optval;
    }

    LOG_ERROR("TcpConnection::handleError name:%s -SO_ERROR:%d\n", name_.c_str(), err);
}   

// Ӧ��д�Ŀ죬�ں˷���������Ҫ�Ѵ����͵�����д�뻺��������������ˮλ�ص�
void TcpConnection::sendInLoop(const void *messgae, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // ֮ǰ���ù�connection��shutdown�������ٷ�����
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnectioned give up writing!\n");
        return;
    }
    
    // channel��һ��д���ݣ����һ�����û�з�������
    if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), messgae, len); 
        if (nwrote >= 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                //  һ����ȫ�����ͣ��Ͳ����ٸ�channel����epollout�¼���
                loop_->queueInLoop(std::bind(writeCompleteCallback_, shared_from_this()));
            }
        }
        else
        {
            nwrote = 0;
            if (errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                if (errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }

    // һ�η���û��ȫ�������꣬ʣ�µ�������Ҫ���浽��������Ȼ���channel
    // ע��epollout�¼���poller����tcp�ķ��ͻ������пռ䣬��֪ͨ��Ӧ��sock-channel������WriteCallback
    // Ҳ���ǵ���TcpConnection::handleWrite�������ѷ��ͻ�����������ȫ���������
    if (! faultError && remaining > 0)
    {
        // Ŀǰ���ͻ�����ʣ��Ĵ��������ݵĳ���
        size_t oldlen = outputBuffer_.readableBytes();
        if (oldlen + remaining >= highWaterMark_
            && oldlen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldlen + remaining)
            );
        }
        outputBuffer_.append((char *)messgae + nwrote, remaining);

        if (!channel_->isWriting())
        {
            channel_->enableWriting();
        }
    }
}

void TcpConnection::shutdownInLoop()
{
    if (!channel_->isWriting()) // ˵���������������Ѿ�ȫ���������
    {
        socket_->shutdownWrite();
    }
}


void TcpConnection::forceClose()
{
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting)
  {
    setState(kDisconnecting);
    loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::forceCloseInLoop()
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}