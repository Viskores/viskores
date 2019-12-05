//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_VectorMagnitude_h
#define vtk_m_filter_VectorMagnitude_h

#include <vtkm/filter/vtkm_filter_export.h>

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/Magnitude.h>

namespace vtkm
{
namespace filter
{

class VTKM_ALWAYS_EXPORT VectorMagnitude : public vtkm::filter::FilterField<VectorMagnitude>
{
public:
  //currently the VectorMagnitude filter only works on vector data.
  using SupportedTypes = vtkm::TypeListVecCommon;

  VTKM_FILTER_EXPORT
  VectorMagnitude();

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::Magnitude Worklet;
};
#ifndef vtkm_filter_VectorMagnitude_cxx
VTKM_FILTER_EXPORT_EXECUTE_METHOD(VectorMagnitude);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/VectorMagnitude.hxx>

#endif // vtk_m_filter_VectorMagnitude_h
