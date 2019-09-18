//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/openmp/internal/ArrayManagerExecutionOpenMP.h>
#include <vtkm/cont/openmp/internal/FunctorsOpenMP.h>
#include <vtkm/cont/openmp/internal/ParallelQuickSortOpenMP.h>
#include <vtkm/cont/openmp/internal/ParallelRadixSortOpenMP.h>

#include <vtkm/BinaryPredicates.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleZip.h>

#include <omp.h>

namespace vtkm
{
namespace cont
{
namespace openmp
{
namespace sort
{

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
  auto portal = values.PrepareForInPlace(DeviceAdapterTagOpenMP());
  auto iter = vtkm::cont::ArrayPortalToIteratorBegin(portal);
  vtkm::Id2 range(0, values.GetNumberOfValues());

  using IterType = typename std::decay<decltype(iter)>::type;
  using Sorter = quick::QuickSorter<IterType, BinaryCompare>;

  Sorter sorter(iter, binary_compare);
  sorter.Execute(range);
}

// Radix sort values:
template <typename T, typename StorageT, class BinaryCompare>
void parallel_sort(vtkm::cont::ArrayHandle<T, StorageT>& values,
                   BinaryCompare binary_compare,
                   vtkm::cont::internal::radix::RadixSortTag)
{
  auto c = vtkm::cont::internal::radix::get_std_compare(binary_compare, T{});
  radix::parallel_radix_sort(
    values.GetStorage().GetArray(), static_cast<std::size_t>(values.GetNumberOfValues()), c);
}

// Value sort -- static switch between quicksort & radix sort
template <typename T, typename Container, class BinaryCompare>
void parallel_sort(vtkm::cont::ArrayHandle<T, Container>& values, BinaryCompare binary_compare)
{
  using namespace vtkm::cont::internal::radix;
  using SortAlgorithmTag = typename sort_tag_type<T, Container, BinaryCompare>::type;

  parallel_sort(values, binary_compare, SortAlgorithmTag{});
}

// Quicksort by key:
template <typename T, typename StorageT, typename U, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<U, StorageU>& values,
                         BinaryCompare binary_compare,
                         vtkm::cont::internal::radix::PSortTag)
{
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

    // Generate an in-memory index array:
    {
      auto handle = ArrayHandleIndex(keys.GetNumberOfValues());
      auto inputPortal = handle.PrepareForInput(DeviceAdapterTagOpenMP());
      auto outputPortal =
        indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagOpenMP());
      openmp::CopyHelper(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
    }

    // Sort the keys and indices:
    ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  vtkm::cont::internal::KeyCompare<T, vtkm::Id, BinaryCompare>(binary_compare),
                  vtkm::cont::internal::radix::PSortTag());

    // Permute the values to their sorted locations:
    {
      auto valuesInPortal = values.PrepareForInput(DeviceAdapterTagOpenMP());
      auto indexPortal = indexArray.PrepareForInput(DeviceAdapterTagOpenMP());
      auto valuesOutPortal = valuesScattered.PrepareForOutput(size, DeviceAdapterTagOpenMP());

      VTKM_OPENMP_DIRECTIVE(parallel for
                            default(none)
                            firstprivate(valuesInPortal, indexPortal, valuesOutPortal)
                            schedule(static)
                            VTKM_OPENMP_SHARED_CONST(size))
      for (vtkm::Id i = 0; i < size; ++i)
      {
        valuesOutPortal.Set(i, valuesInPortal.Get(indexPortal.Get(i)));
      }
    }

    // Copy the values back into the input array:
    {
      auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagOpenMP());
      auto outputPortal =
        values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagOpenMP());
      openmp::CopyHelper(inputPortal, outputPortal, 0, 0, size);
    }
  }
  else
  {
    using ValueType = vtkm::cont::ArrayHandle<U, StorageU>;
    using ZipHandleType = vtkm::cont::ArrayHandleZip<KeyType, ValueType>;

    ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, values);
    parallel_sort(zipHandle,
                  vtkm::cont::internal::KeyCompare<T, U, BinaryCompare>(binary_compare),
                  vtkm::cont::internal::radix::PSortTag{});
  }
}

// Radix sort by key:
template <typename T, typename StorageT, typename StorageU, class BinaryCompare>
void parallel_sort_bykey(vtkm::cont::ArrayHandle<T, StorageT>& keys,
                         vtkm::cont::ArrayHandle<vtkm::Id, StorageU>& values,
                         BinaryCompare binary_compare,
                         vtkm::cont::internal::radix::RadixSortTag)
{
  using namespace vtkm::cont::internal::radix;
  auto c = get_std_compare(binary_compare, T{});
  radix::parallel_radix_sort_key_values(keys.GetStorage().GetArray(),
                                        values.GetStorage().GetArray(),
                                        static_cast<std::size_t>(keys.GetNumberOfValues()),
                                        c);
}
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
    auto inputPortal = handle.PrepareForInput(DeviceAdapterTagOpenMP());
    auto outputPortal =
      indexArray.PrepareForOutput(keys.GetNumberOfValues(), DeviceAdapterTagOpenMP());
    openmp::CopyHelper(inputPortal, outputPortal, 0, 0, keys.GetNumberOfValues());
  }

  const vtkm::Id valuesBytes = static_cast<vtkm::Id>(sizeof(T)) * keys.GetNumberOfValues();
  if (valuesBytes > static_cast<vtkm::Id>(vtkm::cont::internal::radix::MIN_BYTES_FOR_PARALLEL))
  {
    parallel_sort_bykey(keys, indexArray, binary_compare);
  }
  else
  {
    ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, indexArray);
    parallel_sort(zipHandle,
                  vtkm::cont::internal::KeyCompare<T, vtkm::Id, BinaryCompare>(binary_compare),
                  vtkm::cont::internal::radix::PSortTag());
  }

  // Permute the values to their sorted locations:
  {
    auto valuesInPortal = values.PrepareForInput(DeviceAdapterTagOpenMP());
    auto indexPortal = indexArray.PrepareForInput(DeviceAdapterTagOpenMP());
    auto valuesOutPortal = valuesScattered.PrepareForOutput(size, DeviceAdapterTagOpenMP());

    VTKM_OPENMP_DIRECTIVE(parallel for
                          default(none)
                          firstprivate(valuesInPortal, indexPortal, valuesOutPortal)
                          VTKM_OPENMP_SHARED_CONST(size)
                          schedule(static))
    for (vtkm::Id i = 0; i < size; ++i)
    {
      valuesOutPortal.Set(i, valuesInPortal.Get(indexPortal.Get(i)));
    }
  }

  {
    auto inputPortal = valuesScattered.PrepareForInput(DeviceAdapterTagOpenMP());
    auto outputPortal =
      values.PrepareForOutput(valuesScattered.GetNumberOfValues(), DeviceAdapterTagOpenMP());
    openmp::CopyHelper(inputPortal, outputPortal, 0, 0, valuesScattered.GetNumberOfValues());
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
} // end namespace vtkm::cont::openmp::sort
