#ifndef GMB_PAGEALLOC_PAGE_HPP_INCLUDED
#define GMB_PAGEALLOC_PAGE_HPP_INCLUDED
#if _MSCVER
# pragma once
#endif

#include <stdexcept>

#ifdef TRACE_ALLOCS
# include <cstdio>
#endif

#define ALIGN_BY(ptr, obj) \
  reinterpret_cast<obj *>( \
    (reinterpret_cast<uintptr_t>(ptr) + (alignof(obj) - 1)) \
      & ~(alignof(obj)-1))

namespace gmb
{
  template<size_t PageSize>
  class page
  {
    struct page_header
    {
      size_t entry_count;
    };

    struct alloc_item
    {
      uint8_t *p;
      size_t   n;
      uint8_t *offset;
    };

    uint8_t     *data_;
    page_header *header_;
    alloc_item  *table_;

    constexpr static size_t kPageSize = PageSize;

  public:
    explicit page(void *raw, size_t len = kPageSize)
      : data_(reinterpret_cast<uint8_t *>(raw)), header_(nullptr), table_(nullptr)
    {
      if(!data_) {
        throw std::runtime_error("Supplied block is uninitialized!");
      }

      if(len < kPageSize) {
        throw std::runtime_error("Supplied block is smaller than page size!");
      }

      header_ = ALIGN_BY(data_, page_header);

      (*header_).entry_count = 0;

      table_ = reinterpret_cast<alloc_item *>(data_ + kPageSize) - 1;
      table_ = ALIGN_BY(table_, alloc_item);

      if( ((uintptr_t)table_ + 1) > ((uintptr_t)data_ + kPageSize) ) {
        table_--;
      }

      table_->p = reinterpret_cast<uint8_t *>(header_ + 1);
      table_->n = 0;
      table_->offset = table_->p;
    }

    page(page &&other) noexcept
      : data_(other.data_), header_(other.header_), table_(other.table_)
    { 
      other.data_ = nullptr;
      other.header_ = nullptr;
      other.table_ = nullptr;
    }

    page& operator=(page &&rhs) noexcept
    {
      auto tmp = std::move(rhs);
      swap(tmp);
      return *this;
    }

    page(page const &) = delete;
    page & operator=(page const &) = delete;

    void swap(page &rhs) noexcept
    {
      using std::swap;
      swap(data_, rhs.data_);
      swap(header_, rhs.header_);
      swap(table_, rhs.table_);
    }

    void * allocate(size_t n, size_t /*align*/ = 0)
    {
      //  TODO: Allow caller to specify alignment

#ifdef TRACE_ALLOCS
      std::printf("Allocating %i bytes at entry # %i...", n, header_->entry_count + 1);
#endif
      auto *curr = (table_ - (header_->entry_count));
      alloc_item i = { curr->p + curr->n, n, curr->p + curr->n };

      //  If the new allocation overlaps
      //  the resulting alloc table then
      //  we've run out of space...
      if(((uintptr_t)i.p + i.n) > ((uintptr_t)curr - 1)) {
#ifdef TRACE_ALLOCS
        std::printf("failed - no space left in the page\n");
#endif
        return nullptr;
      }

#ifdef TRACE_ALLOCS
      std::printf("succeeded - %i total bytes consumed. %i bytes remaining\n", 
        (i.p + i.n) - table_->p, reinterpret_cast<uint8_t *>(curr - 2) - (i.p + i.n));
#endif

      *(curr - 1) = i;
      header_->entry_count++;
      return i.p;
    }

    void deallocate(void *p)
    {
      //  Explanation:
      //  ***********
      //  The deallocation strategy works on a last in, first out
      //  system.
      //  
      //  The function marks the allocation with offset 'p;
      //  as zero sized. The function then iterates the alloc list, reclaming
      //  any zero byte allocs until it reaches a non-zero alloc.
      //
      //  A side-effect of this is that a late allocation that is long-lived
      //  will prevent the page from reclaming any allocations that
      //  occurred prior.
      //
      //  We can advise that best use results from allocating long-lived
      //  ojects early.

#ifdef TRACE_ALLOCS
      size_t entries = 0;
      auto  *prev = (table_ - header_->entry_count)->p +
        (table_ - header_->entry_count)->n;
#endif
      for(auto *i = table_ - header_->entry_count; i < table_; ++i) {
        if(p == i->offset) {
          i->n = 0;
        }
      }

      for(auto *i = table_ - header_->entry_count; i->n == 0 && i < table_; ++i) {
#ifdef TRACE_ALLOCS
        entries++;
#endif
        header_->entry_count--;
      }

#ifdef TRACE_ALLOCS
      auto *now = (table_ - header_->entry_count)->p + 
        (table_ - header_->entry_count)->n;
      std::printf("%i blocks reclaimed. Freed %i bytes\n", entries, prev - now);
#endif

    }

  };
}

#endif //GMB_PAGEALLOC_PAGE_HPP_INCLUDED
