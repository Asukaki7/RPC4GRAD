#ifndef ROCKET_NET_TCP_TCP_BUFFER_H
#define ROCKET_NET_TCP_TCP_BUFFER_H

#include <memory>
#include <vector>

namespace rocket {

class TcpBuffer {

public:
	typedef std::shared_ptr<TcpBuffer> s_ptr;
	TcpBuffer(int size);
	~TcpBuffer();

	// 返回可读字节数
	int readAble();

	// 返回可写字节数
	int writeAble();

	int getReadIndex();
	int getWriteIndex();

	void writeToBuffer(const char* buf, int size);

	void readFromBuffer(std::vector<char>& read_buf, int size);

	void resizeBuffer(int new_size);

	void adjustBuffer();

	void moveReadIndex(int size);

	void moveWriteIndex(int size);

private:
	int m_read_index{};
	int m_write_index{};
	int m_size{};

public:
	std::vector<char> m_buffer{};
};

} // namespace rocket

#endif