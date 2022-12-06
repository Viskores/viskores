//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_field_transform_WarpScalar_h
#define vtk_m_filter_field_transform_WarpScalar_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/field_transform/vtkm_filter_field_transform_export.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
{
/// \brief Modify points by moving points along point normals by the scalar
/// amount times the scalar factor.
///
/// A filter that modifies point coordinates by moving points along point normals
/// by the scalar amount times the scalar factor.
/// It's a VTK-m version of the vtkWarpScalar in VTK.
/// Useful for creating carpet or x-y-z plots.
/// It doesn't modify the point coordinates, but creates a new point coordinates that have been warped.
class VTKM_FILTER_FIELD_TRANSFORM_EXPORT WarpScalar : public vtkm::filter::FilterField
{
public:
  VTKM_CONT
  explicit WarpScalar(vtkm::FloatDefault scaleAmount);

  ///@{
  /// Choose the secondary field to operate on. In the warp op A + B *
  /// scaleAmount * scalarFactor, B is the secondary field
  VTKM_CONT
  void SetNormalField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->NormalFieldName = name;
    this->NormalFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetNormalFieldName() const { return this->NormalFieldName; }

  VTKM_CONT vtkm::cont::Field::Association GetNormalFieldAssociation() const
  {
    return this->NormalFieldAssociation;
  }
  ///@}

  ///@{
  /// Choose the scalar factor field to operate on. In the warp op A + B *
  /// scaleAmount * scalarFactor, scalarFactor is the scalar factor field.
  VTKM_CONT
  void SetScalarFactorField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::Any)
  {
    this->ScalarFactorFieldName = name;
    this->ScalarFactorFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetScalarFactorFieldName() const
  {
    return this->ScalarFactorFieldName;
  }

  VTKM_CONT vtkm::cont::Field::Association GetScalarFactorFieldAssociation() const
  {
    return this->ScalarFactorFieldAssociation;
  }
  ///@}

private:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& input) override;

  std::string NormalFieldName = "normal";
  vtkm::cont::Field::Association NormalFieldAssociation = vtkm::cont::Field::Association::Any;
  std::string ScalarFactorFieldName = "scalarfactor";
  vtkm::cont::Field::Association ScalarFactorFieldAssociation = vtkm::cont::Field::Association::Any;
  vtkm::FloatDefault ScaleAmount;
};
} // namespace field_transform
} // namespace filter
} // namespace vtkm
#endif // vtk_m_filter_field_transform_WarpScalar_h
