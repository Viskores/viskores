//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_Probe_h
#define vtk_m_filter_Probe_h

#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/Probe.h>

namespace vtkm
{
namespace filter
{

class Probe : public vtkm::filter::FilterDataSet<Probe>
{
public:
  VTKM_CONT
  void SetGeometry(const vtkm::cont::DataSet& geometry);

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  //this call is only valid
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::cont::DataSet Geometry;
  vtkm::worklet::Probe Worklet;
};
}
} // vtkm::filter

#include <vtkm/filter/Probe.hxx>

#endif // vtk_m_filter_Probe_h
