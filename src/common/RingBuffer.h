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

#ifndef VBAM_RINGBUFFER_H_
#define VBAM_RINGBUFFER_H_

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

struct ring_buffer;

/*!
 * Creates a new ring buffer with capacity size
 */
struct ring_buffer *ring_buffer_new(unsigned int size);

/*!
 * Frees the resources allocated for the ring buffer
 */
void ring_buffer_free(struct ring_buffer *buf);

/*!
 * Resets the ring buffer, all data inside the buffer is lost
 */
void ring_buffer_reset(struct ring_buffer *buf);

/*!
 * Writes data of size len into the ring buffer buf.  Returns -1 if the
 * write failed or the number of bytes written
 */
int ring_buffer_write(struct ring_buffer *buf, const void *data,
			unsigned int len);

/*!
 * Returns the number of free bytes available in the buffer
 */
int ring_buffer_avail(struct ring_buffer *buf);

/*!
 * Reads data from the ring buffer buf into memory region pointed to by data.
 * A maximum of len bytes will be read.  Returns -1 if the read failed or
 * the number of bytes read
 */
int ring_buffer_read(struct ring_buffer *buf, void *data,
			unsigned int len);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // VBAM_RINGBUFFER_H_
