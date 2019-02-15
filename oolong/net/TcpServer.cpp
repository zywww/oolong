#include <cassert>

#include <oolong/base/Logger.h>
#include <oolong/net/TcpServer.h>
#include <oolong/net/TcpConnection.h>
#include <oolong/net/SocketAPI.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/EventLoopThreadPool.h>


using namespace oolong;

TcpServer::TcpServer(EventLoop* loop, const EndPoint& endpoint, bool reusePort) :
    loop_(loop),
    acceptor_(loop, endpoint, reusePort),
    threadPool_(loop)
{
    assert(loop);
    acceptor_.setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
    LogDebug << "tcp server create";
}

TcpServer::~TcpServer()
{
    LogDebug << "tcp server destruct";
}

void TcpServer::setThreadNum(size_t num)
{
    threadPool_.setThreadNum(num);
}

void TcpServer::start()
{
    if (started_) return;
    started_ = true;
    threadPool_.start(threadInitCallback_);
    acceptor_.listen();
    LogDebug << "tcp server start";
}

void TcpServer::stop()
{
    acceptor_.stopListening();
    for (auto& conn : connections_)
        ;//conn->close();
    
    connections_.clear(); 
    LogDebug << "tcp server stop";
}

void TcpServer::newConnection(int sockfd, const EndPoint& peer)
{
    // 创建connection
    EventLoop* ownerLoop = threadPool_.getNextLoop();
    EndPoint local = SocketAPI::getLocalAddr(sockfd); // todo assert local == acceptor.localAddress
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(ownerLoop, sockfd, local, peer);
    conn->setMessageCallback(messageCallback_);
    conn->setConnectionCallback(connectionCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1));
    connections_.insert(conn);
    ownerLoop->runInLoop(std::bind(&TcpConnection::connectionEstablished, conn));
}

void TcpServer::removeConnection(TcpConnectionPtr conn)
{
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
    // todo 有可能在非loop线程调用吗?
}

void TcpServer::removeConnectionInLoop(TcpConnectionPtr conn)
{
    loop_->assertInLoopThread();
    assert(connections_.erase(conn));
    EventLoop* ownerLoop = conn->getLoop();
    ownerLoop->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
}