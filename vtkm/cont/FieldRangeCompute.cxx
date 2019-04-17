//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/FieldRangeCompute.h>
#include <vtkm/cont/FieldRangeCompute.hxx>

#include <vtkm/cont/Algorithm.h>

namespace vtkm
{
namespace cont
{

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(const vtkm::cont::DataSet& dataset,
                                                       const std::string& name,
                                                       vtkm::cont::Field::Association assoc)
{
  return vtkm::cont::detail::FieldRangeComputeImpl(
    dataset, name, assoc, VTKM_DEFAULT_TYPE_LIST_TAG());
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(const vtkm::cont::MultiBlock& multiblock,
                                                       const std::string& name,
                                                       vtkm::cont::Field::Association assoc)
{
  return vtkm::cont::detail::FieldRangeComputeImpl(
    multiblock, name, assoc, VTKM_DEFAULT_TYPE_LIST_TAG());
}
}
} // namespace vtkm::cont
