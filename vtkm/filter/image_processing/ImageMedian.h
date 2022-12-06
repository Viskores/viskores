//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_image_processing_ImageMedian_h
#define vtk_m_filter_image_processing_ImageMedian_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/image_processing/vtkm_filter_image_processing_export.h>

/// \brief Median algorithm for general image blur
///
/// The ImageMedian filter finds the median value for each pixel in an image.
/// Currently the algorithm has the following restrictions.
///   - Only supports a neighborhood of 5x5x1 or 3x3x1
///
/// This means that volumes are basically treated as an image stack
/// along the z axis
///
/// Default output field name is 'median'
namespace vtkm
{
namespace filter
{
namespace image_processing
{
class VTKM_FILTER_IMAGE_PROCESSING_EXPORT ImageMedian : public vtkm::filter::FilterField
{
public:
  VTKM_CONT ImageMedian() { this->SetOutputFieldName("median"); }

  VTKM_CONT void Perform3x3() { this->Neighborhood = 1; };
  VTKM_CONT void Perform5x5() { this->Neighborhood = 2; };

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  int Neighborhood = 1;
};
} // namespace image_processing
} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_image_processing_ImageMedian_h
