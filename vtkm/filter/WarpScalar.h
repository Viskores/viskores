//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_WarpScalar_h
#define vtk_m_filter_WarpScalar_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/WarpScalar.h>

namespace vtkm
{
namespace filter
{
/// \brief Modify points by moving points along point normals by the scalar
/// amount times the scalar factor.
///
/// A filter that modifies point coordinates by moving points along point normals
/// by the scalar amount times the scalar factor.
/// It's a VTK-m version of the vtkWarpScalar in VTK.
/// Useful for creating carpet or x-y-z plots.
/// It doesn't modify the point coordinates, but creates a new point coordinates that have been warped.
class WarpScalar : public vtkm::filter::FilterField<WarpScalar>
{
public:
  VTKM_CONT
  WarpScalar(vtkm::FloatDefault scaleAmount);

  //@{
  /// Choose the primary field to operate on. In the warp op A + B *
  /// scaleAmount * scalarFactor, A is the primary field
  VTKM_CONT
  void SetPrimaryField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->SetActiveField(name, association);
  }
  //@}

  VTKM_CONT const std::string& GetPrimaryFieldName() const { return this->GetActiveFieldName(); }

  VTKM_CONT vtkm::cont::Field::Association GetPrimaryFieldAssociation() const
  {
    return this->GetActiveFieldAssociation();
  }

  //@{
  /// When set to true, filter uses a coordinate system as the primary field instead of the one selected
  /// by name. Use SetPrimaryCoordinateSystem to select which coordinate system.
  VTKM_CONT
  void SetUseCoordinateSystemAsPrimaryField(bool flag)
  {
    this->SetUseCoordinateSystemAsField(flag);
  }
  VTKM_CONT
  bool GetUseCoordinateSystemAsPrimaryField() const
  {
    return this->GetUseCoordinateSystemAsField();
  }
  //@}

  //@{
  /// Select the coordinate system index to use as the primary field. This only has an effect when
  /// UseCoordinateSystemAsPrimaryField is true.
  VTKM_CONT
  void SetPrimaryCoordinateSystem(vtkm::Id index) { this->SetActiveCoordinateSystem(index); }
  VTKM_CONT
  vtkm::Id GetPrimaryCoordinateSystemIndex() const
  {
    return this->GetActiveCoordinateSystemIndex();
  }
  //@}

  //@{
  /// Choose the secondary field to operate on. In the warp op A + B *
  /// scaleAmount * scalarFactor, B is the secondary field
  VTKM_CONT
  void SetNormalField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->NormalFieldName = name;
    this->NormalFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetNormalFieldName() const { return this->NormalFieldName; }

  VTKM_CONT vtkm::cont::Field::Association GetNormalFieldAssociation() const
  {
    return this->NormalFieldAssociation;
  }
  //@}

  //@{
  /// Choose the scalar factor field to operate on. In the warp op A + B *
  /// scaleAmount * scalarFactor, scalarFactor is the scalar factor field.
  VTKM_CONT
  void SetScalarFactorField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
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
  //@}

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::WarpScalar Worklet;
  std::string NormalFieldName;
  vtkm::cont::Field::Association NormalFieldAssociation;
  std::string ScalarFactorFieldName;
  vtkm::cont::Field::Association ScalarFactorFieldAssociation;
  vtkm::FloatDefault ScaleAmount;
};

template <>
class FilterTraits<WarpScalar>
{
public:
  // WarpScalar can only applies to Float and Double Vec3 arrays
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};

struct WarpScalarScalarFieldTag
{
};

template <>
struct FilterTraits<WarpScalar, WarpScalarScalarFieldTag>
{
  using InputFieldTypeList = vtkm::TypeListTagFieldScalar;
};
}
}

#include <vtkm/filter/WarpScalar.hxx>

#endif // vtk_m_filter_WarpScalar_h
