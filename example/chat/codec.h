#include <oolong/net/Callbacks.h>
#include <oolong/net/Buffer.h>
#include <oolong/net/TcpConnection.h>
#include <unistd.h>
#include <string>

struct Header //pack 1
{
    uint16_t len_total;
    uint16_t len_name;
    std::string name;
    uint8_t op;
};

struct Message
{
    Header header;
    std::string message;
};

// 2byte len
// other byte content 

class Codec
{
public:
    //using StringMessageCallback = std::function<void(const oolong::TcpConnectionPtr&, const std::string&)>;

    // Codec(StringMessageCallback cb)
    // {
    // }
    
    // for callback
    // void onMessage(const oolong::TcpConnectionPtr& conn, oolong::Buffer* buf)
    // {
    //     while (buf->readableBytes() >= kHeaderLength)
    //     {
    //         std::string msg;
    //         if (!decodeMessage(buf, msg))
    //             break;
    //         messageCallback_(conn, msg);
    //     }
    // }

    bool decodeMessage(oolong::Buffer* buf, std::string& msg)
    {
        if (buf->readableBytes() < kHeaderLength)
            return false;
        uint16_t len = buf->peekUint16();
        if (buf->readableBytes() < len)
            return false;
        msg = std::string(buf->peek()+kHeaderLength, len-kHeaderLength);
        buf->retrieve(len);
        return true;
    }

    std::string encodeMessage(const std::string& msg)
    {
        uint16_t len = kHeaderLength + msg.length(); // todo 检查是否大于16位
        oolong::Buffer buf;
        buf.appendUint16(len);
        return buf.retrieveAllAsString() + msg;
    }

private:
    static const int kHeaderLength = 2; // = sizeof(Header);

    // StringMessageCallback messageCallback_;
};