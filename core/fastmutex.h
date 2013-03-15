/***************************************************************************
 *   Copyright (C) 1998-2013 by authors (see AUTHORS.txt)                  *
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
 *   Lux Renderer website : http://www.luxrender.net                       *
 ***************************************************************************/

#ifndef LUX_FASTMUTEX_H
#define LUX_FASTMUTEX_H
// fastmutex.h*
// Fast mutex class for high-speed, short-duration locking

#if defined(__APPLE__)
#include <libkern/OSAtomic.h>
#elif defined(__WIN32__)
#include <windows.h>
#else
#ifdef WIN32
#undef min
#undef max
#endif
#include <boost/thread/mutex.hpp>
#endif

namespace lux
{
#if defined(__APPLE__)
class fast_mutex
{
private:
    OSSpinLock sl_;

    fast_mutex(fast_mutex const &);
    fast_mutex & operator=(fast_mutex const &);

public:

    fast_mutex()
    {
		sl_ = OS_SPINLOCK_INIT;
    }

    ~fast_mutex()
    {        		
    }

	bool try_lock()
	{
		return OSSpinLockTry(&sl_);
	}

	void lock()
	{
		OSSpinLockLock(&sl_);
	}

	void unlock()
	{
		OSSpinLockUnlock(&sl_);
	}

    class scoped_lock;
    friend class scoped_lock;

    class scoped_lock
    {
    private:

        fast_mutex & m_;
		bool has_lock;

        scoped_lock(scoped_lock const &);
        scoped_lock & operator=(scoped_lock const &);

    public:

        scoped_lock(fast_mutex & m): m_(m)
        {
            m_.lock();
			has_lock = true;
        }

        ~scoped_lock()
        {
			if (has_lock)
				m_.unlock();
        }

		void unlock()
		{
			if (has_lock)
				m_.unlock();
			has_lock = false;
		}
    };
};
#elif defined(WIN32)
class fast_mutex
{
private:

    CRITICAL_SECTION cs_;

    fast_mutex(fast_mutex const &);
    fast_mutex & operator=(fast_mutex const &);

public:

    fast_mutex()
    {
		InitializeCriticalSection(&cs_);
    }

    ~fast_mutex()
    {        
		DeleteCriticalSection(&cs_);
    }

	bool try_lock()
	{
		return TryEnterCriticalSection(&cs_) == TRUE;
	}

	void lock()
	{
		EnterCriticalSection(&cs_);
	}

	void unlock()
	{
		LeaveCriticalSection(&cs_);
	}

    class scoped_lock;
    friend class scoped_lock;

    class scoped_lock
    {
    private:

        fast_mutex & m_;
		bool has_lock;

        scoped_lock(scoped_lock const &);
        scoped_lock & operator=(scoped_lock const &);

    public:

        scoped_lock(fast_mutex & m): m_(m)
        {
            m_.lock();
			has_lock = true;
        }

        ~scoped_lock()
        {
			if (has_lock)
				m_.unlock();
        }

		void unlock()
		{
			if (has_lock)
				m_.unlock();
			has_lock = false;
		}
    };
};
#else
typedef boost::mutex fast_mutex;
#endif

}//namespace lux

#endif // LUX_FASTMUTEX_H
