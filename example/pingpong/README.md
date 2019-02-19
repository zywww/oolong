测试代码：
oolong pingpong
muduo pingpong
asio1.12.2 src/tests/performance

测试环境
虚拟机Ubuntu 16.04.4 LTS 
Intel(R) Core(TM) i7-8700K CPU @ 3.70GHz（虚拟机下只用了8核）
千兆网卡
内存8GB
编译器clang++ 3.8

单线程下，不同连接数
![image](https://github.com/zywww/oolong/blob/master/example/pingpong/image/single-thread.jpg)

多线程100连接数
![image](https://github.com/zywww/oolong/blob/master/example/pingpong/image/multi-thread%20100conn.jpg)

多线程1000连接数
![image](https://github.com/zywww/oolong/blob/master/example/pingpong/image/multi-thread%201000conn.jpg)
