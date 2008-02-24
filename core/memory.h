/***************************************************************************
 *   Copyright (C) 1998-2008 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of Lux Renderer.                                    *
 *                                                                         *
 *   Lux Renderer is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Lux Renderer is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 *                                                                         *
 *   This project is based on PBRT ; see http://www.pbrt.org               *
 *   Lux Renderer website : http://www.luxrender.org                       *
 ***************************************************************************/

#ifndef LUX_MEMORY_H
#define LUX_MEMORY_H

// memory.h*

#include "lux.h"

namespace lux
{
  void *AllocAligned(size_t size);
  void FreeAligned(void *);
}

/*
template <class T> class ObjectArena {
public:
	// ObjectArena Public Methods
	ObjectArena() {
		nAvailable = 0;
	}
	T *Alloc() {
		if (nAvailable == 0) {
			int nAlloc = max((unsigned long)16,
				(unsigned long)(65536/sizeof(T)));
			mem = (T *)AllocAligned(nAlloc * sizeof(T));
			nAvailable = nAlloc;
			toDelete.push_back(mem);
		}
		--nAvailable;
		return mem++;
	}
	operator T *() {
		return Alloc();
	}
	~ObjectArena() { FreeAll(); }
	void FreeAll() {
		for (u_int i = 0; i < toDelete.size(); ++i)
			FreeAligned(toDelete[i]);
		toDelete.erase(toDelete.begin(), toDelete.end());
		nAvailable = 0;
	}


private:
	// ObjectArena Private Data
	T *mem;
	int nAvailable;
	vector<T *> toDelete;
};*/

class  MemoryArena {
public:
	// MemoryArena Public Methods
	MemoryArena(u_int bs = 32768) {
		blockSize = bs;
		curBlockPos = 0;
		currentBlock = (char *)lux::AllocAligned(blockSize);
	}
	~MemoryArena() {
		lux::FreeAligned(currentBlock);
		for (u_int i = 0; i < usedBlocks.size(); ++i)
			lux::FreeAligned(usedBlocks[i]);
		for (u_int i = 0; i < availableBlocks.size(); ++i)
			lux::FreeAligned(availableBlocks[i]);
	}
	void *Alloc(u_int sz) {
		// Round up _sz_ to minimum machine alignment
		#ifndef LUX_USE_SSE
		sz = ((sz + 7) & (~7));
		#else
		sz = ((sz + 15) & (~15));
		#endif
		if (curBlockPos + sz > blockSize) {
			// Get new block of memory for _MemoryArena_
			usedBlocks.push_back(currentBlock);
			if (availableBlocks.size() && sz <= blockSize) {
				currentBlock = availableBlocks.back();
				availableBlocks.pop_back();
			} else
				currentBlock = (char *)lux::AllocAligned(max(sz, blockSize));
			curBlockPos = 0;
		}
		void *ret = currentBlock + curBlockPos;
		curBlockPos += sz;
		return ret;
	}
	void FreeAll() {
		curBlockPos = 0;
		while (usedBlocks.size()) {
			availableBlocks.push_back(usedBlocks.back());
			usedBlocks.pop_back();
		}
	}
private:
	// MemoryArena Private Data
	u_int curBlockPos, blockSize;
	char *currentBlock;
	vector<char *> usedBlocks, availableBlocks;
};
template<class T, int logBlockSize> class BlockedArray {
public:
	friend class boost::serialization::access;
	// BlockedArray Public Methods
	BlockedArray () {}
	BlockedArray(const BlockedArray &b, const T *d = NULL)
	{
		uRes = b.uRes;
		vRes = b.vRes;
		uBlocks = RoundUp(uRes) >> logBlockSize;
		int nAlloc = RoundUp(uRes) * RoundUp(vRes);
		data = (T *)lux::AllocAligned(nAlloc * sizeof(T));
		for (int i = 0; i < nAlloc; ++i)
			new (&data[i]) T(b.data[i]);
		if (d) {
			for (int v = 0; v < b.vRes; ++v) {
				for (int u = 0; u < b.uRes; ++u)
					(*this)(u, v) = d[v * uRes + u];
			}
		}
	}
	BlockedArray(int nu, int nv, const T *d = NULL) {
		uRes = nu;
		vRes = nv;
		uBlocks = RoundUp(uRes) >> logBlockSize;
		int nAlloc = RoundUp(uRes) * RoundUp(vRes);
		data = (T *)lux::AllocAligned(nAlloc * sizeof(T));
		for (int i = 0; i < nAlloc; ++i)
			new (&data[i]) T();
		if (d) {
			for (int v = 0; v < nv; ++v) {
				for (int u = 0; u < nu; ++u)
					(*this)(u, v) = d[v * uRes + u];
			}
		}
	}
	int BlockSize() const { return 1 << logBlockSize; }
	int RoundUp(int x) const {
		return (x + BlockSize() - 1) & ~(BlockSize() - 1);
	}
	int uSize() const { return uRes; }
	int vSize() const { return vRes; }
	~BlockedArray() {
		for (int i = 0; i < uRes * vRes; ++i)
			data[i].~T();
		lux::FreeAligned(data);
	}
	int Block(int a) const { return a >> logBlockSize; }
	int Offset(int a) const { return (a & (BlockSize() - 1)); }
	T &operator()(int u, int v) {
		int bu = Block(u), bv = Block(v);
		int ou = Offset(u), ov = Offset(v);
		int offset = BlockSize() * BlockSize() * (uBlocks * bv + bu);
		offset += BlockSize() * ov + ou;
		return data[offset];
	}
	const T &operator()(int u, int v) const {
		int bu = Block(u), bv = Block(v);
		int ou = Offset(u), ov = Offset(v);
		int offset = BlockSize() * BlockSize() * (uBlocks * bv + bu);
		offset += BlockSize() * ov + ou;
		return data[offset];
	}
	void GetLinearArray(T *a) const {
		for (int v = 0; v < vRes; ++v) {
			for (int u = 0; u < uRes; ++u)
				*a++ = (*this)(u, v);
		}
	}
	
	
	
private:
	// BlockedArray Private Data
	T *data;
	int uRes, vRes, uBlocks;
	
	template<class Archive> void save(Archive & ar, const unsigned int version) const
	{
		ar & uRes;
		ar & vRes;
		ar & uBlocks;

		int nAlloc = RoundUp(uRes) * RoundUp(vRes);
		for (int i = 0; i < nAlloc; ++i)
			ar & data[i];
	}

	template<class Archive>	void load(Archive & ar, const unsigned int version)
	{
		ar & uRes;
		ar & vRes;
		ar & uBlocks;

		int nAlloc = RoundUp(uRes) * RoundUp(vRes);
		data = (T *)lux::AllocAligned(nAlloc * sizeof(T));
		for (int i = 0; i < nAlloc; ++i)
			ar & data[i];
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()
};

#endif // LUX_MEMORY_H

