//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_geometry_refinement_ConvertToPointCloud_h
#define vtk_m_filter_geometry_refinement_ConvertToPointCloud_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/geometry_refinement/vtkm_filter_geometry_refinement_export.h>

namespace vtkm
{
namespace filter
{
namespace geometry_refinement
{

/// @brief Convert a `DataSet` to a point cloud.
///
/// A point cloud in VTK-m is represented as a data set with "vertex" shape cells.
/// This filter replaces the `CellSet` in a `DataSet` with a `CellSet` of only
/// vertex cells. There will be one cell per point.
///
/// This filter is useful for dropping the cells of any `DataSet` so that you can
/// operate on it as just a collection of points. It is also handy for completing
/// a `DataSet` that does not have a `CellSet` associated with it or has points
/// that do not belong to cells.
///
/// Note that all fields associated with cells are dropped. This is because the
/// cells are dropped.
///
class VTKM_FILTER_GEOMETRY_REFINEMENT_EXPORT ConvertToPointCloud : public vtkm::filter::Filter
{
  bool AssociateFieldsWithCells = false;

public:
  /// By default, all the input point fields are kept as point fields in the output.
  /// However, the output has exactly one cell per point and it might be easier to
  /// treat the fields as cell fields. When this flag is turned on, the point field
  /// association is changed to cell.
  ///
  /// Note that any field that is marked as point coordinates will remain as point
  /// fields. It is not valid to set a cell field as the point coordinates.
  ///
  VTKM_CONT void SetAssociateFieldsWithCells(bool flag) { this->AssociateFieldsWithCells = flag; }
  /// @copydoc SetAssociateFieldsWithCells
  VTKM_CONT bool GetAssociateFieldsWithCells() const { return this->AssociateFieldsWithCells; }

protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;
};

}
}
} // namespace vtkm::filter::geometry_refinement

#endif //vtk_m_filter_geometry_refinement_ConvertToPointCloud_h
