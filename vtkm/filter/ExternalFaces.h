//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_filter_ExternalFaces_h
#define vtk_m_filter_ExternalFaces_h

#include <vtkm/filter/DataSetFilter.h>
#include <vtkm/worklet/ExternalFaces.h>

namespace vtkm {
namespace filter {

class ExternalFaces : public vtkm::filter::DataSetFilter<ExternalFaces>
{
public:
  VTKM_CONT_EXPORT
  ExternalFaces();

  template<typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  vtkm::filter::DataSetResult DoExecute(const vtkm::cont::DataSet& input,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT_EXPORT
  bool DoMapField(vtkm::filter::DataSetResult& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  const DeviceAdapter& tag);
};

}
} // namespace vtkm::filter


#include <vtkm/filter/ExternalFaces.hxx>

#endif // vtk_m_filter_Threshold_h