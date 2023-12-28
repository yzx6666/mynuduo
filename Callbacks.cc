#include "Callbacks.h"
#include "Timestamp.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include "Logger.h"
#include "InetAddress.h"

void defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_INFO("%s -> %s is UP\n", 
        conn->localAddr().toIpPort().c_str(),
        conn->peerAddr().toIpPort().c_str(),
        conn->connected() ? "UP" : "DOWN");
}
void defaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime)
{
    buffer->retrieveAll();
}