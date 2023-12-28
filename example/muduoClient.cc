#include <mymuduo/TcpClient.h>
#include <mymuduo/EventLoop.h>
#include <mymuduo/TcpConnection.h>

#include <string>

void onConnection(const TcpConnectionPtr& conn) {
  if (conn->connected()) {
    LOG_INFO("Connected to server");
    conn->send("Hello, muduo!");
  } else {
    LOG_INFO("Disconnected from server\n");
  }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
  std::string msg(buf->retrieveAllAsString());
  LOG_INFO("Received %u bytes from server: %s", msg.size(), msg.c_str());
}

int main() {
  EventLoop loop;
  InetAddress serverAddr(9877, "127.0.0.1");
  TcpClient client(&loop, serverAddr, "EchoClient");
  client.setConnectionCallback(onConnection);
  client.setMessageCallback(onMessage);
  client.connect();
  loop.loop();
}
