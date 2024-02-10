//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=======================================================================
#ifndef vtk_m_filter_image_processing_ComputeMoments_h
#define vtk_m_filter_image_processing_ComputeMoments_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/image_processing/vtkm_filter_image_processing_export.h>

namespace vtkm
{
namespace filter
{
namespace image_processing
{
class VTKM_FILTER_IMAGE_PROCESSING_EXPORT ComputeMoments : public vtkm::filter::Filter
{
public:
  VTKM_CONT ComputeMoments();

  VTKM_CONT void SetRadius(double _radius) { this->Radius = _radius; }

  VTKM_CONT void SetSpacing(vtkm::Vec3f _spacing) { this->Spacing = _spacing; }

  VTKM_CONT void SetOrder(vtkm::Int32 _order) { this->Order = _order; }

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  double Radius = 1;
  vtkm::Vec3f Spacing = { 1.0f, 1.0f, 1.0f };
  vtkm::Int32 Order = 0;
};
} // namespace image_processing
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_image_processing_ComputeMoments_h
