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

VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> MergeRanges(const vtkm::cont::ArrayHandle<vtkm::Range>& a,
                                                 const vtkm::cont::ArrayHandle<vtkm::Range>& b);

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeComputeImpl(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::AssociationEnum assoc,
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
  vtkm::cont::Field::AssociationEnum assoc,
  TypeList,
  StorageList)
{
  return std::accumulate(
    multiblock.begin(),
    multiblock.end(),
    vtkm::cont::ArrayHandle<vtkm::Range>(),
    [&](const vtkm::cont::ArrayHandle<vtkm::Range>& val, const vtkm::cont::DataSet& dataset) {
      auto cur_range =
        vtkm::cont::detail::FieldRangeComputeImpl(dataset, name, assoc, TypeList(), StorageList());
      return vtkm::cont::detail::MergeRanges(val, cur_range);
    });
}
}
}
} //  namespace vtkm::cont::detail

#endif
