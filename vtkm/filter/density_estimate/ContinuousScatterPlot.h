//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_density_estimate_ContinuousScatterPlot_h
#define vtk_m_filter_density_estimate_ContinuousScatterPlot_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/density_estimate/vtkm_filter_density_estimate_export.h>

namespace vtkm
{
namespace filter
{
namespace density_estimate
{
/// \brief Constructs the continuous scatterplot for two given scalar point fields of a mesh.
///
/// The continuous scatterplot is an extension of the discrete scatterplot for continuous bi-variate analysis.
/// This filter outputs an ExplicitDataSet of triangle-shaped cells, whose coordinates on the 2D plane represent respectively
/// the values of both scalar fields. Triangles' points are associated with a scalar field, representing the
/// density of values in the data domain. The filter tetrahedralizes the input dataset before operating.
///
/// If both fields provided don't have the same floating point precision, the output will
/// have the precision of the first one of the pair.
///
/// This implementation is based on the algorithm presented in the publication :
///
/// S. Bachthaler and D. Weiskopf, "Continuous Scatterplots"
/// in IEEE Transactions on Visualization and Computer Graphics,
/// vol. 14, no. 6, pp. 1428-1435, Nov.-Dec. 2008
/// doi: 10.1109/TVCG.2008.119.

class VTKM_FILTER_DENSITY_ESTIMATE_EXPORT ContinuousScatterPlot : public vtkm::filter::FilterField
{
public:
  VTKM_CONT ContinuousScatterPlot() { this->SetOutputFieldName("density"); }

  /// Select both point fields to use when running the filter.
  /// Replaces setting each one individually using `SetActiveField` on indices 0 and 1.
  VTKM_CONT
  void SetActiveFieldsPair(const std::string& fieldName1, const std::string& fieldName2)
  {
    SetActiveField(0, fieldName1, vtkm::cont::Field::Association::Points);
    SetActiveField(1, fieldName2, vtkm::cont::Field::Association::Points);
  };

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};
} // namespace density_estimate
} // namespace filter
} // namespace vtm

#endif //vtk_m_filter_density_estimate_ContinuousScatterPlot_h
