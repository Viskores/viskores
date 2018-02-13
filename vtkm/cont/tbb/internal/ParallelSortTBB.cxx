//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

// Copyright 2010, Takuya Akiba
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Takuya Akiba nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Modifications of Takuya Akiba's original GitHub source code for inclusion
// in VTK-m:
//
//   - Changed parallel threading from OpenMP to TBB tasks
//   - Added minimum threshold for parallel, will instead invoke serial radix sort (kxsort)
//   - Added std::greater<T> and std::less<T> to interface for descending order sorts
//   - Added linear scaling of threads used by the algorithm for more stable performance
//     on machines with lots of available threads (KNL and Haswell)
//
// This file contains an implementation of Satish parallel radix sort
// as documented in the following citation:
//
//  Fast sort on CPUs and GPUs: a case for bandwidth oblivious SIMD sort.
//       N. Satish, C. Kim, J. Chhugani, A. D. Nguyen, V. W. Lee, D. Kim, and P. Dubey.
//       In Proc. SIGMOD, pages 351–362, 2010
#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstring>
#include <functional>
#include <stdint.h>
#include <utility>

#include <vtkm/Types.h>
#include <vtkm/cont/tbb/internal/ParallelSortTBB.h>
#include <vtkm/cont/tbb/internal/kxsort.h>

VTKM_THIRDPARTY_PRE_INCLUDE

#if defined(VTKM_MSVC)

// TBB's header include a #pragma comment(lib,"tbb.lib") line to make all
// consuming libraries link to tbb, this is bad behavior in a header
// based project
#pragma push_macro("__TBB_NO_IMPLICITLINKAGE")
#define __TBB_NO_IMPLICIT_LINKAGE 1

#endif // defined(VTKM_MSVC)

// TBB includes windows.h, so instead we want to include windows.h with the
// correct settings so that we don't clobber any existing function
#include <vtkm/internal/Windows.h>

#include <tbb/task.h>
#include <tbb/tbb_thread.h>

#if defined(VTKM_MSVC)
#pragma pop_macro("__TBB_NO_IMPLICITLINKAGE")
#endif

namespace vtkm
{
namespace cont
{
namespace tbb
{
namespace internal
{

const size_t MIN_BYTES_FOR_PARALLEL = 400000;
const size_t BYTES_FOR_MAX_PARALLELISM = 4000000;
const size_t MAX_CORES = ::tbb::tbb_thread::hardware_concurrency();
const double CORES_PER_BYTE =
  double(MAX_CORES - 1) / double(BYTES_FOR_MAX_PARALLELISM - MIN_BYTES_FOR_PARALLEL);
const double Y_INTERCEPT = 1.0 - CORES_PER_BYTE * MIN_BYTES_FOR_PARALLEL;

namespace utility
{
// Return the number of threads that would be executed in parallel regions
inline size_t GetMaxThreads(size_t num_bytes)
{
  size_t num_cores = (size_t)(CORES_PER_BYTE * double(num_bytes) + Y_INTERCEPT);
  if (num_cores < 1)
  {
    return 1;
  }
  if (num_cores > MAX_CORES)
  {
    return MAX_CORES;
  }
  return num_cores;
}
} // namespace utility

namespace internal
{
// Size of the software managed buffer
const size_t kOutBufferSize = 32;

// Ascending order radix sort is a no-op
template <typename PlainType,
          typename UnsignedType,
          typename CompareType,
          typename ValueManager,
          unsigned int Base>
struct ParallelRadixCompareInternal
{
  inline static void reverse(UnsignedType& t) { (void)t; }
};

// Handle descending order radix sort
template <typename PlainType, typename UnsignedType, typename ValueManager, unsigned int Base>
struct ParallelRadixCompareInternal<PlainType,
                                    UnsignedType,
                                    std::greater<PlainType>,
                                    ValueManager,
                                    Base>
{
  inline static void reverse(UnsignedType& t) { t = ((1 << Base) - 1) - t; }
};

// The algorithm is implemented in this internal class
template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
class ParallelRadixSortInternal
{
public:
  typedef ParallelRadixCompareInternal<PlainType, UnsignedType, CompareType, ValueManager, Base>
    CompareInternal;

  ParallelRadixSortInternal();
  ~ParallelRadixSortInternal();

  void Init(PlainType* data, size_t num_elems);

  PlainType* Sort(PlainType* data, ValueManager* value_manager);

  static void InitAndSort(PlainType* data, size_t num_elems, ValueManager* value_manager);

private:
  CompareInternal compare_internal_;
  size_t num_elems_;
  size_t num_threads_;

  UnsignedType* tmp_;
  size_t** histo_;
  UnsignedType*** out_buf_;
  size_t** out_buf_n_;

  size_t *pos_bgn_, *pos_end_;
  ValueManager* value_manager_;

  void DeleteAll();

  UnsignedType* SortInternal(UnsignedType* data, ValueManager* value_manager);

  // Compute |pos_bgn_| and |pos_end_| (associated ranges for each threads)
  void ComputeRanges();

  // First step of each iteration of sorting
  // Compute the histogram of |src| using bits in [b, b + Base)
  void ComputeHistogram(unsigned int b, UnsignedType* src);

  // Second step of each iteration of sorting
  // Scatter elements of |src| to |dst| using the histogram
  void Scatter(unsigned int b, UnsignedType* src, UnsignedType* dst);
};

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  ParallelRadixSortInternal()
  : num_elems_(0)
  , num_threads_(0)
  , tmp_(NULL)
  , histo_(NULL)
  , out_buf_(NULL)
  , out_buf_n_(NULL)
  , pos_bgn_(NULL)
  , pos_end_(NULL)
{
  assert(sizeof(PlainType) == sizeof(UnsignedType));
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  ~ParallelRadixSortInternal()
{
  DeleteAll();
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
void ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  DeleteAll()
{
  delete[] tmp_;
  tmp_ = NULL;

  for (size_t i = 0; i < num_threads_; ++i)
    delete[] histo_[i];
  delete[] histo_;
  histo_ = NULL;

  for (size_t i = 0; i < num_threads_; ++i)
  {
    for (size_t j = 0; j < 1 << Base; ++j)
    {
      delete[] out_buf_[i][j];
    }
    delete[] out_buf_n_[i];
    delete[] out_buf_[i];
  }
  delete[] out_buf_;
  delete[] out_buf_n_;
  out_buf_ = NULL;
  out_buf_n_ = NULL;

  delete[] pos_bgn_;
  delete[] pos_end_;
  pos_bgn_ = pos_end_ = NULL;

  num_elems_ = 0;
  num_threads_ = 0;
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
void ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  Init(PlainType* data, size_t num_elems)
{
  (void)data;
  DeleteAll();

  num_elems_ = num_elems;

  num_threads_ = utility::GetMaxThreads(num_elems_ * sizeof(PlainType));

  tmp_ = new UnsignedType[num_elems_];
  histo_ = new size_t*[num_threads_];
  for (size_t i = 0; i < num_threads_; ++i)
  {
    histo_[i] = new size_t[1 << Base];
  }

  out_buf_ = new UnsignedType**[num_threads_];
  out_buf_n_ = new size_t*[num_threads_];
  for (size_t i = 0; i < num_threads_; ++i)
  {
    out_buf_[i] = new UnsignedType*[1 << Base];
    out_buf_n_[i] = new size_t[1 << Base];
    for (size_t j = 0; j < 1 << Base; ++j)
    {
      out_buf_[i][j] = new UnsignedType[kOutBufferSize];
    }
  }

  pos_bgn_ = new size_t[num_threads_];
  pos_end_ = new size_t[num_threads_];
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
PlainType*
ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::Sort(
  PlainType* data,
  ValueManager* value_manager)
{
  UnsignedType* src = reinterpret_cast<UnsignedType*>(data);
  UnsignedType* res = SortInternal(src, value_manager);
  return reinterpret_cast<PlainType*>(res);
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
void ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  InitAndSort(PlainType* data, size_t num_elems, ValueManager* value_manager)
{
  ParallelRadixSortInternal prs;
  prs.Init(data, num_elems);
  const PlainType* res = prs.Sort(data, value_manager);
  if (res != data)
  {
    for (size_t i = 0; i < num_elems; ++i)
      data[i] = res[i];
  }
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
UnsignedType*
ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  SortInternal(UnsignedType* data, ValueManager* value_manager)
{

  value_manager_ = value_manager;

  // Compute |pos_bgn_| and |pos_end_|
  ComputeRanges();

  // Iterate from lower bits to higher bits
  const size_t bits = CHAR_BIT * sizeof(UnsignedType);
  UnsignedType *src = data, *dst = tmp_;
  for (unsigned int b = 0; b < bits; b += Base)
  {
    ComputeHistogram(b, src);
    Scatter(b, src, dst);

    std::swap(src, dst);
    value_manager->Next();
  }

  return src;
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
void ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  ComputeRanges()
{
  pos_bgn_[0] = 0;
  for (size_t i = 0; i < num_threads_ - 1; ++i)
  {
    const size_t t = (num_elems_ - pos_bgn_[i]) / (num_threads_ - i);
    pos_bgn_[i + 1] = pos_end_[i] = pos_bgn_[i] + t;
  }
  pos_end_[num_threads_ - 1] = num_elems_;
}

template <typename PlainType,
          typename UnsignedType,
          typename Encoder,
          unsigned int Base,
          typename Function>
class RunTask : public ::tbb::task
{
public:
  RunTask(size_t binary_tree_height,
          size_t binary_tree_position,
          Function f,
          size_t num_elems,
          size_t num_threads)
    : binary_tree_height_(binary_tree_height)
    , binary_tree_position_(binary_tree_position)
    , f_(f)
    , num_elems_(num_elems)
    , num_threads_(num_threads)
  {
  }

  ::tbb::task* execute()
  {
    size_t num_nodes_at_current_height = (size_t)pow(2, (double)binary_tree_height_);
    if (num_threads_ <= num_nodes_at_current_height)
    {
      const size_t my_id = binary_tree_position_ - num_nodes_at_current_height;
      if (my_id < num_threads_)
      {
        f_(my_id);
      }
      return NULL;
    }
    else
    {
      ::tbb::empty_task& p = *new (task::allocate_continuation())::tbb::empty_task();
      RunTask& left = *new (p.allocate_child()) RunTask(
        binary_tree_height_ + 1, 2 * binary_tree_position_, f_, num_elems_, num_threads_);
      RunTask& right = *new (p.allocate_child()) RunTask(
        binary_tree_height_ + 1, 2 * binary_tree_position_ + 1, f_, num_elems_, num_threads_);
      p.set_ref_count(2);
      task::spawn(left);
      task::spawn(right);
      return NULL;
    }
  }

private:
  size_t binary_tree_height_;
  size_t binary_tree_position_;
  Function f_;
  size_t num_elems_;
  size_t num_threads_;
};

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
void ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  ComputeHistogram(unsigned int b, UnsignedType* src)
{
  // Compute local histogram

  auto lambda = [=](const size_t my_id) {
    const size_t my_bgn = pos_bgn_[my_id];
    const size_t my_end = pos_end_[my_id];
    size_t* my_histo = histo_[my_id];

    memset(my_histo, 0, sizeof(size_t) * (1 << Base));
    for (size_t i = my_bgn; i < my_end; ++i)
    {
      const UnsignedType s = Encoder::encode(src[i]);
      UnsignedType t = (s >> b) & ((1 << Base) - 1);
      compare_internal_.reverse(t);
      ++my_histo[t];
    }
  };

  typedef RunTask<PlainType, UnsignedType, Encoder, Base, std::function<void(size_t)>> RunTaskType;

  RunTaskType& root =
    *new (::tbb::task::allocate_root()) RunTaskType(0, 1, lambda, num_elems_, num_threads_);

  ::tbb::task::spawn_root_and_wait(root);

  // Compute global histogram
  size_t s = 0;
  for (size_t i = 0; i < 1 << Base; ++i)
  {
    for (size_t j = 0; j < num_threads_; ++j)
    {
      const size_t t = s + histo_[j][i];
      histo_[j][i] = s;
      s = t;
    }
  }
}

template <typename PlainType,
          typename CompareType,
          typename UnsignedType,
          typename Encoder,
          typename ValueManager,
          unsigned int Base>
void ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>::
  Scatter(unsigned int b, UnsignedType* src, UnsignedType* dst)
{

  auto lambda = [=](const size_t my_id) {
    const size_t my_bgn = pos_bgn_[my_id];
    const size_t my_end = pos_end_[my_id];
    size_t* my_histo = histo_[my_id];
    UnsignedType** my_buf = out_buf_[my_id];
    size_t* my_buf_n = out_buf_n_[my_id];

    memset(my_buf_n, 0, sizeof(size_t) * (1 << Base));
    for (size_t i = my_bgn; i < my_end; ++i)
    {
      const UnsignedType s = Encoder::encode(src[i]);
      UnsignedType t = (s >> b) & ((1 << Base) - 1);
      compare_internal_.reverse(t);
      my_buf[t][my_buf_n[t]] = src[i];
      value_manager_->Push(my_id, t, my_buf_n[t], i);
      ++my_buf_n[t];

      if (my_buf_n[t] == kOutBufferSize)
      {
        size_t p = my_histo[t];
        for (size_t j = 0; j < kOutBufferSize; ++j)
        {
          size_t tp = p++;
          dst[tp] = my_buf[t][j];
        }
        value_manager_->Flush(my_id, t, kOutBufferSize, my_histo[t]);

        my_histo[t] += kOutBufferSize;
        my_buf_n[t] = 0;
      }
    }

    // Flush everything
    for (size_t i = 0; i < 1 << Base; ++i)
    {
      size_t p = my_histo[i];
      for (size_t j = 0; j < my_buf_n[i]; ++j)
      {
        size_t tp = p++;
        dst[tp] = my_buf[i][j];
      }
      value_manager_->Flush(my_id, i, my_buf_n[i], my_histo[i]);
    }
  };

  typedef RunTask<PlainType, UnsignedType, Encoder, Base, std::function<void(size_t)>> RunTaskType;

  RunTaskType& root =
    *new (::tbb::task::allocate_root()) RunTaskType(0, 1, lambda, num_elems_, num_threads_);

  ::tbb::task::spawn_root_and_wait(root);
}
} // namespace internal

// Encoders encode signed/unsigned integers and floating point numbers
// to correctly ordered unsigned integers
namespace encoder
{
class EncoderDummy
{
};

class EncoderUnsigned
{
public:
  template <typename UnsignedType>
  inline static UnsignedType encode(UnsignedType x)
  {
    return x;
  }
};

class EncoderSigned
{
public:
  template <typename UnsignedType>
  inline static UnsignedType encode(UnsignedType x)
  {
    return x ^ (UnsignedType(1) << (CHAR_BIT * sizeof(UnsignedType) - 1));
  }
};

class EncoderDecimal
{
public:
  template <typename UnsignedType>
  inline static UnsignedType encode(UnsignedType x)
  {
    static const size_t bits = CHAR_BIT * sizeof(UnsignedType);
    const UnsignedType a = x >> (bits - 1);
    const UnsignedType b = (-a) | (UnsignedType(1) << (bits - 1));
    return x ^ b;
  }
};
} // namespace encoder

// Value managers are used to generalize the sorting algorithm
// to sorting of keys and sorting of pairs
namespace value_manager
{
class DummyValueManager
{
public:
  inline void Push(int thread __attribute__((unused)),
                   size_t bucket __attribute__((unused)),
                   size_t num __attribute__((unused)),
                   size_t from_pos __attribute__((unused)))
  {
  }

  inline void Flush(int thread __attribute__((unused)),
                    size_t bucket __attribute__((unused)),
                    size_t num __attribute__((unused)),
                    size_t to_pos __attribute__((unused)))
  {
  }

  void Next() {}
};

template <typename ValueType, int Base>
class PairValueManager
{
public:
  PairValueManager()
    : max_elems_(0)
    , max_threads_(0)
    , original_(NULL)
    , tmp_(NULL)
    , src_(NULL)
    , dst_(NULL)
    , out_buf_(NULL)
  {
  }

  ~PairValueManager() { DeleteAll(); }

  void Init(size_t max_elems);

  void Start(ValueType* original, size_t num_elems)
  {
    assert(num_elems <= max_elems_);
    src_ = original_ = original;
    dst_ = tmp_;
  }

  inline void Push(int thread, size_t bucket, size_t num, size_t from_pos)
  {
    out_buf_[thread][bucket][num] = src_[from_pos];
  }

  inline void Flush(int thread, size_t bucket, size_t num, size_t to_pos)
  {
    for (size_t i = 0; i < num; ++i)
    {
      dst_[to_pos++] = out_buf_[thread][bucket][i];
    }
  }

  void Next() { std::swap(src_, dst_); }

  ValueType* GetResult() { return src_; }
private:
  size_t max_elems_;
  int max_threads_;

  static const size_t kOutBufferSize = internal::kOutBufferSize;
  ValueType *original_, *tmp_;
  ValueType *src_, *dst_;
  ValueType*** out_buf_;

  void DeleteAll();
};

template <typename ValueType, int Base>
void PairValueManager<ValueType, Base>::Init(size_t max_elems)
{
  DeleteAll();

  max_elems_ = max_elems;
  max_threads_ = utility::GetMaxThreads(max_elems_ * sizeof(ValueType));

  tmp_ = new ValueType[max_elems];

  out_buf_ = new ValueType**[max_threads_];
  for (int i = 0; i < max_threads_; ++i)
  {
    out_buf_[i] = new ValueType*[1 << Base];
    for (size_t j = 0; j < 1 << Base; ++j)
    {
      out_buf_[i][j] = new ValueType[kOutBufferSize];
    }
  }
}

template <typename ValueType, int Base>
void PairValueManager<ValueType, Base>::DeleteAll()
{
  delete[] tmp_;
  tmp_ = NULL;

  for (int i = 0; i < max_threads_; ++i)
  {
    for (size_t j = 0; j < 1 << Base; ++j)
    {
      delete[] out_buf_[i][j];
    }
    delete[] out_buf_[i];
  }
  delete[] out_buf_;
  out_buf_ = NULL;

  max_elems_ = 0;
  max_threads_ = 0;
}
} // namespace value_manager

// Frontend class for sorting keys
template <typename PlainType,
          typename CompareType,
          typename UnsignedType = PlainType,
          typename Encoder = encoder::EncoderDummy,
          unsigned int Base = 8>
class KeySort
{
  typedef value_manager::DummyValueManager DummyValueManager;
  typedef internal::ParallelRadixSortInternal<PlainType,
                                              CompareType,
                                              UnsignedType,
                                              Encoder,
                                              DummyValueManager,
                                              Base>
    Internal;

public:
  void InitAndSort(PlainType* data, size_t num_elems, const CompareType& comp)
  {
    (void)comp;
    DummyValueManager dvm;
    Internal::InitAndSort(data, num_elems, &dvm);
  }
};

// Frontend class for sorting pairs
template <typename PlainType,
          typename ValueType,
          typename CompareType,
          typename UnsignedType = PlainType,
          typename Encoder = encoder::EncoderDummy,
          int Base = 8>
class PairSort
{
  typedef value_manager::PairValueManager<ValueType, Base> ValueManager;
  typedef internal::
    ParallelRadixSortInternal<PlainType, CompareType, UnsignedType, Encoder, ValueManager, Base>
      Internal;

public:
  void InitAndSort(PlainType* keys, ValueType* vals, size_t num_elems, const CompareType& comp)
  {
    (void)comp;
    ValueManager vm;
    vm.Init(num_elems);
    vm.Start(vals, num_elems);
    Internal::InitAndSort(keys, num_elems, &vm);
    ValueType* res_vals = vm.GetResult();
    if (res_vals != vals)
    {
      for (size_t i = 0; i < num_elems; ++i)
      {
        vals[i] = res_vals[i];
      }
    }
  }

private:
};

#define KEY_SORT_CASE(plain_type, compare_type, unsigned_type, encoder_type)                       \
  template <>                                                                                      \
  class KeySort<plain_type, compare_type>                                                          \
    : public KeySort<plain_type, compare_type, unsigned_type, encoder::Encoder##encoder_type>      \
  {                                                                                                \
  };                                                                                               \
  template <typename V>                                                                            \
  class PairSort<plain_type, V, compare_type>                                                      \
    : public PairSort<plain_type, V, compare_type, unsigned_type, encoder::Encoder##encoder_type>  \
  {                                                                                                \
  };

// Unsigned integers
KEY_SORT_CASE(unsigned int, std::less<unsigned int>, unsigned int, Unsigned);
KEY_SORT_CASE(unsigned int, std::greater<unsigned int>, unsigned int, Unsigned);
KEY_SORT_CASE(unsigned short int, std::less<unsigned short int>, unsigned short int, Unsigned);
KEY_SORT_CASE(unsigned short int, std::greater<unsigned short int>, unsigned short int, Unsigned);
KEY_SORT_CASE(unsigned long int, std::less<unsigned long int>, unsigned long int, Unsigned);
KEY_SORT_CASE(unsigned long int, std::greater<unsigned long int>, unsigned long int, Unsigned);
KEY_SORT_CASE(unsigned long long int,
              std::less<unsigned long long int>,
              unsigned long long int,
              Unsigned);
KEY_SORT_CASE(unsigned long long int,
              std::greater<unsigned long long int>,
              unsigned long long int,
              Unsigned);

// Unsigned char
KEY_SORT_CASE(unsigned char, std::less<unsigned char>, unsigned char, Unsigned);
KEY_SORT_CASE(unsigned char, std::greater<unsigned char>, unsigned char, Unsigned);
KEY_SORT_CASE(char16_t, std::less<char16_t>, uint16_t, Unsigned);
KEY_SORT_CASE(char16_t, std::greater<char16_t>, uint16_t, Unsigned);
KEY_SORT_CASE(char32_t, std::less<char32_t>, uint32_t, Unsigned);
KEY_SORT_CASE(char32_t, std::greater<char32_t>, uint32_t, Unsigned);
KEY_SORT_CASE(wchar_t, std::less<wchar_t>, uint32_t, Unsigned);
KEY_SORT_CASE(wchar_t, std::greater<wchar_t>, uint32_t, Unsigned);

// Signed integers
KEY_SORT_CASE(char, std::less<char>, unsigned char, Signed);
KEY_SORT_CASE(char, std::greater<char>, unsigned char, Signed);
KEY_SORT_CASE(short, std::less<short>, unsigned short, Signed);
KEY_SORT_CASE(short, std::greater<short>, unsigned short, Signed);
KEY_SORT_CASE(int, std::less<int>, unsigned int, Signed);
KEY_SORT_CASE(int, std::greater<int>, unsigned int, Signed);
KEY_SORT_CASE(long, std::less<long>, unsigned long, Signed);
KEY_SORT_CASE(long, std::greater<long>, unsigned long, Signed);
KEY_SORT_CASE(long long, std::less<long long>, unsigned long long, Signed);
KEY_SORT_CASE(long long, std::greater<long long>, unsigned long long, Signed);

// |signed char| and |char| are treated as different types
KEY_SORT_CASE(signed char, std::less<signed char>, unsigned char, Signed);
KEY_SORT_CASE(signed char, std::greater<signed char>, unsigned char, Signed);

// Floating point numbers
KEY_SORT_CASE(float, std::less<float>, uint32_t, Decimal);
KEY_SORT_CASE(float, std::greater<float>, uint32_t, Decimal);
KEY_SORT_CASE(double, std::less<double>, uint64_t, Decimal);
KEY_SORT_CASE(double, std::greater<double>, uint64_t, Decimal);

#undef KEY_SORT_CASE

template <typename T, typename CompareType>
struct run_kx_radix_sort_keys
{
  static void run(T* data, size_t num_elems, const CompareType& comp)
  {
    std::sort(data, data + num_elems, comp);
  }
};

#define KX_SORT_KEYS(key_type)                                                                     \
  template <>                                                                                      \
  struct run_kx_radix_sort_keys<key_type, std::less<key_type>>                                     \
  {                                                                                                \
    static void run(key_type* data, size_t num_elems, const std::less<key_type>& comp)             \
    {                                                                                              \
      (void)comp;                                                                                  \
      kx::radix_sort(data, data + num_elems);                                                      \
    }                                                                                              \
  };

KX_SORT_KEYS(short int);
KX_SORT_KEYS(unsigned short int);
KX_SORT_KEYS(int);
KX_SORT_KEYS(unsigned int);
KX_SORT_KEYS(long int);
KX_SORT_KEYS(unsigned long int);
KX_SORT_KEYS(long long int);
KX_SORT_KEYS(unsigned long long int);
KX_SORT_KEYS(unsigned char);
KX_SORT_KEYS(signed char);
KX_SORT_KEYS(char);
KX_SORT_KEYS(char16_t);
KX_SORT_KEYS(char32_t);
KX_SORT_KEYS(wchar_t);

#undef KX_SORT_KEYS

template <typename T, typename CompareType>
bool use_serial_sort_keys(T* data, size_t num_elems, const CompareType& comp)
{
  size_t total_bytes = (num_elems) * sizeof(T);
  if (total_bytes < MIN_BYTES_FOR_PARALLEL)
  {
    run_kx_radix_sort_keys<T, CompareType>::run(data, num_elems, comp);
    return true;
  }
  return false;
}

// Generate radix sort interfaces for key and key value sorts.
#define VTKM_TBB_SORT_EXPORT(key_type)                                                             \
  void parallel_radix_sort_key_values(                                                             \
    key_type* keys, vtkm::Id* vals, size_t num_elems, const std::greater<key_type>& comp)          \
  {                                                                                                \
    PairSort<key_type, vtkm::Id, std::greater<key_type>> ps;                                       \
    ps.InitAndSort(keys, vals, num_elems, comp);                                                   \
  }                                                                                                \
  void parallel_radix_sort_key_values(                                                             \
    key_type* keys, vtkm::Id* vals, size_t num_elems, const std::less<key_type>& comp)             \
  {                                                                                                \
    PairSort<key_type, vtkm::Id, std::less<key_type>> ps;                                          \
    ps.InitAndSort(keys, vals, num_elems, comp);                                                   \
  }                                                                                                \
  void parallel_radix_sort(key_type* data, size_t num_elems, const std::greater<key_type>& comp)   \
  {                                                                                                \
    if (!use_serial_sort_keys(data, num_elems, comp))                                              \
    {                                                                                              \
      KeySort<key_type, std::greater<key_type>> ks;                                                \
      ks.InitAndSort(data, num_elems, comp);                                                       \
    }                                                                                              \
  }                                                                                                \
  void parallel_radix_sort(key_type* data, size_t num_elems, const std::less<key_type>& comp)      \
  {                                                                                                \
    if (!use_serial_sort_keys(data, num_elems, comp))                                              \
    {                                                                                              \
      KeySort<key_type, std::less<key_type>> ks;                                                   \
      ks.InitAndSort(data, num_elems, comp);                                                       \
    }                                                                                              \
  }


VTKM_TBB_SORT_EXPORT(short int);
VTKM_TBB_SORT_EXPORT(unsigned short int);
VTKM_TBB_SORT_EXPORT(int);
VTKM_TBB_SORT_EXPORT(unsigned int);
VTKM_TBB_SORT_EXPORT(long int);
VTKM_TBB_SORT_EXPORT(unsigned long int);
VTKM_TBB_SORT_EXPORT(long long int);
VTKM_TBB_SORT_EXPORT(unsigned long long int);
VTKM_TBB_SORT_EXPORT(unsigned char);
VTKM_TBB_SORT_EXPORT(signed char);
VTKM_TBB_SORT_EXPORT(char);
VTKM_TBB_SORT_EXPORT(float);
VTKM_TBB_SORT_EXPORT(double);

#undef VTKM_TBB_SORT_EXPORT

VTKM_THIRDPARTY_POST_INCLUDE
}
}
}
}
