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

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

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
