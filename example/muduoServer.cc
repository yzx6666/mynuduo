#include <mymuduo/TcpServer.h>
#include <mymuduo/EventLoop.h>
#include <mymuduo/TcpConnection.h>
#include <string>

void onConnection(const TcpConnectionPtr& conn) {
  if (conn->connected()) {
    LOG_INFO("Client connected\n");
  } else {
    LOG_INFO("Client disconnected\n");
  }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
  std::string msg(buf->retrieveAllAsString());
  LOG_INFO("Received %d bytes from %s\n", msg.size(), conn->peerAddr().toIpPort().c_str());
  conn->send(msg);  // Echo back the message
}

int main() {
  EventLoop loop;
  InetAddress listenAddr(9877);
  TcpServer server(&loop, listenAddr, "EchoServer");
  server.setConnectionCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.start();
  loop.loop();
}
