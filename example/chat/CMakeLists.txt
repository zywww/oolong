add_executable(chat_client client.cpp)
target_link_libraries(chat_client oolong_net)

add_executable(chat_server server.cpp)
target_link_libraries(chat_server oolong_net)