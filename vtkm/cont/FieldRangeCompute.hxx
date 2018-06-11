//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_FieldRangeCompute_hxx
#define vtk_m_cont_FieldRangeCompute_hxx

#include <numeric> // for std::accumulate

namespace vtkm
{
namespace cont
{
namespace detail
{

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeComputeImpl(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList,
  StorageList)
{
  vtkm::cont::Field field;
  try
  {
    field = dataset.GetField(name, assoc);
  }
  catch (vtkm::cont::ErrorBadValue&)
  {
    // field missing, return empty range.
    return vtkm::cont::ArrayHandle<vtkm::Range>();
  }

  return field.GetRange(TypeList(), StorageList());
}

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeComputeImpl(
  const vtkm::cont::MultiBlock& multiblock,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList,
  StorageList)
{
  std::vector<vtkm::Range> result_vector = std::accumulate(
    multiblock.begin(),
    multiblock.end(),
    std::vector<vtkm::Range>(),
    [&](const std::vector<vtkm::Range>& accumulated_value, const vtkm::cont::DataSet& dataset) {
      vtkm::cont::ArrayHandle<vtkm::Range> block_range =
        vtkm::cont::detail::FieldRangeComputeImpl(dataset, name, assoc, TypeList(), StorageList());

      std::vector<vtkm::Range> result = accumulated_value;

      // if the current block has more components than we have seen so far,
      // resize the result to fit all components.
      result.resize(std::max(result.size(), static_cast<size_t>(block_range.GetNumberOfValues())));

      auto portal = block_range.GetPortalConstControl();
      std::transform(vtkm::cont::ArrayPortalToIteratorBegin(portal),
                     vtkm::cont::ArrayPortalToIteratorEnd(portal),
                     result.begin(),
                     result.begin(),
                     std::plus<vtkm::Range>());
      return result;
    });

  return vtkm::cont::make_ArrayHandle(result_vector, vtkm::CopyFlag::On);
}
}
}
} //  namespace vtkm::cont::detail

#endif
