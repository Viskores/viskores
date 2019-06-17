//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=======================================================================
#ifndef vtk_m_filter_ComputeMoments_h
#define vtk_m_filter_ComputeMoments_h

#include <vtkm/filter/FilterCell.h>


namespace vtkm
{
namespace filter
{
class ComputeMoments : public vtkm::filter::FilterCell<ComputeMoments>
{
public:
  VTKM_CONT ComputeMoments();

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>&);

  // TODO: change radius to vec3<int>
  VTKM_CONT void SetRadius(vtkm::Vec<vtkm::Int32, 3> _radius) { this->Radius = _radius; }

  VTKM_CONT void SetOrder(vtkm::Int32 _order) { this->Order = _order; }

private:
  vtkm::Vec<vtkm::Int32, 3> Radius = { 1, 1, 1 };
  vtkm::Int32 Order = 0;
};

template <>
class FilterTraits<vtkm::filter::ComputeMoments>
{
public:
  struct InputFieldTypeList : vtkm::TypeListTagAll
  {
  };
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ComputeMoments.hxx>

#endif //vtk_m_filter_ComputeMoments_h
