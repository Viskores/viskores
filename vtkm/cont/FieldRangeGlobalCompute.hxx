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
#ifndef vtk_m_cont_FieldRangeGlobalCompute_hxx
#define vtk_m_cont_FieldRangeGlobalCompute_hxx

namespace vtkm
{
namespace cont
{
namespace detail
{

VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> MergeRangesGlobal(
  const vtkm::cont::ArrayHandle<vtkm::Range>& range);

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalComputeImpl(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList,
  StorageList)
{
  auto lrange = vtkm::cont::FieldRangeCompute(dataset, name, assoc, TypeList(), StorageList());
  return vtkm::cont::detail::MergeRangesGlobal(lrange);
}

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalComputeImpl(
  const vtkm::cont::MultiBlock& multiblock,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList,
  StorageList)
{
  auto lrange = vtkm::cont::FieldRangeCompute(multiblock, name, assoc, TypeList(), StorageList());
  return vtkm::cont::detail::MergeRangesGlobal(lrange);
}
}
}
}

#endif
