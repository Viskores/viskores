//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ImageConnectivity_h
#define vtk_m_filter_ImageConnectivity_h

#include <vtkm/filter/FilterCell.h>
#include <vtkm/worklet/connectivities/ImageConnectivity.h>

/// \brief Groups connected points that have the same field value
///
///
/// The ImageConnectivity filter finds groups of points that have the same field value and are
/// connected together through their topology. Any point is considered to be connected to its Moore neighborhood:
/// 8 neighboring points for 2D and 27 neighboring points for 3D. As the name implies, ImageConnectivity only
/// works on data with a structured cell set. You will get an error if you use any other type of cell set.
/// The active field passed to the filter must be associated with the points.
/// The result of the filter is a point field of type vtkm::Id. Each entry in the point field will be a number that
/// identifies to which region it belongs. By default, this output point field is named “component”.
namespace vtkm
{
namespace filter
{
class ImageConnectivity : public vtkm::filter::FilterCell<ImageConnectivity>
{
public:
  using SupportedTypes = vtkm::TypeListScalarAll;

  VTKM_CONT ImageConnectivity();

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          const vtkm::filter::PolicyBase<DerivedPolicy>&);
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ImageConnectivity.hxx>

#endif //vtk_m_filter_ImageConnectivity_h
