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
/// \brief Modify points by moving points along a vector multiplied by
/// the scale factor
///
/// A filter that modifies point coordinates by moving points along a vector
/// multiplied by a scale factor. It's a VTK-m version of the vtkWarpVector in VTK.
/// Useful for showing flow profiles or mechanical deformation.
/// This worklet does not modify the input points but generate new point
/// coordinate instance that has been warped.
class WarpVector : public vtkm::filter::FilterField<WarpVector>
{
public:
  using SupportedTypes = vtkm::TypeListFieldVec3;
  using AdditionalFieldStorage =
    vtkm::List<vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_32>::StorageTag,
               vtkm::cont::ArrayHandleConstant<vtkm::Vec3f_64>::StorageTag>;

  VTKM_CONT
  WarpVector(vtkm::FloatDefault scale);

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
}
}

#include <vtkm/filter/WarpVector.hxx>

#endif // vtk_m_filter_WarpVector_h
