/***************************************************************************
 *   Copyright (C) 1998-2009 by authors (see AUTHORS.txt )                 *
 *                                                                         *
 *   This file is part of LuxRender.                                       *
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

#include <boost/serialization/split_member.hpp>

namespace lux
{
// Memory Allocation Functions
#if !defined(LUX_ALIGNMENT)
#ifdef LUX_USE_SSE
#define LUX_ALIGNMENT 16
#endif
#endif
#ifndef L1_CACHE_LINE_SIZE
#define L1_CACHE_LINE_SIZE 64
#endif
template<class T> inline T *AllocAligned(size_t size, std::size_t N = L1_CACHE_LINE_SIZE)
{
	return static_cast<T *>(memalign(N, size * sizeof(T)));
}
template<class T> inline void FreeAligned(T *ptr)
{
#if defined(WIN32) && !defined(__CYGWIN__) // NOBOOK
	_aligned_free(ptr);
#else // NOBOOK
	free(ptr);
#endif // NOBOOK
}

template <typename T, std::size_t N = 16> class AlignedAllocator
{
public:
	typedef T value_type;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

	typedef T *pointer;
	typedef const T *const_pointer;

	typedef T &reference;
	typedef const T &const_reference;

public:
	inline AlignedAllocator() throw() { }

	template <typename T2> inline AlignedAllocator(const AlignedAllocator<T2, N> &) throw () { }

	inline ~AlignedAllocator() throw() { }

	inline pointer adress(reference r) { return &r; }

	inline const_pointer adress(const_reference r) const { return &r; }

	inline pointer allocate(size_type n)
	{
		return AllocAligned<value_type>(n, N);
	}

	inline void deallocate(pointer p, size_type)
	{
		FreeAligned(p);
	}

	inline void construct(pointer p, const value_type &wert)
	{
		new (p) value_type(wert);
	}

	inline void destroy(pointer p)
	{
		p->~value_type();
	}

	inline size_type max_size() const throw()
	{
		return size_type(-1) / sizeof(value_type);
	}

	template <typename T2> struct rebind
	{
		typedef AlignedAllocator<T2, N> other;
	};
};

#define P_CLASS_ATTR __attribute__
#define P_CLASS_ATTR __attribute__

#if defined(WIN32) && !defined(__CYGWIN__) // NOBOOK
class __declspec(align(16)) Aligned16 {
#else // NOBOOK
class Aligned16 {
#endif // NOBOOK
public:

	/*
	Aligned16(){
		if(((int)this & 15) != 0){
			printf("bad alloc\n");
			assert(0);
		}
	}
	*/

	void *operator new(size_t s) { return AllocAligned<char>(s, 16); }

	void *operator new (size_t s, void *q) { return q; }

	void operator delete(void *p) { FreeAligned(p); }
#if defined(WIN32) && !defined(__CYGWIN__) // NOBOOK
} ;
#else // NOBOOK
} __attribute__ ((aligned(16)));
#endif // NOBOOK


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
	MemoryArena(size_t bs = 32768) {
		blockSize = bs;
		curBlockPos = 0;
		currentBlock = lux::AllocAligned<char>(blockSize);
	}
	~MemoryArena() {
		lux::FreeAligned(currentBlock);
		for (size_t i = 0; i < usedBlocks.size(); ++i)
			lux::FreeAligned(usedBlocks[i]);
		for (size_t i = 0; i < availableBlocks.size(); ++i)
			lux::FreeAligned(availableBlocks[i]);
	}
	void *Alloc(size_t sz) {
		// Round up _sz_ to minimum machine alignment
		#if defined(LUX_ALIGNMENT)
		sz = ((sz + (LUX_ALIGNMENT-1)) & (~(LUX_ALIGNMENT-1)));
		#else
		sz = ((sz + 7) & (~7));
		#endif
		if (curBlockPos + sz > blockSize) {
			// Get new block of memory for _MemoryArena_
			usedBlocks.push_back(currentBlock);
			if (availableBlocks.size() > 0 && sz <= blockSize) {
				currentBlock = availableBlocks.back();
				availableBlocks.pop_back();
			} else
				currentBlock = lux::AllocAligned<char>(max(sz, blockSize));
			curBlockPos = 0;
		}
		void *ret = currentBlock + curBlockPos;
		curBlockPos += sz;
		return ret;
	}
	void FreeAll() {
		curBlockPos = 0;
		while (usedBlocks.size() > 0) {
			availableBlocks.push_back(usedBlocks.back());
			usedBlocks.pop_back();
		}
	}
private:
	// MemoryArena Private Data
	size_t curBlockPos, blockSize;
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
		data = lux::AllocAligned<T>(nAlloc);
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
		data = lux::AllocAligned<T>(nAlloc);
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
		data = lux::AllocAligned<T>(nAlloc);
		for (int i = 0; i < nAlloc; ++i)
			ar & data[i];
	}
	BOOST_SERIALIZATION_SPLIT_MEMBER()
};

#endif // LUX_MEMORY_H

