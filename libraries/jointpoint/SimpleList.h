// SimpleList.h

#ifndef _SIMPLELIST_h
#define _SIMPLELIST_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#ifndef NULL
#define NULL 0
#endif

template<typename T>
class SimpleList
{
public:
    typedef T* iterator;

    SimpleList()
    {
        _internalArray = NULL;
        _endPosition = 0;
        _allocBlocks = 0;
        _preAllocBlocks = 0;
    }

    ~SimpleList()
    {
        delete[] _internalArray;
        _internalArray = NULL;
        _endPosition = 0;
        _allocBlocks = 0;
        _preAllocBlocks = 0;
    }

    void push_back(T item)
    {
        if (_endPosition == _allocBlocks)
            AllocOneBlock(false);

        _internalArray[_endPosition] = item;
        ++_endPosition;
    }

    void push_front(T item)
    {
        if (_endPosition == _allocBlocks)
            AllocOneBlock(true);
        else
        {
            for (int i = _endPosition; i > 0; --i)
                _internalArray[i] = _internalArray[i - 1];
        }

        _internalArray[0] = item;
        ++_endPosition;
    }

    void pop_back()
    {
        if (_endPosition == 0)
            return;

        --_endPosition;

        if (_allocBlocks > _preAllocBlocks)
            DeAllocOneBlock(false);
    }

    void pop_front()
    {
        if (_endPosition == 0)
            return;

        --_endPosition;

        if (_allocBlocks > _preAllocBlocks)
            DeAllocOneBlock(true);
        else
        {
            for (int i = 0; i < _endPosition; ++i)
                _internalArray[i] = _internalArray[i + 1];
        }
    }

    iterator erase(iterator position)
    {
        int offSet = int(position - _internalArray);

        if (offSet == _endPosition - 1) // Last item.
        {
            pop_back();
            return end();
        }

        --_endPosition;

        if (_allocBlocks > _preAllocBlocks)
        {
            --_allocBlocks;
            T* newArray = new T[_allocBlocks];

            for (int i = 0; i < _endPosition; ++i)
            {
                if (i >= offSet)
                    newArray[i] = _internalArray[i + 1];
                else
                    newArray[i] = _internalArray[i];
            }

            delete[] _internalArray;
            _internalArray = newArray;
        }
        else
        {
            for (int i = offSet; i < _endPosition; ++i)
                _internalArray[i] = _internalArray[i + 1];
        }

        return _internalArray + offSet;
    }

    void reserve(int size)
    {
        if (size == 0 || size < _allocBlocks)
            return;

        _allocBlocks = size;
        _preAllocBlocks = size;

        T* newArray = new T[_allocBlocks];

        for (int i = 0; i < _endPosition; ++i)
            newArray[i] = _internalArray[i];

        delete[] _internalArray;
        _internalArray = newArray;
    }

    void clear()
    {
        if (_allocBlocks > _preAllocBlocks)
        {
            _allocBlocks = _preAllocBlocks;

            T* newArray = NULL;

            if (_allocBlocks > 0)
                newArray = new T[_allocBlocks];

            delete[] _internalArray;
            _internalArray = newArray;
        }

        _endPosition = 0;
    }

    void shrink_to_fit()
    {
        _preAllocBlocks = _endPosition;
        _allocBlocks = _endPosition;

        T* newArray = NULL;

        if (_allocBlocks > 0)
            newArray = new T[_allocBlocks];

        for (int i = 0; i < _endPosition; ++i)
            newArray[i] = _internalArray[i];

        delete[] _internalArray;
        _internalArray = newArray;
    }

    void sort(bool (*comp)(T, T))
    {
      T* ar = _internalArray;
      uint8_t n = _endPosition;
      uint8_t i, j, gap, swapped = 1;
      T temp;

      gap = n;
      while (gap > 1 || swapped == 1)
      {
        gap = gap * 10 / 13;
        if (gap == 9 || gap == 10) gap = 11;
        if (gap < 1) gap = 1;
        swapped = 0;
        for (i = 0, j = gap; j < n; i++, j++)
        {
          if (comp(ar[i], ar[j]))
          {
            temp = ar[i];
            ar[i] = ar[j];
            ar[j] = temp;
            swapped = 1;
          }
        }
      }
    }

    inline iterator begin() const { return _internalArray; }
    inline iterator end() const { return _internalArray + _endPosition; }

    inline bool empty() { return (_endPosition == 0); }
    inline unsigned int size() { return _endPosition; }
    inline unsigned int capacity() { return _allocBlocks; }

private:

    void AllocOneBlock(bool shiftItems)
    {
        ++_allocBlocks;
        T* newArray = new T[_allocBlocks];

        for (int i = 0; i < _endPosition; ++i)
            newArray[shiftItems ? (i + 1) : i] = _internalArray[i];

        delete[] _internalArray;
        _internalArray = newArray;
    }

    void DeAllocOneBlock(bool shiftItems)
    {
        --_allocBlocks;

        if (_allocBlocks == 0)
        {
            delete[] _internalArray;
            _internalArray = NULL;
            return;
        }

        T* newArray = new T[_allocBlocks];

        for (int i = 0; i < _endPosition; ++i)
            newArray[i] = _internalArray[shiftItems ? (i + 1) : i];

        delete[] _internalArray;
        _internalArray = newArray;
    }

private:

    T* _internalArray;
    int _endPosition;
    int _allocBlocks;
    int _preAllocBlocks;
};

#endif
