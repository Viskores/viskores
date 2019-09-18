//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_tbb_internal_ParallelSort_h
#define vtk_m_cont_tbb_internal_ParallelSort_h

#include <vtkm/BinaryPredicates.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/internal/ParallelRadixSortInterface.h>

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
namespace sort
{

// Declare the compiled radix sort specializations:
VTKM_DECLARE_RADIX_SORT()

// Forward declare entry points (See stack overflow discussion 7255281 --
// templated overloads of template functions are not specialization, and will
// be resolved during the first phase of two part lookup).
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(vtkm::cont::ArrayHandle<T, Container>&, BinaryCompare);
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>&,
                         vtkm::cont::ArrayHandle<U, StorageU>&,
                         BinaryCompare);

// Quicksort values:
template <typename HandleType, class BinaryCompare>
void parallel_sort(HandleType& values,
                   BinaryCompare binary_compare,
                   vtkm::cont::internal::radix::PSortTag)
{
  auto arrayPortal = values.PrepareForInPlace(vtkm::cont::DeviceAdapterTagTBB());

  using IteratorsType = vtkm::cont::ArrayPortalToIterators<decltype(arrayPortal)>;
  IteratorsType iterators(arrayPortal);

  internal::WrappedBinaryOperator<bool, BinaryCompare> wrappedCompare(binary_compare);
  ::tbb::parallel_sort(iterators.GetBegin(), iterators.GetEnd(), wrappedCompare);
}

// Radix sort values:
template <typename T, typename StorageT, class BinaryCompare>
void parallel_sort(vtkm::cont::ArrayHandle<T, StorageT>& values,
                   BinaryCompare binary_compare,
                   vtkm::cont::internal::radix::RadixSortTag)
{
  using namespace vtkm::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  parallel_radix_sort(
    values.GetStorage().GetArray(), static_cast<std::size_t>(values.GetNumberOfValues()), c);
}

// Value sort -- static switch between quicksort and radix sort
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(vtkm::cont::ArrayHandle<T, Container>& values, BinaryCompare binary_compare)
{
  using namespace vtkm::cont::internal::radix;
  using SortAlgorithmTag = typename sort_tag_type<T, Container, BinaryCompare>::type;
  parallel_sort(values, binary_compare, SortAlgorithmTag{});
}


// Quicksort by key
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         vtkm::cont::internal::radix::PSortTag)
{
  using namespace vtkm::cont::internal::radix;
  using KeyType = vtkm::cont::ArrayHandle<T, StorageT>;
  constexpr bool larger_than_64bits = sizeof(U) > sizeof(vtkm::Int64);
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

// Radix sort by key -- Specialize for vtkm::Id values:
template <typename T, typename StorageT, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<vtkm::Id, StorageU>& values,
                         BinaryCompare binary_compare,
                         vtkm::cont::internal::radix::RadixSortTag)
{
  using namespace vtkm::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  parallel_radix_sort_key_values(keys.GetStorage().GetArray(),
                                 values.GetStorage().GetArray(),
                                 static_cast<std::size_t>(keys.GetNumberOfValues()),
                                 c);
}

// Radix sort by key -- Generic impl:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         vtkm::cont::internal::radix::RadixSortTag)
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
                  vtkm::cont::internal::radix::PSortTag{});
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

// Sort by key -- static switch between radix and quick sort:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare)
{
  using namespace vtkm::cont::internal::radix;
  using SortAlgorithmTag =
    typename sortbykey_tag_type<T, U, StorageT, StorageU, BinaryCompare>::type;
  parallel_sort_bykey(keys, values, binary_compare, SortAlgorithmTag{});
}
}
}
}
} // end namespace vtkm::cont::tbb::sort

#endif // vtk_m_cont_tbb_internal_ParallelSort_h
