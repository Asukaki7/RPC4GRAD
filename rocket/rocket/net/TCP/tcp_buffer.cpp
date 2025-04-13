#include "rocket/net/TCP/tcp_buffer.h"
#include "rocket/common/log.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <vector>

namespace rocket {

TcpBuffer::TcpBuffer(int size)
    : m_size(size) {
	m_buffer.resize(size);
}

TcpBuffer::~TcpBuffer() {}

// 返回可读字节数
int TcpBuffer::readAble() { return m_write_index - m_read_index; }

// 返回可写字节数
int TcpBuffer::writeAble() { return m_size - m_write_index; }

int TcpBuffer::getReadIndex() { return m_read_index; }

int TcpBuffer::getWriteIndex() { return m_write_index; }

void TcpBuffer::writeToBuffer(const char* buf, int size) {
	if (size > writeAble()) {
		// 调整buffer 大小 扩容
		int new_size = (int)(1.5 * (m_size + m_write_index));
		resizeBuffer(new_size);
	}
	memcpy(&m_buffer[m_write_index], buf, size);
	m_write_index += size;
}

void TcpBuffer::readFromBuffer(std::vector<char>& read_buf, int size) {
	if (readAble() == 0) {
		return;
	}
	int read_size = readAble() > size ? size : readAble();

	std::vector<char> tmp(read_size);
	memcpy(&tmp[0], &m_buffer[m_read_index], read_size);

	read_buf.swap(tmp);
	m_read_index += read_size;

	adjustBuffer();
}

void TcpBuffer::resizeBuffer(int new_size) {
	std::vector<char> tmp(new_size);
	int count = std::min(new_size, readAble());

	memcpy(&tmp[0], &m_buffer[m_read_index], count);

	m_read_index = 0;
	m_write_index = count;
	m_size = new_size;

	m_buffer.swap(tmp);
}

void TcpBuffer::adjustBuffer() {
	if (m_read_index < static_cast<int>(m_buffer.size() / 3)) {
		return;
	}

	const int count = readAble();
	if (count > 0) {
		// 使用 memmove 处理可能重叠的内存区域
		memmove(&m_buffer[0], &m_buffer[m_read_index], count);
	}

	// 重置读写索引
	m_read_index = 0;
	m_write_index = count;
}

void TcpBuffer::moveReadIndex(int size) {
	size_t new_size = m_read_index + size;
	if (new_size >= m_buffer.size()) {
		ERRORLOG("moveReadIndex error, invalid size %d, old_read_index %d, "
		         "buffer size %d",
		         size, m_read_index, m_buffer.size());
		return;
	}
	m_read_index = new_size;
	adjustBuffer();
}

void TcpBuffer::moveWriteIndex(int size) {
	size_t new_size = m_write_index + size;
	if (new_size >= m_buffer.size()) {
		ERRORLOG("moveWriteIndex error, invalid size %d, old_write_index %d, "
		         "buffer size %d",
		         size, m_write_index, m_buffer.size());
		return;
	}
	m_write_index = new_size;
	adjustBuffer();
}

} // namespace rocket