#include "Buffer.h"

Buffer::Buffer(uint64 _size)
{
	data.resize(_size);
}

Buffer::Buffer(const void* _data, uint64 _size)
{
	data.resize(_size);
	memcpy(data.data(), _data, _size);
}

void Buffer::SetData(const void* _data, uint64 _size, uint64 _index)
{
	if (_index + _size > data.size())
		data.resize(_index + _size);
    
	memcpy(data.data() + _index, _data, _size);
}

uint64 Buffer::GetSize() const
{
	return data.size();
}