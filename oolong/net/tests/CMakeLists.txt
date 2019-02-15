add_executable(channel_test Channel_test.cpp)
target_link_libraries(channel_test oolong_net)

add_executable(eventloop_test EventLoop_test.cpp)
target_link_libraries(eventloop_test oolong_net oolong_base)

add_executable(tcpserver_test TcpServer_test.cpp)
target_link_libraries(tcpserver_test oolong_net oolong_base)

add_executable(tcpclient_test TcpClient_test.cpp)
target_link_libraries(tcpclient_test oolong_net oolong_base)

add_executable(eventloopthread_test EventLoopThread_test.cpp)
target_link_libraries(eventloopthread_test oolong_net oolong_base)

add_executable(eventloopthreadpool_test EventLoopThreadPool_test.cpp)
target_link_libraries(eventloopthreadpool_test oolong_net oolong_base)

add_executable(timer_test Timer_test.cpp)
target_link_libraries(timer_test oolong_net oolong_base)

if(BOOSTTEST_LIBRARY)
    add_executable(buffer_unittest Buffer_unittest.cpp)
    target_link_libraries(buffer_unittest oolong_net boost_unit_test_framework)
    add_test(NAME buffer_unittest COMMAND buffer_unittest)

    add_executable(endpoint_unittest EndPoint_unittest.cpp)
    target_link_libraries(endpoint_unittest oolong_net boost_unit_test_framework)
    add_test(NAME endpoint_unittest COMMAND endpoint_unittest)
endif()