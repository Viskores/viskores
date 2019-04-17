//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ExtractPoints_h
#define vtk_m_filter_ExtractPoints_h

#include <vtkm/cont/ImplicitFunctionHandle.h>
#include <vtkm/filter/CleanGrid.h>
#include <vtkm/filter/FilterDataSet.h>
#include <vtkm/worklet/ExtractPoints.h>

namespace vtkm
{
namespace filter
{
/// @brief Extract only points from a geometry using an implicit function
///
///
/// Extract only the  points that are either inside or outside of a
/// VTK-m implicit function. Examples include planes, spheres, boxes,
/// etc.
/// Note that while any geometry type can be provided as input, the output is
/// represented by an explicit representation of points using
/// vtkm::cont::CellSetSingleType
class ExtractPoints : public vtkm::filter::FilterDataSet<ExtractPoints>
{
public:
  VTKM_CONT
  ExtractPoints();

  /// When CompactPoints is set, instead of copying the points and point fields
  /// from the input, the filter will create new compact fields without the unused elements
  VTKM_CONT
  bool GetCompactPoints() const { return this->CompactPoints; }
  VTKM_CONT
  void SetCompactPoints(bool value) { this->CompactPoints = value; }

  /// Set the volume of interest to extract
  void SetImplicitFunction(const vtkm::cont::ImplicitFunctionHandle& func)
  {
    this->Function = func;
  }

  const vtkm::cont::ImplicitFunctionHandle& GetImplicitFunction() const { return this->Function; }

  VTKM_CONT
  bool GetExtractInside() { return this->ExtractInside; }
  VTKM_CONT
  void SetExtractInside(bool value) { this->ExtractInside = value; }
  VTKM_CONT
  void ExtractInsideOn() { this->ExtractInside = true; }
  VTKM_CONT
  void ExtractInsideOff() { this->ExtractInside = false; }

  template <typename DerivedPolicy>
  vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  bool DoMapField(vtkm::cont::DataSet& result,
                  const vtkm::cont::ArrayHandle<T, StorageType>& input,
                  const vtkm::filter::FieldMetadata& fieldMeta,
                  vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  bool ExtractInside;
  vtkm::cont::ImplicitFunctionHandle Function;

  bool CompactPoints;
  vtkm::filter::CleanGrid Compactor;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/ExtractPoints.hxx>

#endif // vtk_m_filter_ExtractPoints_h
