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
                                                       vtkm::cont::Field::AssociationEnum assoc)
{
  return vtkm::cont::detail::FieldRangeComputeImpl(
    dataset, name, assoc, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> FieldRangeCompute(const vtkm::cont::MultiBlock& multiblock,
                                                       const std::string& name,
                                                       vtkm::cont::Field::AssociationEnum assoc)
{
  return vtkm::cont::detail::FieldRangeComputeImpl(
    multiblock, name, assoc, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

//-----------------------------------------------------------------------------
namespace detail
{
VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> MergeRanges(const vtkm::cont::ArrayHandle<vtkm::Range>& a,
                                                 const vtkm::cont::ArrayHandle<vtkm::Range>& b)
{
  const auto num_vals_a = a.GetNumberOfValues();
  const auto num_vals_b = b.GetNumberOfValues();
  if (num_vals_b == 0)
  {
    return a;
  }
  if (num_vals_a == 0)
  {
    return b;
  }

  vtkm::cont::ArrayHandle<vtkm::Range> result;
  vtkm::cont::Algorithm::Transform(a, b, result, vtkm::Add());
  return result;
}
} // namespace detail
}
} // namespace vtkm::cont
