/*
 *
 *  AT chat library with GLib integration
 *
 *  Copyright (C) 2008-2011  Intel Corporation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <string.h>

#include <glib.h>

#include "RingBuffer.h"

#define MAX_SIZE 262144

struct ring_buffer {
	unsigned char *buffer;
	unsigned int size;
	unsigned int mask;
	unsigned int in;
	unsigned int out;
};

struct ring_buffer *ring_buffer_new(unsigned int size)
{
	unsigned int real_size = 1;
	struct ring_buffer *buffer;

	/* Find the next power of two for size */
	while (real_size < size && real_size < MAX_SIZE)
		real_size = real_size << 1;

	if (real_size > MAX_SIZE)
		return NULL;

	buffer = g_slice_new(struct ring_buffer);
	if (buffer == NULL)
		return NULL;

	buffer->buffer = g_slice_alloc(real_size);
	if (buffer->buffer == NULL) {
		g_free(buffer);
		return NULL;
	}

	buffer->size = real_size;
	buffer->mask = real_size - 1;
	buffer->in = 0;
	buffer->out = 0;

	return buffer;
}

int ring_buffer_write(struct ring_buffer *buf, const void *data,
			unsigned int len)
{
	unsigned int end;
	unsigned int offset;
	const unsigned char *d = data; /* Needed to satisfy non-gcc compilers */

	/* Determine how much we can actually write */
	len = MIN(len, buf->size - buf->in + buf->out);

	/* Determine how much to write before wrapping */
	offset = buf->in & buf->mask;
	end = MIN(len, buf->size - offset);
	memcpy(buf->buffer+offset, d, end);

	/* Now put the remainder on the beginning of the buffer */
	memcpy(buf->buffer, d + end, len - end);

	buf->in += len;

	return len;
}

int ring_buffer_read(struct ring_buffer *buf, void *data, unsigned int len)
{
	unsigned int end;
	unsigned int offset;
	unsigned char *d = data;

	len = MIN(len, buf->in - buf->out);

	/* Grab data from buffer starting at offset until the end */
	offset = buf->out & buf->mask;
	end = MIN(len, buf->size - offset);
	memcpy(d, buf->buffer + offset, end);

	/* Now grab remainder from the beginning */
	memcpy(d + end, buf->buffer, len - end);

	buf->out += len;

	if (buf->out == buf->in)
		buf->out = buf->in = 0;

	return len;
}

void ring_buffer_reset(struct ring_buffer *buf)
{
	if (buf == NULL)
		return;

	buf->in = 0;
	buf->out = 0;
}

int ring_buffer_avail(struct ring_buffer *buf)
{
	if (buf == NULL)
		return -1;

	return buf->size - buf->in + buf->out;
}

void ring_buffer_free(struct ring_buffer *buf)
{
	if (buf == NULL)
		return;

	g_slice_free1(buf->size, buf->buffer);
	g_slice_free1(sizeof(struct ring_buffer), buf);
}
