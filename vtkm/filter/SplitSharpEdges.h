//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_SplitSharpEdges_h
#define vtk_m_filter_SplitSharpEdges_h

#include <vtkm/filter/FilterDataSetWithField.h>
#include <vtkm/worklet/SplitSharpEdges.h>

namespace vtkm
{
namespace filter
{

/// \brief Split sharp manifold edges where the feature angle between the
///  adjacent surfaces are larger than the threshold value
///
/// Split sharp manifold edges where the feature angle between the adjacent
/// surfaces are larger than the threshold value. When an edge is split, it
/// would add a new point to the coordinates and update the connectivity of
/// an adjacent surface.
/// Ex. there are two adjacent triangles(0,1,2) and (2,1,3). Edge (1,2) needs
/// to be split. Two new points 4(duplication of point 1) an 5(duplication of point 2)
/// would be added and the later triangle's connectivity would be changed
/// to (5,4,3).
/// By default, all old point's fields would be copied to the new point.
/// Use with caution.
class SplitSharpEdges : public vtkm::filter::FilterDataSetWithField<SplitSharpEdges>
{
public:
  // SplitSharpEdges filter needs cell normals to decide split.
  using SupportedTypes = vtkm::TypeListFieldVec3;

  VTKM_CONT
  SplitSharpEdges();

  VTKM_CONT
  void SetFeatureAngle(vtkm::FloatDefault value) { this->FeatureAngle = value; }

  VTKM_CONT
  vtkm::FloatDefault GetFeatureAngle() const { return this->FeatureAngle; }

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input,
                                          const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                          const vtkm::filter::FieldMetadata& fieldMeta,
                                          vtkm::filter::PolicyBase<DerivedPolicy> policy);

  //Map a new field onto the resulting dataset after running the filter
  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT bool DoMapField(vtkm::cont::DataSet& result,
                            const vtkm::cont::ArrayHandle<T, StorageType>& input,
                            const vtkm::filter::FieldMetadata& fieldMeta,
                            vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::FloatDefault FeatureAngle;
  vtkm::worklet::SplitSharpEdges Worklet;
};
}
} // namespace vtkm::filter

#include <vtkm/filter/SplitSharpEdges.hxx>

#endif // vtk_m_filter_SplitSharpEdges_h
