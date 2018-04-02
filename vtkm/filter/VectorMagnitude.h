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

#ifndef vtk_m_filter_VectorMagnitude_h
#define vtk_m_filter_VectorMagnitude_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/Magnitude.h>

namespace vtkm
{
namespace filter
{

class VectorMagnitude : public vtkm::filter::FilterField<VectorMagnitude>
{
public:
  VTKM_CONT
  VectorMagnitude();

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                          const DeviceAdapter& tag);

private:
  vtkm::worklet::Magnitude Worklet;
};

template <>
class FilterTraits<VectorMagnitude>
{ //currently the VectorMagnitude filter only works on vector data.
public:
  using InputFieldTypeList = TypeListTagVecCommon;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/VectorMagnitude.hxx>

#endif // vtk_m_filter_VectorMagnitude_h
