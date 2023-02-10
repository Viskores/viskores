//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_connected_components_ImageConnectivity_h
#define vtk_m_filter_connected_components_ImageConnectivity_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/connected_components/vtkm_filter_connected_components_export.h>

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
namespace connected_components
{
class VTKM_FILTER_CONNECTED_COMPONENTS_EXPORT ImageConnectivity : public vtkm::filter::FilterField
{
public:
  VTKM_CONT ImageConnectivity() { this->SetOutputFieldName("component"); }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace connected_components

} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_connected_components_ImageConnectivity_h
