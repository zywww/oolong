set(net_srcs 
    Channel.cpp
    EPollPoller.cpp
    EventLoop.cpp
    Timer.cpp
    Buffer.cpp
    Acceptor.cpp
    EndPoint.cpp
    SocketAPI.cpp
    TcpConnection.cpp
    TcpServer.cpp
    TcpSocket.cpp
    Connector.cpp
    TcpClient.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp)

add_library(oolong_net ${net_srcs})
target_link_libraries(oolong_net oolong_base pthread)

install(TARGETS oolong_net DESTINATION lib)

set(HEADERS
    Callbacks.h
    Channel.h
    EndPoint.h)
install(FILES ${HEADERS} DESTINATION include/oolong/net)


if(NOT CMAKE_BUILD_NO_EXAMPLES)
  add_subdirectory(tests)
endif()