//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ImageMedian_h
#define vtk_m_filter_ImageMedian_h

#include <vtkm/filter/FilterCell.h>

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
class ImageMedian : public vtkm::filter::FilterField<ImageMedian>
{
  int Neighborhood = 1;

public:
  using SupportedTypes = vtkm::TypeListScalarAll;

  VTKM_CONT ImageMedian();

  VTKM_CONT void Perform3x3() { this->Neighborhood = 1; };
  VTKM_CONT void Perform5x5() { this->Neighborhood = 2; };


  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>&);
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ImageMedian.hxx>

#endif //vtk_m_filter_ImageMedian_h
