//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_WarpVector_h
#define vtk_m_filter_WarpVector_h

#include <vtkm/filter/FilterField.h>
#include <vtkm/worklet/WarpVector.h>

namespace vtkm
{
namespace filter
{
/// \brief Modify points by moving points along a vector then timing
/// the scale factor
///
/// A filter that modifies point coordinates by moving points along a vector
/// then timing a scale factor. It's a VTK-m version of the vtkWarpVector in VTK.
/// Useful for showing flow profiles or mechanical deformation.
/// This worklet does not modify the input points but generate new point
/// coordinate instance that has been warped.
class WarpVector : public vtkm::filter::FilterField<WarpVector>
{
public:
  VTKM_CONT
  WarpVector(vtkm::FloatDefault scale);

  //@{
  /// Choose the primary field to operate on. In the warp op A + B *scale, A is
  /// the primary field
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
  /// Choose the vector field to operate on. In the warp op A + B *scale, B is
  /// the vector field
  VTKM_CONT
  void SetVectorField(
    const std::string& name,
    vtkm::cont::Field::Association association = vtkm::cont::Field::Association::ANY)
  {
    this->VectorFieldName = name;
    this->VectorFieldAssociation = association;
  }

  VTKM_CONT const std::string& GetVectorFieldName() const { return this->VectorFieldName; }

  VTKM_CONT vtkm::cont::Field::Association GetVectorFieldAssociation() const
  {
    return this->VectorFieldAssociation;
  }
  //@}

  template <typename T, typename StorageType, typename DerivedPolicy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(
    const vtkm::cont::DataSet& input,
    const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, StorageType>& field,
    const vtkm::filter::FieldMetadata& fieldMeta,
    vtkm::filter::PolicyBase<DerivedPolicy> policy);

private:
  vtkm::worklet::WarpVector Worklet;
  std::string VectorFieldName;
  vtkm::cont::Field::Association VectorFieldAssociation;
  vtkm::FloatDefault Scale;
};

template <>
class FilterTraits<WarpVector>
{
public:
  // WarpVector can only applies to Float and Double Vec3 arrays
  using InputFieldTypeList = vtkm::TypeListTagFieldVec3;
};
}
}

#include <vtkm/filter/WarpVector.hxx>

#endif // vtk_m_filter_WarpVector_h
