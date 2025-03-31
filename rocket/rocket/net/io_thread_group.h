#ifndef ROCKET_NET_IO_THREAD_GROUP_H
#define ROCKET_NET_IO_THREAD_GROUP_H

#include "rocket/common/log.h"
#include "rocket/net/io_thread.h"
#include <vector>

namespace rocket {
class IOthreadGroup {
public:
	IOthreadGroup(int size);
    ~IOthreadGroup();

    void start();
    void join();
    IOThread* getIOthread();
private:
	int m_size{0};
	std::vector<IOThread*> m_io_thread_groups{};

    int m_index{-1};
};

} // namespace rocket

#endif