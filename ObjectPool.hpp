#pragma once

#include <cstddef>
#include <memory>

template < typename ObjectType >
class ObjectPool
{
public:

  ObjectPool(size_t defaultGrowth = 64) noexcept :
    _defaultGrowthSize{ defaultGrowth == 0 ? 1 : defaultGrowth }
  {
  }

  ObjectPool(const ObjectPool& other) = delete;

  ObjectPool(ObjectPool&& other) = delete;

  ~ObjectPool()
  {
    while (_lastAllocPage != nullptr)
    {
      void* nextPage = NextOf(_lastAllocPage);

      free(_lastAllocPage);
      
      _lastAllocPage = nextPage;
    }
  }

  template < typename ...Args >
  inline ObjectType* Allocate(Args&&... args)
  {
    if (IsEmpty())
    {
      if (!Grow(_defaultGrowthSize))
      {
        return nullptr;
      }
    }

    ObjectType* ret{ static_cast<ObjectType*>(_frontSegment) };
    
    _frontSegment = NextOf(_frontSegment);

    new (ret) ObjectType{ std::forward<Args>(args)... };

    return ret;
  }

  inline void Deallocate(ObjectType* ptr) noexcept
  {
    ptr->~ObjectType();

    NextOf(ptr) = _frontSegment;

    _frontSegment = ptr;
  }

  inline bool IsEmpty() const noexcept
  {
    return _frontSegment == nullptr;
  }

  bool Grow(size_t segmentCount) noexcept
  {
    const size_t newPageSize = sizeof(void*) + (SegmentSize * segmentCount);
    
    char* ptr = static_cast<char*>(malloc(newPageSize));
    if (ptr == nullptr)
    {
      return false;
    }

    NextOf(ptr) = _lastAllocPage;
    _lastAllocPage = ptr;

    char* endOfNewPage = ptr + newPageSize;
    ptr += sizeof(void*); 

    while (ptr != endOfNewPage)
    {
      NextOf(ptr) = _frontSegment;
      _frontSegment = ptr;
      ptr += SegmentSize;
    }

    return true;
  }

private:

  static inline void*& NextOf(void* pageOrSegment) noexcept
  {
    return *static_cast<void**>(pageOrSegment);
  }

  static constexpr size_t SegmentSize = sizeof(ObjectType) < sizeof(void*) ? sizeof(void*) : sizeof(ObjectType);

  void* _lastAllocPage = nullptr;
  void* _frontSegment = nullptr;
  const size_t _defaultGrowthSize;
};