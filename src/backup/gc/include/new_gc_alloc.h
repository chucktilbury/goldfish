/*
 * Copyright (c) 1996-1998 by Silicon Graphics.  All rights reserved.
 *
 * THIS MATERIAL IS PROVIDED AS IS, WITH ABSOLUTELY NO WARRANTY EXPRESSED
 * OR IMPLIED.  ANY USE IS AT YOUR OWN RISK.
 *
 * Permission is hereby granted to use or copy this program
 * for any purpose,  provided the above notices are retained on all copies.
 * Permission to modify the code and to distribute modified code is granted,
 * provided the above notices are retained, and a notice that the code was
 * modified is included with the above copyright notice.
 */

//
// This is a revision of gc_allocator.h for SGI STL versions > 3.0.
// Unlike earlier versions, it supplements the standard (STL) alloc.h
// instead of replacing it.
//
// This is sloppy about variable names used in header files.
// It also doesn't yet understand the new header file names or
// namespaces.
//
// This assumes the collector has been compiled with -DGC_ATOMIC_UNCOLLECTABLE.
// The user should also consider -DREDIRECT_MALLOC=GC_uncollectable_malloc,
// to ensure that object allocated through malloc are traced.
//
// Some of this could be faster in the explicit deallocation case.
// In particular, we spend too much time clearing objects on the
// free lists.  That could be avoided.
//
// This uses template classes with static members, and hence does not work
// with g++ 2.7.2 and earlier.
//
// Unlike its predecessor, this one simply defines
//      gc_alloc
//      single_client_gc_alloc
//      traceable_alloc
//      single_client_traceable_alloc
//
// It does not redefine alloc.  Nor does it change the default allocator,
// though the user may wish to do so.  (The argument against changing
// the default allocator is that it may introduce subtle link compatibility
// problems.  The argument for changing it is that the usual default
// allocator is usually a very bad choice for a garbage collected environment.)
//

#ifndef GC_NEW_ALLOC_H
#define GC_NEW_ALLOC_H

#include "gc.h"

#if GC_GNUC_PREREQ(3, 0)
# include <bits/stl_alloc.h>
# ifndef __STL_BEGIN_NAMESPACE
#   define __STL_BEGIN_NAMESPACE namespace std {
#   define __STL_END_NAMESPACE };
# endif
# ifndef __STL_USE_STD_ALLOCATORS
#   define __STL_USE_STD_ALLOCATORS
# endif
#else
# include <stack>   // A more portable way to get stl_alloc.h file.
#endif

/* A hack to deal with gcc 3.1.  If you are using gcc3.1 and later,     */
/* you should probably really use gc_allocator.h instead.               */
#if GC_GNUC_PREREQ(3, 1)
# define simple_alloc __simple_alloc
#endif

#include <stddef.h>
#include <string.h>

// We can't include gc_priv.h, since that pulls in way too much stuff.
#include "gc_alloc_ptrs.h"

#include "gc_mark.h" // for GC_generic_malloc

#define GC_generic_malloc_words_small(lw, k) \
                        GC_generic_malloc((lw) * sizeof(GC_word), k)

#define GC_ALLOCATOR_THROW_OR_ABORT() GC_abort_on_oom()

// Object kinds; must match PTRFREE, NORMAL, UNCOLLECTABLE, and
// AUNCOLLECTABLE in gc_priv.h.

enum { GC_PTRFREE = 0, GC_NORMAL = 1, GC_UNCOLLECTABLE = 2,
       GC_AUNCOLLECTABLE = 3 };

enum { GC_max_fast_bytes = 255 };

enum { GC_byte_alignment = 8 };

#if defined(CPPCHECK)
  const unsigned GC_bytes_per_word = sizeof(char *);
  const unsigned GC_word_alignment = GC_byte_alignment/GC_bytes_per_word;
#else
  enum { GC_bytes_per_word = sizeof(char *) };
  enum { GC_word_alignment = GC_byte_alignment/GC_bytes_per_word };
#endif

inline void * &GC_obj_link(void * p)
{   return *reinterpret_cast<void **>(p);  }

// Compute a number of words >= n+1 bytes.
// The +1 allows for pointers one past the end.
inline size_t GC_round_up(size_t n)
{
    return ((n + GC_byte_alignment)/GC_byte_alignment)*GC_word_alignment;
}

// The same but don't allow for extra byte.
inline size_t GC_round_up_uncollectable(size_t n)
{
    return ((n + GC_byte_alignment - 1)/GC_byte_alignment)*GC_word_alignment;
}

template <int dummy>
class GC_aux_template {
public:
  // File local count of allocated words.  Occasionally this is
  // added into the global count.  A separate count is necessary since the
  // real one must be updated with a procedure call.
  static size_t GC_bytes_recently_allocd;

  // Same for uncollectible memory.  Not yet reflected in either
  // GC_bytes_recently_allocd or GC_non_gc_bytes.
  static size_t GC_uncollectable_bytes_recently_allocd;

  // Similar counter for explicitly deallocated memory.
  static size_t GC_bytes_recently_freed;

  // Again for uncollectible memory.
  static size_t GC_uncollectable_bytes_recently_freed;

  static void * GC_out_of_line_malloc(size_t nwords, int kind);
};

template <int dummy>
size_t GC_aux_template<dummy>::GC_bytes_recently_allocd = 0;

template <int dummy>
size_t GC_aux_template<dummy>::GC_uncollectable_bytes_recently_allocd = 0;

template <int dummy>
size_t GC_aux_template<dummy>::GC_bytes_recently_freed = 0;

template <int dummy>
size_t GC_aux_template<dummy>::GC_uncollectable_bytes_recently_freed = 0;

template <int dummy>
void * GC_aux_template<dummy>::GC_out_of_line_malloc(size_t nwords, int kind)
{
    void * op = GC_generic_malloc_words_small(nwords, kind);
    if (0 == op)
        GC_ALLOCATOR_THROW_OR_ABORT();

    GC_word non_gc_bytes = GC_get_non_gc_bytes();
    GC_bytes_recently_allocd += GC_uncollectable_bytes_recently_allocd;
    non_gc_bytes += GC_uncollectable_bytes_recently_allocd;
    GC_uncollectable_bytes_recently_allocd = 0;

    GC_bytes_recently_freed += GC_uncollectable_bytes_recently_freed;
    non_gc_bytes -= GC_uncollectable_bytes_recently_freed;
    GC_uncollectable_bytes_recently_freed = 0;
    GC_set_non_gc_bytes(non_gc_bytes);

    GC_incr_bytes_allocd(GC_bytes_recently_allocd);
    GC_bytes_recently_allocd = 0;

    GC_incr_bytes_freed(GC_bytes_recently_freed);
    GC_bytes_recently_freed = 0;
    return op;
}

typedef GC_aux_template<0> GC_aux;

// A fast, single-threaded, garbage-collected allocator
// We assume the first word will be immediately overwritten.
// In this version, deallocation is not a no-op, and explicit
// deallocation is likely to help performance.
template <int dummy>
class single_client_gc_alloc_template {
    public:
        static void * allocate(size_t n)
        {
            size_t nwords = GC_round_up(n);
            void ** flh;
            void * op;

            if (n > GC_max_fast_bytes) {
                op = GC_malloc(n);
                if (0 == op)
                    GC_ALLOCATOR_THROW_OR_ABORT();
                return op;
            }
            flh = &GC_objfreelist_ptr[nwords];
            op = *flh;
            if (0 == op) {
                return GC_aux::GC_out_of_line_malloc(nwords, GC_NORMAL);
            }
            *flh = GC_obj_link(op);
            GC_aux::GC_bytes_recently_allocd += nwords * GC_bytes_per_word;
            return op;
        }
        static void * ptr_free_allocate(size_t n)
        {
            size_t nwords = GC_round_up(n);
            void ** flh;
            void * op;

            if (n > GC_max_fast_bytes) {
                op = GC_malloc_atomic(n);
                if (0 == op)
                    GC_ALLOCATOR_THROW_OR_ABORT();
                return op;
            }
            flh = &GC_aobjfreelist_ptr[nwords];
            op = *flh;
            if (0 == op) {
                return GC_aux::GC_out_of_line_malloc(nwords, GC_PTRFREE);
            }
            *flh = GC_obj_link(op);
            GC_aux::GC_bytes_recently_allocd += nwords * GC_bytes_per_word;
            return op;
        }
        static void deallocate(void *p, size_t n)
        {
            if (n > GC_max_fast_bytes)  {
                GC_free(p);
            } else {
                size_t nwords = GC_round_up(n);
                void ** flh = &GC_objfreelist_ptr[nwords];

                GC_obj_link(p) = *flh;
                memset(reinterpret_cast<char *>(p) + GC_bytes_per_word, 0,
                       GC_bytes_per_word * (nwords - 1));
                *flh = p;
                GC_aux::GC_bytes_recently_freed += nwords * GC_bytes_per_word;
            }
        }
        static void ptr_free_deallocate(void *p, size_t n)
        {
            if (n > GC_max_fast_bytes) {
                GC_free(p);
            } else {
                size_t nwords = GC_round_up(n);
                void ** flh = &GC_aobjfreelist_ptr[nwords];

                GC_obj_link(p) = *flh;
                *flh = p;
                GC_aux::GC_bytes_recently_freed += nwords * GC_bytes_per_word;
            }
        }
};

typedef single_client_gc_alloc_template<0> single_client_gc_alloc;

// Once more, for uncollectible objects.
template <int dummy>
class single_client_traceable_alloc_template {
    public:
        static void * allocate(size_t n)
        {
            size_t nwords = GC_round_up_uncollectable(n);
            void ** flh;
            void * op;

            if (n > GC_max_fast_bytes) {
                op = GC_malloc_uncollectable(n);
                if (0 == op)
                    GC_ALLOCATOR_THROW_OR_ABORT();
                return op;
            }
            flh = &GC_uobjfreelist_ptr[nwords];
            op = *flh;
            if (0 == op) {
                return GC_aux::GC_out_of_line_malloc(nwords, GC_UNCOLLECTABLE);
            }
            *flh = GC_obj_link(op);
            GC_aux::GC_uncollectable_bytes_recently_allocd +=
                                        nwords * GC_bytes_per_word;
            return op;
        }
        static void * ptr_free_allocate(size_t n)
        {
            size_t nwords = GC_round_up_uncollectable(n);
            void ** flh;
            void * op;

            if (n > GC_max_fast_bytes) {
                op = GC_malloc_atomic_uncollectable(n);
                if (0 == op)
                    GC_ALLOCATOR_THROW_OR_ABORT();
                return op;
            }
            flh = &GC_auobjfreelist_ptr[nwords];
            op = *flh;
            if (0 == op) {
                return GC_aux::GC_out_of_line_malloc(nwords, GC_AUNCOLLECTABLE);
            }
            *flh = GC_obj_link(op);
            GC_aux::GC_uncollectable_bytes_recently_allocd +=
                                        nwords * GC_bytes_per_word;
            return op;
        }
        static void deallocate(void *p, size_t n)
        {
            if (n > GC_max_fast_bytes)  {
                GC_free(p);
            } else {
                size_t nwords = GC_round_up_uncollectable(n);
                void ** flh = &GC_uobjfreelist_ptr[nwords];

                GC_obj_link(p) = *flh;
                *flh = p;
                GC_aux::GC_uncollectable_bytes_recently_freed +=
                                nwords * GC_bytes_per_word;
            }
        }
        static void ptr_free_deallocate(void *p, size_t n)
        {
            if (n > GC_max_fast_bytes) {
                GC_free(p);
            } else {
                size_t nwords = GC_round_up_uncollectable(n);
                void ** flh = &GC_auobjfreelist_ptr[nwords];

                GC_obj_link(p) = *flh;
                *flh = p;
                GC_aux::GC_uncollectable_bytes_recently_freed +=
                                nwords * GC_bytes_per_word;
            }
        }
};

typedef single_client_traceable_alloc_template<0> single_client_traceable_alloc;

template < int dummy >
class gc_alloc_template {
    public:
        static void * allocate(size_t n) {
            void * op = GC_malloc(n);
            if (0 == op)
                GC_ALLOCATOR_THROW_OR_ABORT();
            return op;
        }
        static void * ptr_free_allocate(size_t n) {
            void * op = GC_malloc_atomic(n);
            if (0 == op)
                GC_ALLOCATOR_THROW_OR_ABORT();
            return op;
        }
        static void deallocate(void *, size_t) { }
        static void ptr_free_deallocate(void *, size_t) { }
};

typedef gc_alloc_template < 0 > gc_alloc;

template < int dummy >
class traceable_alloc_template {
    public:
        static void * allocate(size_t n) {
            void * op = GC_malloc_uncollectable(n);
            if (0 == op)
                GC_ALLOCATOR_THROW_OR_ABORT();
            return op;
        }
        static void * ptr_free_allocate(size_t n) {
            void * op = GC_malloc_atomic_uncollectable(n);
            if (0 == op)
                GC_ALLOCATOR_THROW_OR_ABORT();
            return op;
        }
        static void deallocate(void *p, size_t) { GC_free(p); }
        static void ptr_free_deallocate(void *p, size_t) { GC_free(p); }
};

typedef traceable_alloc_template < 0 > traceable_alloc;

// We want to specialize simple_alloc so that it does the right thing
// for all pointer-free types.  At the moment there is no portable way to
// even approximate that.  The following approximation should work for
// SGI compilers, and recent versions of g++.

// GC_SPECIALIZE() is used internally.
#define GC_SPECIALIZE(T,alloc) \
  class simple_alloc<T, alloc> { \
  public: \
    static T *allocate(size_t n) \
        { reinterpret_cast<T*>(alloc::ptr_free_allocate(0 == n ? 1 \
                                                    : n * sizeof(T))); } \
    static T *allocate(void) \
        { return reinterpret_cast<T*>(alloc::ptr_free_allocate(sizeof(T))); } \
    static void deallocate(T *p, size_t n) \
        { alloc::ptr_free_deallocate(p, 0 == n ? 1 : n * sizeof(T)); } \
    static void deallocate(T *p) \
        { alloc::ptr_free_deallocate(p, sizeof(T)); } \
  };

__STL_BEGIN_NAMESPACE

GC_SPECIALIZE(char, gc_alloc)
GC_SPECIALIZE(int, gc_alloc)
GC_SPECIALIZE(unsigned, gc_alloc)
GC_SPECIALIZE(float, gc_alloc)
GC_SPECIALIZE(double, gc_alloc)

GC_SPECIALIZE(char, traceable_alloc)
GC_SPECIALIZE(int, traceable_alloc)
GC_SPECIALIZE(unsigned, traceable_alloc)
GC_SPECIALIZE(float, traceable_alloc)
GC_SPECIALIZE(double, traceable_alloc)

GC_SPECIALIZE(char, single_client_gc_alloc)
GC_SPECIALIZE(int, single_client_gc_alloc)
GC_SPECIALIZE(unsigned, single_client_gc_alloc)
GC_SPECIALIZE(float, single_client_gc_alloc)
GC_SPECIALIZE(double, single_client_gc_alloc)

GC_SPECIALIZE(char, single_client_traceable_alloc)
GC_SPECIALIZE(int, single_client_traceable_alloc)
GC_SPECIALIZE(unsigned, single_client_traceable_alloc)
GC_SPECIALIZE(float, single_client_traceable_alloc)
GC_SPECIALIZE(double, single_client_traceable_alloc)

__STL_END_NAMESPACE

#ifdef __STL_USE_STD_ALLOCATORS

__STL_BEGIN_NAMESPACE

template <class _Tp>
struct _Alloc_traits<_Tp, gc_alloc >
{
  static const bool _S_instanceless = true;
  typedef simple_alloc<_Tp, gc_alloc > _Alloc_type;
  typedef __allocator<_Tp, gc_alloc > allocator_type;
};

inline bool operator==(const gc_alloc&,
                       const gc_alloc&)
{
  return true;
}

inline bool operator!=(const gc_alloc&,
                       const gc_alloc&)
{
  return false;
}

template <class _Tp>
struct _Alloc_traits<_Tp, single_client_gc_alloc >
{
  static const bool _S_instanceless = true;
  typedef simple_alloc<_Tp, single_client_gc_alloc > _Alloc_type;
  typedef __allocator<_Tp, single_client_gc_alloc > allocator_type;
};

inline bool operator==(const single_client_gc_alloc&,
                       const single_client_gc_alloc&)
{
  return true;
}

inline bool operator!=(const single_client_gc_alloc&,
                       const single_client_gc_alloc&)
{
  return false;
}

template <class _Tp>
struct _Alloc_traits<_Tp, traceable_alloc >
{
  static const bool _S_instanceless = true;
  typedef simple_alloc<_Tp, traceable_alloc > _Alloc_type;
  typedef __allocator<_Tp, traceable_alloc > allocator_type;
};

inline bool operator==(const traceable_alloc&,
                       const traceable_alloc&)
{
  return true;
}

inline bool operator!=(const traceable_alloc&,
                       const traceable_alloc&)
{
  return false;
}

template <class _Tp>
struct _Alloc_traits<_Tp, single_client_traceable_alloc >
{
  static const bool _S_instanceless = true;
  typedef simple_alloc<_Tp, single_client_traceable_alloc > _Alloc_type;
  typedef __allocator<_Tp, single_client_traceable_alloc > allocator_type;
};

inline bool operator==(const single_client_traceable_alloc&,
                       const single_client_traceable_alloc&)
{
  return true;
}

inline bool operator!=(const single_client_traceable_alloc&,
                       const single_client_traceable_alloc&)
{
  return false;
}

__STL_END_NAMESPACE

#endif /* __STL_USE_STD_ALLOCATORS */

#endif /* GC_NEW_ALLOC_H */
