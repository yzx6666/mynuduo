#pragma once

#include <memory>
#include <functional>

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;

using MessageCallback = std::function<void(const TcpConnectionPtr& 
                                            , Buffer*
                                            , Timestamp)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, size_t)>;


void defaultConnectionCallback(const TcpConnectionPtr& conn);
// {
    // if (conn->connected())
    // {
    //     // LOG_INFO("%s -> %s is UP\n", conn->localAddress().toIpPort().c_str(), conn->peerAddress().toIpPort().c_str());
    // }
    // else 
    // {
    //     LOG_INFO("%s -> %s is DOWN\n", conn->localAddress().toIpPort().c_str() , conn->peerAddress().toIpPort().c_str());
    // }
    
// }
void defaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime);
// {
//     buffer->retrieveAll();
// }