//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_filter_entity_extraction_ExternalFaces_h
#define vtkm_filter_entity_extraction_ExternalFaces_h

#include <vtkm/filter/NewFilterField.h>
#include <vtkm/filter/entity_extraction/vtkm_filter_entity_extraction_export.h>

namespace vtkm
{
namespace worklet
{
struct ExternalFaces;
}
namespace filter
{
namespace entity_extraction
{
/// \brief  Extract external faces of a geometry
///
/// ExternalFaces is a filter that extracts all external faces from a
/// data set. An external face is defined is defined as a face/side of a cell
/// that belongs only to one cell in the entire mesh.
/// @warning
/// This filter is currently only supports propagation of point properties
///
class VTKM_FILTER_ENTITY_EXTRACTION_EXPORT ExternalFaces : public vtkm::filter::NewFilterField
{
public:
  ExternalFaces();
  ~ExternalFaces() override;

  // New Design: I am too lazy to make this filter thread-safe. Let's use it as an example of
  // thread un-safe filter.
  bool CanThread() const override { return false; }

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
  void SetPassPolyData(bool value);

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  vtkm::cont::DataSet GenerateOutput(const vtkm::cont::DataSet& input,
                                     vtkm::cont::CellSetExplicit<>& outCellSet);

  VTKM_CONT bool MapFieldOntoOutput(vtkm::cont::DataSet& result, const vtkm::cont::Field& field);

  bool CompactPoints = false;
  bool PassPolyData = true;

  // Note: This shared state as a data member requires us to explicitly implement the
  // constructor and destructor in the .cxx file, after the compiler actually have
  // seen the definition of worklet:ExternalFaces, even if the implementation of
  // the cstr/dstr is just = default. Otherwise the compiler does not know how to
  // allocate/free storage for the std::unique_ptr.
  std::unique_ptr<vtkm::worklet::ExternalFaces> Worklet;
};
} // namespace entity_extraction
class VTKM_DEPRECATED(1.8, "Use vtkm::filter::entity_extraction::ExternalFaces.") ExternalFaces
  : public vtkm::filter::entity_extraction::ExternalFaces
{
  using entity_extraction::ExternalFaces::ExternalFaces;
};
} // namespace filter
} // namespace vtkm

#endif // vtkm_filter_entity_extraction_ExternalFaces_h
