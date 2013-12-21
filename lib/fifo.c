/*
 * A generic kernel FIFO implementation.
 *
 * Copyright (C) 2009 Stefani Seibold <stefani@seibold.net>
 * Copyright (C) 2004 Stelian Pop <stelian@popies.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fifo.h>


static void _kfifo_init(struct kfifo *fifo, void *buffer,
		unsigned int size)
{
	fifo->buffer = buffer;
	fifo->size = size;

	kfifo_reset(fifo);
}

/**
 * kfifo_init - initialize a FIFO using a preallocated buffer
 * @fifo: the fifo to assign the buffer
 * @buffer: the preallocated buffer to be used.
 * @size: the size of the internal buffer, this has to be a power of 2.
 *
 */
void kfifo_init(struct kfifo *fifo, void *buffer, unsigned int size)
{
	if(!is_power_of_2(size))
		size = roundup_pow_of_two(size);

	_kfifo_init(fifo, buffer, size);
}

/**
 * kfifo_alloc - allocates a new FIFO internal buffer
 * @fifo: the fifo to assign then new buffer
 * @size: the size of the buffer to be allocated, this have to be a power of 2.
 * @gfp_mask: get_free_pages mask, passed to kmalloc()
 *
 * This function dynamically allocates a new fifo internal buffer
 *
 * The size will be rounded-up to a power of 2.
 * The buffer will be release with kfifo_free().
 * Return 0 if no error, otherwise the an error code
 */
int kfifo_alloc(struct kfifo *fifo, unsigned int size)
{
	unsigned char *buffer;

	if(!is_power_of_2(size))
		size = roundup_pow_of_two(size);

	buffer = malloc(size);
	if (!buffer) {
		_kfifo_init(fifo, NULL, 0);
		return -1;
	}

	_kfifo_init(fifo, buffer, size);

	return 0;
}


/**
 * kfifo_free - frees the FIFO internal buffer
 * @fifo: the fifo to be freed.
 */
void kfifo_free(struct kfifo *fifo)
{
	free(fifo->buffer);
	_kfifo_init(fifo, NULL, 0);
}

/**
 * kfifo_skip - skip output data
 * @fifo: the fifo to be used.
 * @len: number of bytes to skip
 */
void kfifo_skip(struct kfifo *fifo, unsigned int len)
{
	if (len < kfifo_len(fifo)) {
		__kfifo_add_out(fifo, len);
		return;
	}
	kfifo_reset_out(fifo);
}

static inline void __kfifo_in_data(struct kfifo *fifo,
		const void *from, unsigned int len, unsigned int off)
{
	unsigned int l;

	/*
	 * Ensure that we sample the fifo->out index -before- we
	 * start putting bytes into the kfifo.
	 */

	off = __kfifo_off(fifo, fifo->in + off);

	/* first put the data starting from fifo->in to buffer end */
	l = min(len, fifo->size - off);
	memcpy(fifo->buffer + off, from, l);

	/* then put the rest (if any) at the beginning of the buffer */
	memcpy(fifo->buffer, from + l, len - l);
}

static inline void __kfifo_out_data(struct kfifo *fifo,
		void *to, unsigned int len, unsigned int off)
{
	unsigned int l;

	/*
	 * Ensure that we sample the fifo->in index -before- we
	 * start removing bytes from the kfifo.
	 */

	off = __kfifo_off(fifo, fifo->out + off);

	/* first get the data from fifo->out until the end of the buffer */
	l = min(len, fifo->size - off);
	memcpy(to, fifo->buffer + off, l);

	/* then get the rest (if any) from the beginning of the buffer */
	memcpy(to + l, fifo->buffer, len - l);
}


unsigned int __kfifo_in_n(struct kfifo *fifo,
	const void *from, unsigned int len, unsigned int recsize)
{
	if (kfifo_avail(fifo) < len + recsize)
		return len + 1;

	__kfifo_in_data(fifo, from, len, recsize);
	return 0;
}

/**
 * kfifo_in - puts some data into the FIFO
 * @fifo: the fifo to be used.
 * @from: the data to be added.
 * @len: the length of the data to be added.
 *
 * This function copies at most @len bytes from the @from buffer into
 * the FIFO depending on the free space, and returns the number of
 * bytes copied.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int kfifo_in(struct kfifo *fifo, const void *from,
				unsigned int len)
{
	len = min(kfifo_avail(fifo), len);

	__kfifo_in_data(fifo, from, len, 0);
	__kfifo_add_in(fifo, len);
	return len;
}


unsigned int __kfifo_in_generic(struct kfifo *fifo,
	const void *from, unsigned int len, unsigned int recsize)
{
	return __kfifo_in_rec(fifo, from, len, recsize);
}


unsigned int __kfifo_out_n(struct kfifo *fifo,
	void *to, unsigned int len, unsigned int recsize)
{
	if (kfifo_len(fifo) < len + recsize)
		return len;

	__kfifo_out_data(fifo, to, len, recsize);
	__kfifo_add_out(fifo, len + recsize);
	return 0;
}


/**
 * kfifo_out - gets some data from the FIFO
 * @fifo: the fifo to be used.
 * @to: where the data must be copied.
 * @len: the size of the destination buffer.
 *
 * This function copies at most @len bytes from the FIFO into the
 * @to buffer and returns the number of copied bytes.
 *
 * Note that with only one concurrent reader and one concurrent
 * writer, you don't need extra locking to use these functions.
 */
unsigned int kfifo_out(struct kfifo *fifo, void *to, unsigned int len)
{
	len = min(kfifo_len(fifo), len);

	__kfifo_out_data(fifo, to, len, 0);
	__kfifo_add_out(fifo, len);

	return len;
}

/**
 * kfifo_out_peek - copy some data from the FIFO, but do not remove it
 * @fifo: the fifo to be used.
 * @to: where the data must be copied.
 * @len: the size of the destination buffer.
 * @offset: offset into the fifo
 *
 * This function copies at most @len bytes at @offset from the FIFO
 * into the @to buffer and returns the number of copied bytes.
 * The data is not removed from the FIFO.
 */
unsigned int kfifo_out_peek(struct kfifo *fifo, void *to, unsigned int len,
			    unsigned offset)
{
	len = min(kfifo_len(fifo), len + offset);

	__kfifo_out_data(fifo, to, len, offset);
	return len;
}


unsigned int __kfifo_out_generic(struct kfifo *fifo,
	void *to, unsigned int len, unsigned int recsize,
	unsigned int *total)
{
	return __kfifo_out_rec(fifo, to, len, recsize, total);
}



unsigned int __kfifo_peek_generic(struct kfifo *fifo, unsigned int recsize)
{
	if (recsize == 0)
		return kfifo_avail(fifo);

	return __kfifo_peek_n(fifo, recsize);
}


void __kfifo_skip_generic(struct kfifo *fifo, unsigned int recsize)
{
	__kfifo_skip_rec(fifo, recsize);
}



