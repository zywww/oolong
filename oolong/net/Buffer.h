#pragma once
#include <string>
#include <cassert>
#include <algorithm>

#include <oolong/base/noncopyable.h>

namespace oolong
{
///     +------------------+------------------+
///     |  readable bytes  |  writable bytes  |
///     |     (CONTENT)    |                  |
///     +------------------+------------------+
///     |                  |                  |
/// readerIndex   <=   writerIndex    <=     size
    class Buffer : noncopyable
    {
        static const size_t kInitSize = 1024;
    public:
        Buffer(size_t initSize = kInitSize) :
            size_(initSize)
        {
            data_ = new char[size_];
        }
        ~Buffer()
        {
            delete[] data_;
        }

        const char* peek() const
        {
            return data_ + readerIndex_;
        };

        size_t readableBytes() const
        {
            assert(readerIndex_ <= writerIndex_);
            return writerIndex_ - readerIndex_;
        }

        void retrieveAll()
        {
            readerIndex_ = writerIndex_ = 0;
        }

        std::string retrieveAllAsString()
        {
            std::string str(peek(), readableBytes());
            retrieveAll();
            return str;
        }

        void retrieve(size_t n)
        {
            assert(n <= readableBytes());
            if (n < readableBytes())
                readerIndex_ += n;
            else 
                retrieveAll();
        }

        void append(const char* data, size_t len)
        {
            ensureWritableBytes(len);
            std::copy(data, data+len, data_+writerIndex_);
            writerIndex_ += len;
        }

        void appendUint8(uint8_t val)
        {
            append(reinterpret_cast<const char*>(&val), sizeof(val));
        }
        void appendInt8(int8_t val)
        {
            appendUint8(val);
        }
        void appendUint16(uint16_t val)
        {
            val = htobe16(val);
            append(reinterpret_cast<const char*>(&val), sizeof(val));
        }
        void appendInt16(int16_t val)
        {
            val = htobe16(val);
            append(reinterpret_cast<const char*>(&val), sizeof(val));
        }
        void appendUint32(uint32_t val)
        {
            val = htobe32(val);
            append(reinterpret_cast<const char*>(&val), sizeof(val));
        }
        void appendInt32(uint32_t val)
        {
            val = htobe32(val);
            append(reinterpret_cast<const char*>(&val), sizeof(val));
        }
        void appendUint64(uint64_t val)
        {
            val = htobe64(val);
            append(reinterpret_cast<const char*>(&val), sizeof(val));
        }
        void appendInt64(uint64_t val)
        {
            val = htobe64(val);
            append(reinterpret_cast<const char*>(&val), sizeof(val));
        }

        uint8_t peekUint8()
        {
            uint8_t val;
            assert(readableBytes() >= sizeof(val));
            std::copy(peek(), peek()+sizeof(val), reinterpret_cast<char*>(&val));
            return val;
        }
        int8_t peekInt8()
        {
            return peekUint8();
        }
        uint16_t peekUint16()
        {
            uint16_t val;
            assert(readableBytes() >= sizeof(val));
            std::copy(peek(), peek()+sizeof(val), reinterpret_cast<char*>(&val));
            return be16toh(val);
        }
        int16_t peekInt16()
        {
            return peekInt16();
        }
        uint32_t peekUint32()
        {
            uint32_t val;
            assert(readableBytes() >= sizeof(val));
            std::copy(peek(), peek()+sizeof(val), reinterpret_cast<char*>(&val));
            return be32toh(val);
        }
        int32_t peekInt32()
        {
            return peekUint32();
        }
        uint64_t peekUint64()
        {
            uint64_t val;
            assert(readableBytes() >= sizeof(val));
            std::copy(peek(), peek()+sizeof(val), reinterpret_cast<char*>(&val));
            return be64toh(val);
        }
        int64_t peekInt64()
        {
            return peekUint64();
        }


        uint8_t readUint8()
        {
            uint8_t val = peekUint8();
            retrieve(sizeof(val));
            return val;
        }
        int8_t readInt8()
        {
            return readUint8();
        }
        uint16_t readUint16()
        {
            uint16_t val = peekUint16();
            retrieve(sizeof(val));
            return val;
        }
        int16_t readInt16()
        {
            return readUint16();
        }
        uint32_t readUint32()
        {
            uint32_t val = peekUint32();
            retrieve(sizeof(val));
            return val;
        }
        int32_t readInt32()
        {
            return readUint32();
        }
        uint64_t readUint64()
        {
            uint64_t val = peekUint64();
            retrieve(sizeof(val));
            return val;
        }
        int64_t readInt64()
        {
            return readUint64();
        }

        ssize_t readFd(int fd);

    private:

        size_t writableBytes() const 
        {
            assert(writerIndex_ <= size_);
            return size_ - writerIndex_;
        }

        size_t alreadyReadBytes() const
        {
            return readerIndex_;
        }

        void ensureWritableBytes(size_t len)
        {
            if (len <= writableBytes())
                return;

            if (alreadyReadBytes() + writableBytes() >= len)
            {
                // 移动内容到空间的前部
                size_t readable = readableBytes();
                std::copy(data_+readerIndex_, data_+writerIndex_, data_);
                readerIndex_ = 0;
                writerIndex_ = readable;
            }
            else
            {
                // 把内容复制到新空间
                size_t newSize = size_ * 2;
                newSize = std::max(newSize, readableBytes() + len);
                char* newData = new char[newSize];
                std::copy(data_+readerIndex_, data_+writerIndex_, newData);
                writerIndex_ = readableBytes();
                readerIndex_ = 0;
                delete[] data_;
                data_ = newData;
                size_ = newSize;
            }
        }

        // char* begin() { return data_; }
        
    
        char* data_ = nullptr;
        size_t size_;
        size_t writerIndex_ = 0;
        size_t readerIndex_ = 0;
    };
}