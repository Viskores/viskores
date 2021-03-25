//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_woklet_particleadvection_field_h
#define vtkm_woklet_particleadvection_field_h

#include <vtkm/Types.h>

#include <vtkm/VecVariable.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/exec/CellInterpolate.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

template <typename FieldArrayType>
class ExecutionVelocityField
{
public:
  using FieldPortalType = typename FieldArrayType::ReadPortalType;

  VTKM_CONT
  ExecutionVelocityField(FieldArrayType velocityValues,
                         vtkm::cont::DeviceAdapterId device,
                         vtkm::cont::Token& token)
    : VelocityValues(velocityValues.PrepareForInput(device, token))
  {
  }

  VTKM_EXEC void GetValue(const vtkm::VecVariable<vtkm::Id, 8>& indices,
                          const vtkm::Id vertices,
                          const vtkm::Vec3f& parametric,
                          const vtkm::UInt8 cellShape,
                          vtkm::VecVariable<vtkm::Vec3f, 2>& value) const
  {
    vtkm::Vec3f velocityInterp;
    vtkm::VecVariable<vtkm::Vec3f, 8> velocities;
    for (vtkm::IdComponent i = 0; i < vertices; i++)
      velocities.Append(VelocityValues.Get(indices[i]));
    vtkm::exec::CellInterpolate(velocities, parametric, cellShape, velocityInterp);
    value = vtkm::make_Vec(velocityInterp);
  }

private:
  FieldPortalType VelocityValues;
};

template <typename FieldArrayType>
class ExecutionElectroMagneticField
{
public:
  using FieldPortalType = typename FieldArrayType::ReadPortalType;

  VTKM_CONT
  ExecutionElectroMagneticField(FieldArrayType electricValues,
                                FieldArrayType magneticValues,
                                vtkm::cont::DeviceAdapterId device,
                                vtkm::cont::Token& token)
    : ElectricValues(electricValues.PrepareForInput(device, token))
    , MagneticValues(magneticValues.PrepareForInput(device, token))
  {
  }

  VTKM_EXEC void GetValue(const vtkm::VecVariable<vtkm::Id, 8>& indices,
                          const vtkm::Id vertices,
                          const vtkm::Vec3f& parametric,
                          const vtkm::UInt8 cellShape,
                          vtkm::VecVariable<vtkm::Vec3f, 2>& value) const
  {
    vtkm::Vec3f electricInterp, magneticInterp;
    vtkm::VecVariable<vtkm::Vec3f, 8> electric;
    vtkm::VecVariable<vtkm::Vec3f, 8> magnetic;
    for (vtkm::IdComponent i = 0; i < vertices; i++)
    {
      electric.Append(ElectricValues.Get(indices[i]));
      magnetic.Append(MagneticValues.Get(indices[i]));
    }
    vtkm::exec::CellInterpolate(electric, parametric, cellShape, electricInterp);
    vtkm::exec::CellInterpolate(magnetic, parametric, cellShape, magneticInterp);
    value = vtkm::make_Vec(electricInterp, magneticInterp);
  }

private:
  FieldPortalType ElectricValues;
  FieldPortalType MagneticValues;
};

template <typename FieldArrayType>
class VelocityField : public vtkm::cont::ExecutionObjectBase
{
public:
  using ExecutionType = ExecutionVelocityField<FieldArrayType>;

  VTKM_CONT
  VelocityField() = default;

  VTKM_CONT
  VelocityField(const FieldArrayType& fieldValues)
    : FieldValues(fieldValues)
  {
  }

  VTKM_CONT
  const ExecutionType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                          vtkm::cont::Token& token) const
  {
    return ExecutionType(this->FieldValues, device, token);
  }

private:
  FieldArrayType FieldValues;
};

template <typename FieldArrayType>
class ElectroMagneticField : public vtkm::cont::ExecutionObjectBase
{
public:
  using ExecutionType = ExecutionElectroMagneticField<FieldArrayType>;

  VTKM_CONT
  ElectroMagneticField() = default;

  VTKM_CONT
  ElectroMagneticField(const FieldArrayType& electricField, const FieldArrayType& magneticField)
    : ElectricField(electricField)
    , MagneticField(magneticField)
  {
  }

  VTKM_CONT
  const ExecutionType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                          vtkm::cont::Token& token) const
  {
    return ExecutionType(this->ElectricField, this->MagneticField, device, token);
  }

private:
  FieldArrayType ElectricField;
  FieldArrayType MagneticField;
};

} // namespace particleadvection
} // namespace worklet
} // namespace
#endif //vtkm_woklet_particleadvection_field_h
