//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExternalFaces_h
#define vtk_m_filter_ExternalFaces_h

#include <vtkm/filter/vtkm_filter_extra_export.h>

#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/worklet/ExternalFaces.h>

namespace vtkm
{
namespace filter
{

/// \brief  Extract external faces of a geometry
///
/// ExternalFaces is a filter that extracts all external faces from a
/// data set. An external face is defined is defined as a face/side of a cell
/// that belongs only to one cell in the entire mesh.
/// @warning
/// This filter is currently only supports propagation of point properties
///
class VTKM_ALWAYS_EXPORT ExternalFaces : public vtkm::filter::FilterDataSet<ExternalFaces>
{
public:
  VTKM_FILTER_EXTRA_EXPORT
  ExternalFaces();

  // When CompactPoints is set, instead of copying the points and point fields
  // from the input, the filter will create new compact fields without the
  // unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  // When PassPolyData is set (the default), incoming poly data (0D, 1D, and 2D cells)
  // will be passed to the output external faces data set.
  VTKM_CONT
  bool GetPassPolyData() const { return this->PassPolyData; }
  VTKM_CONT
  void SetPassPolyData(bool value)
  {
    this->PassPolyData = value;
    this->Worklet.SetPassPolyData(value);
  }

  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  VTKM_FILTER_EXTRA_EXPORT VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                                             const vtkm::cont::Field& field);

  template <typename DerivedPolicy>
  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result,
                                    const vtkm::cont::Field& field,
                                    vtkm::filter::PolicyBase<DerivedPolicy>)
  {
    return this->MapFieldOntoOutput(result, field);
  }

private:
  bool CompactPoints;
  bool PassPolyData;

  VTKM_FILTER_EXTRA_EXPORT vtkm::cont::DataSet GenerateOutput(
    const vtkm::cont::DataSet& input,
    vtkm::cont::CellSetExplicit<>& outCellSet);

  vtkm::filter::CleanGrid Compactor;
  vtkm::worklet::ExternalFaces Worklet;
};
#ifndef vtkm_filter_ExternalFaces_cxx
VTKM_FILTER_EXPORT_EXECUTE_METHOD(ExternalFaces);
#endif
}
} // namespace vtkm::filter

#include <vtkm/filter/ExternalFaces.hxx>

#endif // vtk_m_filter_ExternalFaces_h
