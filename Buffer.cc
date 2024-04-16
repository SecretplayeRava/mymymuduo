#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

/**
 * 从fd上读取数据  Poller工作在LT模式
 * Buffer缓冲区是有大小的！ 但是从fd上读数据的时候，却不知道tcp数据最终的大小
 */ 
ssize_t Buffer::readFd(int fd, int* saveErrno)
{
    char extrabuf[65536] = {0}; // 栈上的内存空间  64K  //栈临时空间，出了作用域会自动回收 一次最少读65536/1024 = 64K
    
    struct iovec vec[2];
    
    const size_t writable = writableBytes(); // 这是Buffer底层缓冲区剩余的可写空间大小
    vec[0].iov_base = begin() + writerIndex_;  //指向可写区域
    vec[0].iov_len = writable;   // 可写区域的可写长度

    vec[1].iov_base = extrabuf;   // 栈区临时空间
    vec[1].iov_len = sizeof extrabuf; 
    
    const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;  // 判断可以写的区域是不是>64K,需不需要额外空间
    const ssize_t n = ::readv(fd, vec, iovcnt);  // 调用readvd读取fd数据写入buffer
    if (n < 0)
    {
        *saveErrno = errno;
    }
    else if (n <= writable) // Buffer的可写缓冲区已经够存储读出来的数据了  
    {
        writerIndex_ += n;
    }
    else // extrabuf里面也写入了数据 
    {
        writerIndex_ = buffer_.size();  //写满了
        append(extrabuf, n - writable);  // writerIndex_开始写 n - writable大小的数据
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int* saveErrno)
{
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}