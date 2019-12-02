//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_CleanGrid_h
#define vtk_m_filter_CleanGrid_h

#include <vtkm/filter/vtkm_filter_export.h>

#include <vtkm/filter/FilterDataSet.h>

#include <vtkm/worklet/PointMerge.h>
#include <vtkm/worklet/RemoveDegenerateCells.h>
#include <vtkm/worklet/RemoveUnusedPoints.h>

namespace vtkm
{
namespace filter
{

/// \brief Clean a mesh to an unstructured grid
///
/// This filter takes a data set and essentially copies it into a new data set.
/// The newly constructed data set will have the same cells as the input and
/// the topology will be stored in a \c CellSetExplicit<>. The filter will also
/// optionally remove all unused points.
///
/// Note that the result of \c CleanGrid is not necessarily smaller than the
/// input. For example, "cleaning" a data set with a \c CellSetStructured
/// topology will actually result in a much larger data set.
///
/// \todo Add a feature to merge points that are coincident or within a
/// tolerance.
///
class VTKM_ALWAYS_EXPORT CleanGrid : public vtkm::filter::FilterDataSet<CleanGrid>
{
public:
  VTKM_FILTER_EXPORT
  CleanGrid();

  /// When the CompactPointFields flag is true, the filter will identify any
  /// points that are not used by the topology. This is on by default.
  ///
  VTKM_CONT bool GetCompactPointFields() const { return this->CompactPointFields; }
  VTKM_CONT void SetCompactPointFields(bool flag) { this->CompactPointFields = flag; }

  /// When the MergePoints flag is true, the filter will identify any coincident
  /// points and merge them together. The distance two points can be to considered
  /// coincident is set with the tolerance flags. This is on by default.
  ///
  VTKM_CONT bool GetMergePoints() const { return this->MergePoints; }
  VTKM_CONT void SetMergePoints(bool flag) { this->MergePoints = flag; }

  /// Defines the tolerance used when determining whether two points are considered
  /// coincident. If the ToleranceIsAbsolute flag is false (the default), then this
  /// tolerance is scaled by the diagonal of the points.
  ///
  VTKM_CONT vtkm::Float64 GetTolerance() const { return this->Tolerance; }
  VTKM_CONT void SetTolerance(vtkm::Float64 tolerance) { this->Tolerance = tolerance; }

  /// When ToleranceIsAbsolute is false (the default) then the tolerance is scaled
  /// by the diagonal of the bounds of the dataset. If true, then the tolerance is
  /// taken as the actual distance to use.
  ///
  VTKM_CONT bool GetToleranceIsAbsolute() const { return this->ToleranceIsAbsolute; }
  VTKM_CONT void SetToleranceIsAbsolute(bool flag) { this->ToleranceIsAbsolute = flag; }

  /// Determine whether a cell is degenerate (that is, has repeated points that drops
  /// its dimensionalit) and removes them. This is on by default.
  ///
  VTKM_CONT bool GetRemoveDegenerateCells() const { return this->RemoveDegenerateCells; }
  VTKM_CONT void SetRemoveDegenerateCells(bool flag) { this->RemoveDegenerateCells = flag; }

  /// When FastMerge is true (the default), some corners are cut when computing
  /// coincident points. The point merge will go faster but the tolerance will not
  /// be strictly followed.
  ///
  VTKM_CONT bool GetFastMerge() const { return this->FastMerge; }
  VTKM_CONT void SetFastMerge(bool flag) { this->FastMerge = flag; }

  template <typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData,
                                          vtkm::filter::PolicyBase<Policy> policy);


  template <typename ValueType, typename Storage, typename Policy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<ValueType, Storage>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<Policy>)
  {
    if (fieldMeta.IsPointField() && (this->GetCompactPointFields() || this->GetMergePoints()))
    {
      vtkm::cont::ArrayHandle<ValueType> compactedArray;
      if (this->GetCompactPointFields())
      {
        compactedArray = this->PointCompactor.MapPointFieldDeep(input);
        if (this->GetMergePoints())
        {
          compactedArray = this->PointMerger.MapPointField(compactedArray);
        }
      }
      else if (this->GetMergePoints())
      {
        compactedArray = this->PointMerger.MapPointField(input);
      }
      result.AddField(fieldMeta.AsField(compactedArray));
    }
    else if (fieldMeta.IsCellField() && this->GetRemoveDegenerateCells())
    {
      result.AddField(fieldMeta.AsField(this->CellCompactor.ProcessCellField(input)));
    }
    else
    {
      result.AddField(fieldMeta.AsField(input));
    }

    return true;
  }

private:
  bool CompactPointFields;
  bool MergePoints;
  vtkm::Float64 Tolerance;
  bool ToleranceIsAbsolute;
  bool RemoveDegenerateCells;
  bool FastMerge;

  VTKM_FILTER_EXPORT vtkm::cont::DataSet GenerateOutput(
    const vtkm::cont::DataSet& inData,
    vtkm::cont::CellSetExplicit<>& outputCellSet);

  vtkm::worklet::RemoveUnusedPoints PointCompactor;
  vtkm::worklet::RemoveDegenerateCells CellCompactor;
  vtkm::worklet::PointMerge PointMerger;
};

#ifndef vtkm_filter_CleanGrid_cxx
VTKM_FILTER_EXPORT_EXECUTE_METHOD(CleanGrid);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/CleanGrid.hxx>

#endif //vtk_m_filter_CleanGrid_h
