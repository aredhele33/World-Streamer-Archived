#pragma once

#define NOMINMAX
#include <Windows.h>

#include <vector>
#include <cstdint>
#include <cassert>

struct AllocatorInfo
{
	uint64_t ID   { 0 };
	bool     free { true };

	std::vector<unsigned char> bytes;
};

// Dumb and slow allocator to avoid allocating bunch of small buffers inside load request (no manual new or delete)
struct BufferAllocator
{
	inline void Init()
	{
		buffers.resize(64);
		for (uint64_t i = 0; i < buffers.size(); ++i)
		{
			buffers[i].ID = i;
			buffers[i].bytes.resize(1024 * 128);
		}
	}

	inline AllocatorInfo* GetFreeBuffer()
	{
		for (uint64_t i = 0; i < buffers.size(); ++i)
		{
			if (buffers[i].free)
			{
				buffers[i].free = false;
				return &buffers[i];
			}
		}

		// Should not happen, but you likely want to know if it happens
		assert(false);
		return nullptr;
	}

	inline void FreeBuffer(AllocatorInfo* info)
	{
		buffers[info->ID].free = true;
	}

	std::vector<AllocatorInfo> buffers;
};


struct CellLoadingResult
{
	uint64_t	   cellID;
	OVERLAPPED	   asynchIORequest;
	AllocatorInfo* allocInfo;
};

struct CellLoadingRequest
{
	uint64_t cellID;
	void(*CompletionCallback)(const CellLoadingResult&);
};

// Internal structures for the streamer
struct CellInfo
{
	uint64_t cellID; // Which cell ?
	uint64_t offset; // Which offset in the binary game file ?
	uint64_t size;	 // What is the size of the cell in bytes ?
};

struct CellInfoEx
{
	uint64_t cellID;
	uint64_t cellX;
	uint64_t cellY;
};