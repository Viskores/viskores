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

#ifndef vtk_m_filter_ExtractStructured_h
#define vtk_m_filter_ExtractStructured_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/ExtractStructured.h>

namespace vtkm {
namespace filter {

class ExtractStructured : public vtkm::filter::FilterDataSet<ExtractStructured>
{
public:
  VTKM_CONT
  ExtractStructured();

  // Set the bounding box for the volume of interest
  VTKM_CONT
  const vtkm::Bounds& GetVOI() const       { return this->VOI; }
  VTKM_CONT
  void SetVOI(vtkm::Bounds voi)            { this->VOI = voi; }

  // Sampling rate
  VTKM_CONT
  const vtkm::Id3& GetSampleRate() const   { return this->SampleRate; }
  VTKM_CONT
  void SetSampleRate(vtkm::Id3 sampleRate) { this->SampleRate = sampleRate; }

  template<typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT
  vtkm::filter::ResultDataSet DoExecute(const vtkm::cont::DataSet& input,
                                        const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                        const DeviceAdapter& tag);

  // Map new field onto the resulting dataset after running the filter
  template<typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
  VTKM_CONT
  bool DoMapField(vtkm::filter::ResultDataSet& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                  const DeviceAdapter& tag);

private:
  vtkm::Bounds VOI;
  vtkm::Id3 SampleRate;
  vtkm::worklet::ExtractStructured Worklet;
};

}
} // namespace vtkm::filter


#include <vtkm/filter/ExtractStructured.hxx>

#endif // vtk_m_filter_ExtractStructured_h
