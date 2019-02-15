#include <oolong/net/EndPoint.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <string>

using namespace oolong;
using std::string;

BOOST_AUTO_TEST_CASE(testEndPoint)
{
    EndPoint addr(1234);
    BOOST_CHECK_EQUAL(addr.toIp(), string("0.0.0.0"));
    BOOST_CHECK_EQUAL(addr.toIpPort(), string("0.0.0.0:1234"));


    EndPoint addr2("1.2.3.4", 8888);
    BOOST_CHECK_EQUAL(addr2.toIp(), string("1.2.3.4"));
    BOOST_CHECK_EQUAL(addr2.toIpPort(), string("1.2.3.4:8888"));

    EndPoint addr3("255.254.253.252", 65535);
    BOOST_CHECK_EQUAL(addr3.toIp(), string("255.254.253.252"));
    BOOST_CHECK_EQUAL(addr3.toIpPort(), string("255.254.253.252:65535"));
}


