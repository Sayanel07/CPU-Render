#pragma once
#include "../../common/types.h"
#include <vector>

struct Buffer
{
	Buffer() = default;
	Buffer(uint64 _size);
	Buffer(const void* _data, uint64 _size);

	void SetData(const void* _data, uint64 _size, uint64 _index = 0);

	template<typename T = void>
	T* GetData() const;

	uint64 GetSize() const;

private:
	std::vector<char> data = {};
};

template<typename T>
inline T* Buffer::GetData() const
{
	return (T*)data.data();
}