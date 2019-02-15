#include <memory>

#include <oolong/base/Logger.h>
#include <oolong/net/Connector.h>
#include <oolong/net/SocketAPI.h>
#include <oolong/net/EventLoop.h>
#include <oolong/net/TcpClient.h>
#include <oolong/net/TcpConnection.h>

using namespace oolong;

TcpClient::TcpClient(EventLoop* loop, const EndPoint& peer) :
    loop_(loop),
    connector_(std::make_shared<Connector>(loop, peer)),
    connectionCallback_(defaultConnectionCallback)
{
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
    LogDebug << "TcpClient::TcpClient()";
}

TcpClient::~TcpClient()
{
    LogDebug << "TcpClient::~TcpClient()";
    TcpConnectionPtr conn;
    bool unique;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        unique = connection_.unique();
        conn = connection_;
    }
    if (conn)
    {
        CloseCallback cb = std::bind(&TcpClient::removeConnection, this, _1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique)
            conn->forceClose();
    }
    else
    {
        connector_->stop();
    }
}

void TcpClient::connect()
{
    connect_ = true;
    connector_->start();
}

void TcpClient::disconnect()
{
    connect_ = false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_)
        connection_->shutdown();
}

void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
    loop_->assertInLoopThread();
    EndPoint local = SocketAPI::getLocalAddr(sockfd);
    EndPoint peer = SocketAPI::getPeerAddr(sockfd);
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, sockfd, local, peer);
    conn->setMessageCallback(messageCallback_);
    conn->setConnectionCallback(connectionCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnectionByPeer, this, _1));
    {
        std::lock_guard<std::mutex> lock(mutex_);
        connection_ = conn;
    }
    conn->connectionEstablished();
}

void TcpClient::removeConnectionByPeer(const TcpConnectionPtr& conn)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        assert(conn == connection_);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
    if (retry_ && connect_)
    {
        LogDebug << "TcpClient reconnecting";
        connector_->restart();
    }
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->queueInLoop(std::bind(&TcpConnection::connectionDestroyed, conn));
}