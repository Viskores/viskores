//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_clean_grid_CleanGrid_h
#define vtk_m_filter_clean_grid_CleanGrid_h

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/clean_grid/vtkm_filter_clean_grid_export.h>

namespace vtkm
{
namespace filter
{

// Forward declaration for stateful Worklets
namespace clean_grid
{
struct SharedStates;

/// \brief Clean a mesh to an unstructured grid.
///
/// This filter converts the cells of its input to an explicit representation
/// and potentially removes redundant or unused data.
/// The newly constructed data set will have the same cells as the input and
/// the topology will be stored in a `vtkm::cont::CellSetExplicit<>`. The filter will also
/// optionally remove all unused points.
///
/// Note that the result of `CleanGrid` is not necessarily smaller than the
/// input. For example, "cleaning" a data set with a `vtkm::cont::CellSetStructured`
/// topology will actually result in a much larger data set.
///
/// `CleanGrid` can optionally merge close points. The closeness of points is determined
/// by the coordinate system. If there are multiple coordinate systems, the desired
/// coordinate system can be selected with the `SetActiveCoordinateSystem()` method.
///
class VTKM_FILTER_CLEAN_GRID_EXPORT CleanGrid : public vtkm::filter::Filter
{
public:
  /// When the CompactPointFields flag is true, the filter will identify and remove any
  /// points that are not used by the topology. This is on by default.
  ///
  VTKM_CONT bool GetCompactPointFields() const { return this->CompactPointFields; }
  /// @copydoc GetCompactPointFields
  VTKM_CONT void SetCompactPointFields(bool flag) { this->CompactPointFields = flag; }

  /// When the MergePoints flag is true, the filter will identify any coincident
  /// points and merge them together. The distance two points can be to considered
  /// coincident is set with the tolerance flags. This is on by default.
  ///
  VTKM_CONT bool GetMergePoints() const { return this->MergePoints; }
  /// @copydoc GetMergePoints
  VTKM_CONT void SetMergePoints(bool flag) { this->MergePoints = flag; }

  /// Defines the tolerance used when determining whether two points are considered
  /// coincident. Because floating point parameters have limited precision, point
  /// coordinates that are essentially the same might not be bit-wise exactly the same.
  /// Thus, the `CleanGrid` filter has the ability to find and merge points that are
  /// close but perhaps not exact. If the ToleranceIsAbsolute flag is false (the default),
  /// then this tolerance is scaled by the diagonal of the points.
  ///
  VTKM_CONT vtkm::Float64 GetTolerance() const { return this->Tolerance; }
  /// @copydoc GetTolerance
  VTKM_CONT void SetTolerance(vtkm::Float64 tolerance) { this->Tolerance = tolerance; }

  /// When ToleranceIsAbsolute is false (the default) then the tolerance is scaled
  /// by the diagonal of the bounds of the dataset. If true, then the tolerance is
  /// taken as the actual distance to use.
  ///
  VTKM_CONT bool GetToleranceIsAbsolute() const { return this->ToleranceIsAbsolute; }
  /// @copydoc GetToleranceIsAbsolute
  VTKM_CONT void SetToleranceIsAbsolute(bool flag) { this->ToleranceIsAbsolute = flag; }

  /// When RemoveDegenerateCells is true (the default), then `CleanGrid` will look
  /// for repeated points in cells and, if the repeated points cause the cell to drop
  /// dimensionality, the cell is removed. This is particularly useful when point merging
  /// is on as this operation can create degenerate cells.
  ///
  VTKM_CONT bool GetRemoveDegenerateCells() const { return this->RemoveDegenerateCells; }
  /// @copydoc GetRemoveDegenerateCells
  VTKM_CONT void SetRemoveDegenerateCells(bool flag) { this->RemoveDegenerateCells = flag; }

  /// When FastMerge is true (the default), some corners are cut when computing
  /// coincident points. The point merge will go faster but the tolerance will not
  /// be strictly followed.
  ///
  VTKM_CONT bool GetFastMerge() const { return this->FastMerge; }
  /// @copydoc GetFastMerge
  VTKM_CONT void SetFastMerge(bool flag) { this->FastMerge = flag; }

private:
  VTKM_CONT
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;

  VTKM_CONT vtkm::cont::DataSet GenerateOutput(const vtkm::cont::DataSet& inData,
                                               vtkm::cont::CellSetExplicit<>& outputCellSet,
                                               clean_grid::SharedStates& worklets);

  bool CompactPointFields = true;
  bool MergePoints = true;
  vtkm::Float64 Tolerance = 1.0e-6;
  bool ToleranceIsAbsolute = false;
  bool RemoveDegenerateCells = true;
  bool FastMerge = true;
};
} // namespace clean_grid

} // namespace filter
} // namespace vtkm

#endif //vtk_m_filter_clean_grid_CleanGrid_h
