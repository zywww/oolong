#pragma once
#include <string>
#include <cassert>
#include <algorithm>

namespace oolong
{
///     +------------------+------------------+
///     |  readable bytes  |  writable bytes  |
///     |     (CONTENT)    |                  |
///     +------------------+------------------+
///     |                  |                  |
/// readerIndex   <=   writerIndex    <=     size
    class Buffer
    {
        static const size_t kInitSize = 1024;
    public:
        Buffer(size_t initSize = kInitSize) :
            size_(initSize)
        {
            data_ = new char[size_];
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
                char* newData = new char[newSize];
                std::copy(data_+readerIndex_, data_+writerIndex_, newData);
                writerIndex_ = readableBytes();
                readerIndex_ = 0;
                delete data_;
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