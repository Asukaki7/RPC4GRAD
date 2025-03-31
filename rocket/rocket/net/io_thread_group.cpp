#include "rocket/net/io_thread_group.h"
#include "io_thread.h"

namespace rocket {

IOthreadGroup::IOthreadGroup(int size)
    : m_size(size) {
	m_io_thread_groups.resize(size);
	for (int i = 0; i < size; i++) {
		m_io_thread_groups[i] = new IOThread();
	}
}

IOthreadGroup::~IOthreadGroup() {}

void IOthreadGroup::start() {
	for (const auto& IOthread : m_io_thread_groups) {
		IOthread->start();
	}
}

void IOthreadGroup::join() {
	for (const auto& IOthread : m_io_thread_groups) {
		IOthread->join();
	}
}

IOThread* IOthreadGroup::getIOthread() {
	if (m_index == m_io_thread_groups.size() || m_index == -1) {
		m_index = 0;
	}
	return m_io_thread_groups[m_index];
}
} // namespace rocket