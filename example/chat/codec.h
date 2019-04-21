#include <oolong/net/Callbacks.h>
#include <oolong/net/Buffer.h>
#include <oolong/net/TcpConnection.h>
#include <unistd.h>
#include <string>

struct Header //pack 1
{
    uint32_t len;
};

struct Message
{
    Header header;
    std::string message;
};


class Codec
{
public:
    static bool decodeMessage(oolong::Buffer* buf, std::string& msg)
    {
        if (buf->readableBytes() < kHeaderLength)
            return false;
        uint32_t len = buf->peekUint32();
        if (buf->readableBytes() < len + kHeaderLength)
            return false;
        msg = std::string(buf->peek()+kHeaderLength, len);
        buf->retrieve(len+kHeaderLength);
        return true;
    }

    static std::string encodeMessage(const std::string& msg)
    {
        uint32_t len = msg.length();
        oolong::Buffer buf;
        buf.appendUint32(len);
        buf.append(msg.data(), msg.length());
        return buf.retrieveAllAsString();
    }

private:
    static const int kHeaderLength = 4; // = sizeof(Header);

    // StringMessageCallback messageCallback_;
};