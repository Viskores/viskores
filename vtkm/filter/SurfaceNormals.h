//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_filter_SurfaceNormals_h
#define vtk_m_filter_SurfaceNormals_h

#include <vtkm/filter/FilterCell.h>

namespace vtkm
{
namespace filter
{

class SurfaceNormals : public vtkm::filter::FilterCell<SurfaceNormals>
{
public:
  using vtkm::filter::FilterCell<SurfaceNormals>::Execute;

  VTKM_CONT
  vtkm::filter::ResultField Execute(const vtkm::cont::DataSet& input);

  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  vtkm::filter::ResultField DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& points,
    const vtkm::filter::FieldMetadata& fieldMeta,
    const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
    const DeviceAdapter& device);
};

template <>
class FilterTraits<SurfaceNormals>
{
public:
  using InputFieldTypeList =
    vtkm::ListTagBase<vtkm::Vec<vtkm::Float32, 3>, vtkm::Vec<vtkm::Float64, 3>>;
};
}
} // vtkm::filter

#include <vtkm/filter/SurfaceNormals.hxx>

#endif // vtk_m_filter_SurfaceNormals_h
