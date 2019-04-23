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

  VTKM_CONT void SetRadius(int _radius) { this->radius = _radius; }

  VTKM_CONT void SetOrder(int _order) { this->order = _order; }

private:
  int radius;
  int order;
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
