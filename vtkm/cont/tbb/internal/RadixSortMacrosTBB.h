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

#ifndef vtk_m_cont_tbb_internal_RadixSortMacrosTBB_h
#define vtk_m_cont_tbb_internal_RadixSortMacrosTBB_h

#define VTKM_INSTANTIATE_RADIX_SORT_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, Compare)                 \
  template <>                                                                                      \
  void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::Sort(                              \
    vtkm::cont::ArrayHandle<Type, vtkm::cont::StorageTagBasic>& values, Compare binary_compare)    \
  {                                                                                                \
    ::parallel_radix_sort_tbb::parallel_radix_sort(                                                \
      values.GetStorage().GetArray(), values.GetNumberOfValues(), binary_compare);                 \
  }                                                                                                \
  void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::RadixSortByKey(                    \
    vtkm::cont::ArrayHandle<Type, vtkm::cont::StorageTagBasic>& keys,                              \
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& values,                        \
    Compare comp)                                                                                  \
  {                                                                                                \
    ::parallel_radix_sort_tbb::parallel_radix_sort_key_values(keys.GetStorage().GetArray(),        \
                                                              values.GetStorage().GetArray(),      \
                                                              keys.GetNumberOfValues(),            \
                                                              comp);                               \
  }

#define VTKM_INSTANTIATE_RADIX_SORT_FOR_VALUE_TYPE(Type)                                           \
  VTKM_INSTANTIATE_RADIX_SORT_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, std::less<Type>)               \
  VTKM_INSTANTIATE_RADIX_SORT_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, std::greater<Type>)

#define VTKM_DECLARE_RADIX_SORT_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, Compare)                     \
  template <>                                                                                      \
  void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::Sort(                              \
    vtkm::cont::ArrayHandle<Type, vtkm::cont::StorageTagBasic>& values, Compare binary_compare);

#define VTKM_DECLARE_RADIX_SORT_FOR_VALUE_TYPE(Type)                                               \
  VTKM_DECLARE_RADIX_SORT_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, std::less<Type>)                   \
  VTKM_DECLARE_RADIX_SORT_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, std::greater<Type>)

#define VTKM_DECLARE_RADIX_SORT_BY_KEY_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, Compare)              \
  VTKM_CONT_EXPORT static void RadixSortByKey(                                                     \
    vtkm::cont::ArrayHandle<Type, vtkm::cont::StorageTagBasic>& keys,                              \
    vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>& values,                        \
    Compare comp);                                                                                 \
                                                                                                   \
  template <typename U>                                                                            \
  VTKM_CONT_EXPORT static void SortByKey(                                                          \
    vtkm::cont::ArrayHandle<Type, vtkm::cont::StorageTagBasic>& keys,                              \
    vtkm::cont::ArrayHandle<U, vtkm::cont::StorageTagBasic>& values,                               \
    Compare comp)                                                                                  \
  {                                                                                                \
    using KeyType = vtkm::cont::ArrayHandle<Type, vtkm::cont::StorageTagBasic>;                    \
    using ValueType = vtkm::cont::ArrayHandle<U, vtkm::cont::StorageTagBasic>;                     \
    using IndexType = vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic>;              \
    using ZipHandleType = vtkm::cont::ArrayHandleZip<KeyType, IndexType>;                          \
                                                                                                   \
    IndexType indexArray;                                                                          \
    ValueType valuesScattered;                                                                     \
    const vtkm::Id size = values.GetNumberOfValues();                                              \
                                                                                                   \
    Copy(ArrayHandleIndex(keys.GetNumberOfValues()), indexArray);                                  \
                                                                                                   \
    if (sizeof(Type) * keys.GetNumberOfValues() > 400000)                                          \
    {                                                                                              \
      RadixSortByKey(keys, indexArray, comp);                                                      \
    }                                                                                              \
    else                                                                                           \
    {                                                                                              \
      ZipHandleType zipHandle = vtkm::cont::make_ArrayHandleZip(keys, indexArray);                 \
      Sort(zipHandle, vtkm::cont::internal::KeyCompare<Type, vtkm::Id, Compare>(comp));            \
    }                                                                                              \
                                                                                                   \
    tbb::ScatterPortal(values.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),                  \
                       indexArray.PrepareForInput(vtkm::cont::DeviceAdapterTagTBB()),              \
                       valuesScattered.PrepareForOutput(size, vtkm::cont::DeviceAdapterTagTBB())); \
                                                                                                   \
    Copy(valuesScattered, values);                                                                 \
  }

#define VTKM_DECLARE_RADIX_SORT_BY_KEY_FOR_VALUE_TYPE(Type)                                        \
  VTKM_DECLARE_RADIX_SORT_BY_KEY_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, std::less<Type>)            \
  VTKM_DECLARE_RADIX_SORT_BY_KEY_FOR_VALUE_TYPE_AND_COMPARE_TYPE(Type, std::greater<Type>)

#endif // vtk_m_cont_tbb_internal_RadixSortMacrosTBB_h
