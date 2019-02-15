set(base_srcs
    Timestamp.cpp
    CurrentThread.cpp)

add_library(oolong_base ${base_srcs})
target_link_libraries(oolong_base pthread rt)

install(TARGETS oolong_base DESTINATION lib)

set(
    header_files
    Timerstamp.h
)

file(GLOB HEADERS "*.h")
install(FILES ${HEADERS} DESTINATION include/oolong/base)

if (NOT CMAKE_BUILD_NO_EXAMPLES)
    add_subdirectory(tests)
endif()