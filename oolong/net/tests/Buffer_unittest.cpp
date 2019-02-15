#include <oolong/net/Buffer.h>

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <string.h>

using namespace oolong;
using namespace std;

BOOST_AUTO_TEST_CASE(testReadableBytes)
{
    Buffer buf;
    BOOST_CHECK_EQUAL(buf.readableBytes(), 0);

    string str(200, 'x');
    buf.append(str.c_str(), str.size());
    BOOST_CHECK_EQUAL(buf.readableBytes(), 200);

    buf.retrieve(50);
    BOOST_CHECK_EQUAL(buf.readableBytes(), str.size()-50);

    buf.append(str.c_str(), str.size());
    BOOST_CHECK_EQUAL(buf.readableBytes(), str.size()*2-50);
}

