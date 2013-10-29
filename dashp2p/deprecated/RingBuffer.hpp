/****************************************************************************
 * RingBuffer.hpp                                                           *
 ****************************************************************************
 * Copyright (C) 2012 Technische Universitaet Berlin                        *
 *                                                                          *
 * Created on: Jun 6, 2012                                                  *
 * Authors: Konstantin Miller <konstantin.miller@tu-berlin.de>              *
 *                                                                          *
 * This program is free software: you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation, either version 3 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.     *
 ****************************************************************************/

#ifndef _RINGBUFFER_HPP_
#define _RINGBUFFER_HPP_

#include <stdio.h>
#include <vector>
using std::vector;
#include <assert.h>

template <class X> class RingBuffer
{
  /* public methods */
public:
  RingBuffer(unsigned initialCapacity, bool ifOverwrite, bool ifAutoResize, bool ifDebug = false);
  /* makes deep copies of all elements */
  RingBuffer(const RingBuffer& from);
  const RingBuffer& operator=(const RingBuffer& from);
  /* destructor calls delete for the elements in the buffer */
  ~RingBuffer();
  void check() const;

  /* calls destructors of all elements. after the call, size() == 0,
   * capacity remains the same. */
  void deleteAllElements();

  void setIfDebug(bool ifDebug) {this->ifDebug = ifDebug;}
  /* the number of elements currently stored. NOT the amount of reserved memory. */
  unsigned size() const;
  /* the amount of reserved memory */
  unsigned capacity() const;
  /* if size() == capacity() */
  bool full() const;
  /* if size() == 0 */
  bool empty() const;
  /* changes the capacity of the RingBuffer */
  void reserve(unsigned newCapacity);
  /* appends a (optionally: deep copy of the) value */
  void append(X& x, bool ifCopy);
  /* returns internal pointer to the n-th element after the first one */
  const X& getFirst(unsigned n = 0) const;
  const X& operator[](const unsigned& n) const;
  /* returns internal pointer to the n-th element before the last one */
  const X& getLast(unsigned n = 0) const;
  /* returns the first element from the buffer and removes it from the buffer.
   * no memory operations (allocate, free or copy) are performed */
  const X* removeFirst();

  /* private methods */
private:
  void print() const;

  /* private variables */
private:
  vector<X*>*       data;
  unsigned          first;
  unsigned          last;
  bool              ifOverwrite;
  bool              ifAutoResize;
  bool              ifDebug;
};





template <class X>
RingBuffer<X>::RingBuffer(unsigned initialCapacity, bool ifOverwrite, bool ifAutoResize, bool ifDebug)
{
  if(ifDebug) {
    printf("enter RingBuffer constructor\n");
    fflush(stdout);
  }

  /* ifOverwrite and ifAutoResize cannot be true at the same time */
  dp2p_assert(!ifOverwrite || !ifAutoResize);

  /* we don't want the buffer to have zero capacity */
  dp2p_assert(initialCapacity > 0);

  data = new vector<X*>(initialCapacity);
  data->resize(data->capacity(), NULL);
  first = data->capacity();
  last = data->capacity();
  this->ifOverwrite = ifOverwrite;
  this->ifAutoResize = ifAutoResize;
  this->ifDebug = ifDebug;
}

template <class X>
RingBuffer<X>::RingBuffer(const RingBuffer& from)
{
  if(from.ifDebug) {
    printf("enter RingBuffer copy constructor\n");
    fflush(stdout);
  }

  data = NULL;
  this->operator=(from);

  if(ifDebug) {
    printf("leaving RingBuffer copy constructor\n");
    fflush(stdout);
  }
}

template <class X>
const RingBuffer<X>& RingBuffer<X>::operator=(const RingBuffer& from)
{
  if(from.ifDebug) {
    printf("enter RingBuffer::operator=\n");
    fflush(stdout);
  }

  if(this == &from)
    return *this;

  if(data != NULL) {
    deleteAllElements();
    delete data;
    data = NULL;
  }

  data = new vector<X*>(from.data->capacity());
  data->resize(data->capacity(), NULL);
  first = from.first;
  last = from.last;
  ifOverwrite = from.ifOverwrite;
  ifAutoResize = from.ifAutoResize;
  ifDebug = from.ifDebug;

  if (empty())
    return *this;

  if (first <= last) {
    for (unsigned i = first; i <= last; ++i)
      data->at(i) = new X(*from.data->at(i));
  } else {
    for (unsigned i = 0; i <= last; ++i)
      data->at(i) = new X(*from.data->at(i));
    for (unsigned i = first; i < data->capacity(); ++i)
      data->at(i) = new X(*from.data->at(i));
  }

  if(ifDebug) {
    printf("leaving RingBuffer::operator=\n");
    fflush(stdout);
  }

  return *this;
}

template <class X>
RingBuffer<X>::~RingBuffer()
{
  if (ifDebug) {
    printf ("enter ~RingBuffer\n");
    fflush (stdout);
  }
  deleteAllElements();
  delete data;
}

template <class X>
unsigned RingBuffer<X>::size() const
{
  if (data == NULL)
    return 0;
  else if (first == data->capacity())
    return 0;
  else if (last >= first)
    return last - first + 1;
  else
    return data->capacity() - (first - last - 1);
}

template <class X>
void RingBuffer<X>::deleteAllElements()
{
  if(ifDebug) {
    printf("enter deleteAllElements\n");
    fflush(stdout);
  }

//  while (!empty()) {
//    delete &getFirst();
//    removeFirst();
//  }

  if (empty())
      return;

  if (first <= last) {
    for (unsigned i = first; i <= last; ++i)
      delete data->at(i);
  } else {
    for (unsigned i = 0; i < last; ++i)
      delete data->at(i);
    for (unsigned i = first; i < data->capacity(); ++i)
      delete data->at(i);
  }
  first = data->capacity();
  last = data->capacity();
}

template <class X>
unsigned RingBuffer<X>::capacity() const
{
  if(ifDebug) {
    printf("enter capacity\n");
    fflush(stdout);
  }
  return data->capacity();
}

template <class X>
bool RingBuffer<X>::full() const
{
  if(ifDebug) {
    printf("enter full\n");
    fflush(stdout);
  }
  return size() == data->capacity();
}

template <class X>
bool RingBuffer<X>::empty() const
{
  if(ifDebug) {
    printf("enter empty\n");
    fflush(stdout);
  }
  return size() == 0;
}

template <class X>
void RingBuffer<X>::reserve(unsigned newCapacity)
{
  if (ifDebug) {
    printf("enter reserve\n");
    fflush(stdout);
  }

  vector<X*>* newData = new vector<X*>(newCapacity);
  newData->resize(newData->capacity());

  /* if the buffer was empty, then we are done */
  if (empty()) {
    delete data;
    data = newData;
    first = newCapacity;
    last = newCapacity;
    return;
  }

  /* copy the data into the new memory */
  if (first <= last) {
    for (unsigned i = 0; i <= last - first; ++i)
      newData->at(i) = data->at(first + i);
  } else {
    for (unsigned i = 0; i < data->capacity() - first; ++i)
      newData->at(i) = data->at(first + i);
    for (unsigned i = 0; i <= last; ++i)
      newData->at(data->capacity() - first + i) = data->at(i);
  }

  delete data;
  data = newData;
  last = size() - 1; // this must be set before the next call, because size() uses this->first
  first = 0;
}

template <class X>
void RingBuffer<X>::append(X& x, bool ifCopy)
{
  if (ifDebug) {
    printf ("enter append. &x = %p. data=%p, first=%u, last=%u, size()=%u, data->size()=%u, data->capacity()=%u\n",
        &x, data, first, last, size(), data->size(), data->capacity());
    printf ("ifOverwrite=%u, ifAutoResize=%u\n", ifOverwrite, ifAutoResize);
    fflush (stdout);
  }

  /* this works only if the buffer never can have zero capacity, which we ensure in the constructor */
  if (empty()) {
    if (ifCopy)
      data->at(0) = new X(x);
    else
      data->at(0) = &x;
    first = last = 0;
    return;
  }

  if (full()) {
    dp2p_assert(ifOverwrite || ifAutoResize);
    if (ifAutoResize)
      reserve(2 * capacity());
  }

  last = (last + 1) % data->capacity();
  if (ifCopy)
    data->at(last) = new X(x);
  else
    data->at(last) = &x;

  if (last == first)
    first = (first + 1) % data->capacity();
}

template <class X>
const X& RingBuffer<X>::getFirst (unsigned n) const
{
  if (ifDebug) {
    printf ("enter getFirst\n");
    fflush(stdout);
  }

  if (n >= size()) {
    printf("n == %u, size() == %u\n", n, size());
    fflush(stdout);
    dp2p_assert(n < size());
  }

  if (data->capacity() - first > n)
    return *data->at(first + n);
  else
    return *data->at(n - data->capacity() + first);
}

template <class X>
const X& RingBuffer<X>::operator[](const unsigned& n) const
{
  return getFirst(n);
}

template <class X>
const X& RingBuffer<X>::getLast(unsigned n) const
{
  if (ifDebug) {
    printf ("enter getLast\n");
    fflush(stdout);
  }

  assert (n < size());

  if (last >= n)
    return *data->at(last - n);
  else
    return *data->at(data->capacity() - (n - last));
}

template <class X>
const X* RingBuffer<X>::removeFirst()
{
  if (ifDebug) {
    printf ("enter removeFirst.\n");
    fflush(stdout);
  }

  assert (!empty());

  const X* retval = data->at(first);
  data->at(first) = NULL;

  /* did we just removed the only element in the buffer? */
  if (first == last) {
    first = data->capacity();
    last = data->capacity();
  } else {
    first = (first + 1) % data->capacity();
  }

  return retval;
}

template <class X>
void RingBuffer<X>::print() const
{
  printf ("========== Dumping ring buffer ==========\n");
  printf ("capacity = %u, num elements = %u, first = %u, last = %u\n",
      capacity(), size(), first, last);
  printf ("=========================================\n");
}

#endif
