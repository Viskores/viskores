//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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

template <typename TypeList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalComputeImpl(
  const vtkm::cont::DataSet& dataset,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList)
{
  auto lrange = vtkm::cont::FieldRangeCompute(dataset, name, assoc, TypeList());
  return vtkm::cont::detail::MergeRangesGlobal(lrange);
}

template <typename TypeList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeGlobalComputeImpl(
  const vtkm::cont::PartitionedDataSet& pds,
  const std::string& name,
  vtkm::cont::Field::Association assoc,
  TypeList)
{
  auto lrange = vtkm::cont::FieldRangeCompute(pds, name, assoc, TypeList());
  return vtkm::cont::detail::MergeRangesGlobal(lrange);
}
}
}
}

#endif
