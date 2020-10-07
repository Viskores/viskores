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

#include <vtkm/filter/FilterField.h>


namespace vtkm
{
namespace filter
{
class ComputeMoments : public vtkm::filter::FilterField<ComputeMoments>
{
public:
  using SupportedTypes = vtkm::List<vtkm::Float32,
                                    vtkm::Float64,
                                    vtkm::Vec<vtkm::Float32, 2>,
                                    vtkm::Vec<vtkm::Float64, 2>,
                                    vtkm::Vec<vtkm::Float32, 3>,
                                    vtkm::Vec<vtkm::Float64, 3>,
                                    vtkm::Vec<vtkm::Float32, 4>,
                                    vtkm::Vec<vtkm::Float64, 4>,
                                    vtkm::Vec<vtkm::Float32, 6>,
                                    vtkm::Vec<vtkm::Float64, 6>,
                                    vtkm::Vec<vtkm::Float32, 9>,
                                    vtkm::Vec<vtkm::Float64, 9>>;

  VTKM_CONT ComputeMoments();

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>&);

  VTKM_CONT void SetRadius(double _radius) { this->Radius = _radius; }

  VTKM_CONT void SetSpacing(vtkm::Vec3f _spacing) { this->Spacing = _spacing; }

  VTKM_CONT void SetOrder(vtkm::Int32 _order) { this->Order = _order; }

private:
  double Radius = 1;
  vtkm::Vec3f Spacing = { 1.0f, 1.0f, 1.0f };
  vtkm::Int32 Order = 0;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ComputeMoments.hxx>

#endif //vtk_m_filter_ComputeMoments_h
