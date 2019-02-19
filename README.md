一个用于Linux多线程服务器的c++非阻塞网络库  

使用例子
一个echo服务器,代码非常简单:
```c++
    ...
class EchoServer
{
public:
    EchoServer(EventLoop* loop, const EndPoint listenAddr) :
        loop_(loop),
        server_(loop, listenAddr)
    {
        server_.setMessageCallback(std::bind(&EchoServer::onMessage, this, _1, _2));
    }

    void start()
    {
        server_.start();
    }

private:
    void onMessage(const TcpConnectionPtr conn, Buffer* buf)
    {
        conn->send(*buf);
    }
    
    EventLoop* loop_;
    TcpServer server_;
};

int main()
{
    EventLoop loop;
    EndPoint listenAddr(10010);
    EchoServer server(&loop, listenAddr);
    server.start();
    loop.loop();
}
```


依赖:  
不依赖其他库，编译需安装cmake  

使用:  
./build  

TODO:  
更高效的logger  

参考:  
muduo  

性能测试:  
见 example\pingpong  
