/*
 * This file is part of libsidplayfp, a SID player engine.
 *
 * Copyright (C) Michael Schwendt <mschwendt@yahoo.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <assert.h>
#include <stdint.h>

template <class T> class Buffer_sidtt
{
 public:
	Buffer_sidtt() : dummy(0)
	{
		kill();
	}

	Buffer_sidtt(T* inBuf, uint_least32_t inLen) : dummy(0)
	{
		kill();
		if (inBuf!=0 && inLen!=0)
		{
			buf = inBuf;
			bufLen = inLen;
		}
	}

	bool assign(T* newBuf, uint_least32_t newLen)
	{
		erase();
		buf = newBuf;
		bufLen = newLen;
		return (buf!=0);
	}

	T* get() const  { return buf; }
	uint_least32_t len() const  { return bufLen; }

	T* xferPtr()
	{
		T* tmpBuf = buf;
		buf = 0;
		return tmpBuf;
	}

	uint_least32_t xferLen()
	{
		const uint_least32_t tmpBufLen = bufLen;
		bufLen = 0;
		return tmpBufLen;
	}

	T& operator[](uint_least32_t index)
	{
		return (index < bufLen) ? buf[index] : dummy;
	}

	bool isEmpty() const  { return (buf==0); }

	void erase()
	{
		if (buf!=0 && bufLen!=0)
		{
#ifndef SID_HAVE_BAD_COMPILER
			delete[] buf;
#else
			delete[] (void *) buf;
#endif
		}
		kill();
	}

	~Buffer_sidtt()
	{
		erase();
	}

 private:
	T* buf;
	uint_least32_t bufLen;
	T dummy;

	void kill()
	{
		buf = 0;
		bufLen = 0;
	}

 private:	// prevent copying
	// SAW - Need function body so code can be fully instatiated
	// for exporting from dll.  Use asserts in debug mode as these
	// should not be used.
	Buffer_sidtt(const Buffer_sidtt&) : dummy (0) { assert(0); }
	Buffer_sidtt& operator=(Buffer_sidtt& b)
	{
		assert(0);
		return b;
	}
};

#endif  /* BUFFER_H */
