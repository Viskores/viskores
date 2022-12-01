//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_WarpVector_h
#define vtk_m_filter_field_transform_WarpVector_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief Modify points by moving points along a vector multiplied by
/// the scale factor
///
/// A filter that modifies point coordinates by moving points along a vector
/// multiplied by a scale factor. It's a VTK-m version of the vtkWarpVector in VTK.
/// Useful for showing flow profiles or mechanical deformation.
/// This worklet does not modify the input points but generate new point
/// coordinate instance that has been warped.
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT WarpVector : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  explicit WarpVector(vtkm::FloatDefault scale);

  ///@{
  /// Choose the vector field to operate on. In the warp op A + B *scale, B is
  /// the vector field
  VTKM_CONT
  void SetVectorField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->VectorFieldName = name;
    this->VectorFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetVectorFieldName() const { return this->VectorFieldName; }

  VTKM_CONT vtkm::cont::Field::Association GetVectorFieldAssociation() const
  {
    return this->VectorFieldAssociation;
  }
  ///@}

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  std::string VectorFieldName = "normal";
  vtkm::cont::Field::Association VectorFieldAssociation = vtkm::cont::Field::Association::Any;
  vtkm::FloatDefault Scale;
};
} // namespace field_transform
} // namespace filter
} // namespace vtkm
#endif // vtk_m_filter_field_transform_WarpVector_h
