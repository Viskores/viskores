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

#ifndef vtk_m_filter_VertexClustering_h
#define vtk_m_filter_VertexClustering_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/VertexClustering.h>

namespace vtkm
{
namespace filter
{

class VertexClustering : public vtkm::filter::FilterDataSet<VertexClustering>
{
public:
  VTKM_CONT
  VertexClustering();

  VTKM_CONT
  void SetNumberOfDivisions(const vtkm::Id3& num) { this->NumberOfDivisions = num; }

  VTKM_CONT
  const vtkm::Id3& GetNumberOfDivisions() const { return this->NumberOfDivisions; }

  template <typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT vtkm::filter::ResultDataSet DoExecute(
    const vtkm::cont::DataSet& input, const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
    const DeviceAdapter& tag);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT bool DoMapField(vtkm::filter::ResultDataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                            const DeviceAdapter& tag);

private:
  vtkm::Id3 NumberOfDivisions;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/VertexClustering.hxx>

#endif // vtk_m_filter_VertexClustering_h
