//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_cont_tbb_internal_ParallelSort_h
#define vtk_m_cont_tbb_internal_ParallelSort_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleZip.h>

#include <vtkm/cont/tbb/internal/ArrayManagerExecutionTBB.h>
#include <vtkm/cont/tbb/internal/DeviceAdapterTagTBB.h>
#include <vtkm/cont/tbb/internal/FunctorsTBB.h>
#include <vtkm/cont/tbb/internal/ParallelSortTBB.hxx>

#include <type_traits>

namespace vtkm
{
namespace cont
{
namespace tbb
{
namespace internal
{
struct RadixSortTag
{
};
struct PSortTag
{
};

template <typename T>
struct is_valid_compare_type : std::integral_constant<bool, false>
{
};
template <typename T>
struct is_valid_compare_type<std::less<T>> : std::integral_constant<bool, true>
{
};
template <typename T>
struct is_valid_compare_type<std::greater<T>> : std::integral_constant<bool, true>
{
};
template <>
struct is_valid_compare_type<vtkm::SortLess> : std::integral_constant<bool, true>
{
};
template <>
struct is_valid_compare_type<vtkm::SortGreater> : std::integral_constant<bool, true>
{
};
template <typename BComp, typename T>
BComp&& get_std_compare(BComp&& b, T&&)
{
  return std::forward<BComp>(b);
}
template <typename T>
std::less<T> get_std_compare(vtkm::SortLess, T&&)
{
  return std::less<T>{};
}
template <typename T>
std::greater<T> get_std_compare(vtkm::SortGreater, T&&)
{
  return std::greater<T>{};
}


template <typename T, typename StorageTag, typename BinaryCompare>
struct sort_tag_type
{
  using type = PSortTag;
};
template <typename T, typename BinaryCompare>
struct sort_tag_type<T, vtkm::cont::StorageTagBasic, BinaryCompare>
{
  using PrimT = std::is_arithmetic<T>;
  using BComp = is_valid_compare_type<BinaryCompare>;
  using type =
    typename std::conditional<PrimT::value && BComp::value, RadixSortTag, PSortTag>::type;
};

template <typename T, typename U, typename StorageTagT, typename StorageTagU, class BinaryCompare>
struct sortbykey_tag_type
{
  using type = PSortTag;
};
template <typename T, typename U, typename BinaryCompare>
struct sortbykey_tag_type<T,
                          U,
                          vtkm::cont::StorageTagBasic,
                          vtkm::cont::StorageTagBasic,
                          BinaryCompare>
{
  using PrimT = std::is_arithmetic<T>;
  using PrimU = std::is_arithmetic<U>;
  using BComp = is_valid_compare_type<BinaryCompare>;
  using type = typename std::conditional<PrimT::value && PrimU::value && BComp::value,
                                         RadixSortTag,
                                         PSortTag>::type;
};


#define VTKM_TBB_SORT_EXPORT(key_type)                                                             \
  VTKM_CONT_EXPORT void parallel_radix_sort(                                                       \
    key_type* data, size_t num_elems, const std::greater<key_type>& comp);                         \
  VTKM_CONT_EXPORT void parallel_radix_sort(                                                       \
    key_type* data, size_t num_elems, const std::less<key_type>& comp);                            \
  VTKM_CONT_EXPORT void parallel_radix_sort_key_values(                                            \
    key_type* keys, vtkm::Id* vals, size_t num_elems, const std::greater<key_type>& comp);         \
  VTKM_CONT_EXPORT void parallel_radix_sort_key_values(                                            \
    key_type* keys, vtkm::Id* vals, size_t num_elems, const std::less<key_type>& comp);

// Generate radix sort interfaces for key and key value sorts.
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

template <typename T, typename Container, class BinaryCompare>
void parallel_sort(vtkm::cont::ArrayHandle<T, Container>& values, BinaryCompare binary_compare)
{
  using SortAlgorithmTag = typename sort_tag_type<T, Container, BinaryCompare>::type;
  parallel_sort(values, binary_compare, SortAlgorithmTag{});
}
template <typename HandleType, class BinaryCompare>
void parallel_sort(HandleType& values, BinaryCompare binary_compare, PSortTag)
{
  auto arrayPortal = values.PrepareForInPlace(vtkm::cont::DeviceAdapterTagTBB());

  using IteratorsType = vtkm::cont::ArrayPortalToIterators<decltype(arrayPortal)>;
  IteratorsType iterators(arrayPortal);

  internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
  ::tbb::parallel_sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
}
template <typename T, typename StorageT, class BinaryCompare>
void parallel_sort(vtkm::cont::ArrayHandle<T, StorageT>& values,
                   BinaryCompare binary_compare,
                   RadixSortTag)
{
  auto c = get_std_compare(binary_compare, T{});
  parallel_radix_sort(
    values.GetStorage().GetArray(), static_cast<std::size_t>(values.GetNumberOfValues()), c);
}

template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare)
{
  using SortAlgorithmTag =
    typename sortbykey_tag_type<T, U, StorageT, StorageU, BinaryCompare>::type;
  parallel_sort_bykey(keys, values, binary_compare, SortAlgorithmTag{});
}
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         PSortTag)
{
  using KeyType = vtkm::cont::ArrayHandle<T, StorageT>;
  VTKM_CONSTEXPR bool larger_than_64bits = sizeof(U) > sizeof(vtkm::Int64);
  if (larger_than_64bits)
  {
    /// More efficient sort:
    /// Move value indexes when sorting and reorder the value array at last

    using ValueType = vtkm::cont::ArrayHandle<U, StorageU>;
    using IndexType = vtkm::cont::ArrayHandle<vtkm::Id>;
    using ZipHandleType = vtkm::cont::ArrayHandleZip<KeyType, IndexType>;

    IndexType indexArray;
    ValueType valuesScattered;
    const vtkm::Id size = values.GetNumberOfValues();

    {
      auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
      auto inputPortal = handle.PrepareForInput(DeviceAdapterTagTBB());
      auto outputPortal =
        indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagTBB());
      tbb::CopyPortals(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
    }

    ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  vtkm::cont::internal::KeyCompare<T, vtkm::Id, BinaryCompare>(binary_compare),
                  PSortTag());

    tbb::ScatterPortal(values.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                       indexArray.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                       valuesScattered.PrepareForOutput(size, vtkm::cont::DeviceAdapterTagTBB()));

    {
      auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagTBB());
      auto outputPortal =
        values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagTBB());
      tbb::CopyPortals(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
    }
  }
  else
  {
    using ValueType = vtkm::cont::ArrayHandle<U, StorageU>;
    using ZipHandleType = vtkm::cont::ArrayHandleZip<KeyType, ValueType>;

    ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, values);
    parallel_sort(
      zipHandle, vtkm::cont::internal::KeyCompare<T, U, BinaryCompare>(binary_compare), PSortTag{});
  }
}
template <typename T, typename StorageT, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<vtkm::Id, StorageU>& values,
                         BinaryCompare binary_compare,
                         RadixSortTag)
{
  auto c = get_std_compare(binary_compare, T{});
  parallel_radix_sort_key_values(keys.GetStorage().GetArray(),
                                 values.GetStorage().GetArray(),
                                 static_cast<std::size_t>(keys.GetNumberOfValues()),
                                 c);
}
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         RadixSortTag)
{
  using KeyType = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>;
  using ValueType = vtkm::cont::ArrayHandle<U, vtkm::cont::StorageTagBasic>;
  using IndexType = vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>;
  using ZipHandleType = vtkm::cont::ArrayHandleZip<KeyType, IndexType>;

  IndexType indexArray;
  ValueType valuesScattered;
  const vtkm::Id size = values.GetNumberOfValues();

  {
    auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
    auto inputPortal = handle.PrepareForInput(DeviceAdapterTagTBB());
    auto outputPortal =
      indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagTBB());
    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
  }

  if (static_cast<vtkm::Id>(sizeof(T)) * keys.GetNumberOfValues() > 400000)
  {
    parallel_sort_bykey(keys, indexArray, binary_compare);
  }
  else
  {
    ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  vtkm::cont::internal::KeyCompare<T, vtkm::Id, BinaryCompare>(binary_compare),
                  PSortTag{});
  }

  tbb::ScatterPortal(values.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                     indexArray.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),
                     valuesScattered.PrepareForOutput(size, vtkm::cont::DeviceAdapterTagTBB()));

  {
    auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagTBB());
    auto outputPortal =
      values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagTBB());
    tbb::CopyPortals(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
  }
}
}
}
}
}

#endif // vtk_m_cont_tbb_internal_ParallelSort_h
