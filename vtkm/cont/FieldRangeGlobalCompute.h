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
#ifndef vtk_m_cont_FieldRangeGlobalCompute_h
#define vtk_m_cont_FieldRangeGlobalCompute_h

#include <vtkm/cont/FieldRangeCompute.h>

#include <vtkm/cont/FieldRangeGlobalCompute.hxx>

namespace vtkm
{
namespace cont
{
/// \brief utility functions to compute global ranges for dataset fields.
///
/// These functions compute global ranges for fields in a dataset or a multiblock.
/// In non-distributed environments, this is exactly same as `FieldRangeCompute`. In
/// distributed environments, however, the range is computed locally on each rank
/// and then a reduce-all collective is performed to reduces the ranges on all ranks.

//{@
/// Returns the range for a field from a dataset. If the field is not present, an empty
/// ArrayHandle will be returned.
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::ANY);

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList,
  StorageList)
{
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);

  return detail::FieldRangeGlobalComputeImpl(dataset, name, assoc, TypeList(), StorageList());
}

template <typename TypeList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList)
{
  VTKM_IS_LIST_TAG(TypeList);
  return detail::FieldRangeGlobalComputeImpl(
    dataset, name, assoc, TypeList(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}
//@}

//{@
/// Returns the range for a field from a multiblock. If the field is not present on any
/// of the blocks, an empty ArrayHandle will be returned. If the field is present on some blocks,
/// but not all, those blocks without the field are skipped.
///
/// The returned array handle will have as many values as the maximum number of components for
/// the selected field across all blocks.
VTKM_CONT_EXPORT
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::MultiBlock& multiblock,
  const std::string& name,
  vtkm::cont::Field::Association assoc = vtkm::cont::Field::Association::ANY);

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::MultiBlock& multiblock,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList,
  StorageList)
{
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);

  return detail::FieldRangeGlobalComputeImpl(multiblock, name, assoc, TypeList(), StorageList());
}

template <typename TypeList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalCompute(
  const vtkm::cont::MultiBlock& multiblock,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList)
{
  VTKM_IS_LIST_TAG(TypeList);
  return detail::FieldRangeGlobalComputeImpl(
    multiblock, name, assoc, TypeList(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}
//@}
}
} // namespace vtkm::cont

#endif
