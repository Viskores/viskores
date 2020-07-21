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

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/CellInterpolate.h>

namespace vtkm
{
namespace worklet
{
namespace particleadvection
{

class ExecutionField : public vtkm::VirtualObjectBase
{
public:
  VTKM_EXEC_CONT
  virtual ~ExecutionField() noexcept override {}

  VTKM_EXEC
  virtual void GetValue(const vtkm::VecVariable<vtkm::Id, 8>& indices,
                        const vtkm::Id vertices,
                        const vtkm::Vec3f& parametric,
                        const vtkm::UInt8 cellShape,
                        vtkm::VecVariable<vtkm::Vec3f, 2>& value) const = 0;
};

template <typename DeviceAdapter, typename FieldArrayType>
class ExecutionVelocityField : public vtkm::worklet::particleadvection::ExecutionField
{
public:
  using FieldPortalType =
    typename FieldArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  VTKM_CONT
  ExecutionVelocityField(FieldArrayType velocityValues, vtkm::cont::Token& token)
    : VelocityValues(velocityValues.PrepareForInput(DeviceAdapter(), token))
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

template <typename DeviceAdapter, typename FieldArrayType>
class ExecutionElectroMagneticField : public vtkm::worklet::particleadvection::ExecutionField
{
public:
  using FieldPortalType =
    typename FieldArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst;

  VTKM_CONT
  ExecutionElectroMagneticField(FieldArrayType electricValues,
                                FieldArrayType magneticValues,
                                vtkm::cont::Token& token)
    : ElectricValues(electricValues.PrepareForInput(DeviceAdapter(), token))
    , MagneticValues(magneticValues.PrepareForInput(DeviceAdapter(), token))
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

class Field : public vtkm::cont::ExecutionObjectBase
{
public:
  using HandleType = vtkm::cont::VirtualObjectHandle<ExecutionField>;

  virtual ~Field() = default;

  VTKM_CONT
  virtual const ExecutionField* PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId,
                                                    vtkm::cont::Token& token) const = 0;
};

template <typename FieldArrayType>
class VelocityField : public vtkm::worklet::particleadvection::Field
{
public:
  VTKM_CONT
  VelocityField(const FieldArrayType& fieldValues)
    : FieldValues(fieldValues)
  {
  }

  struct VelocityFieldFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              FieldArrayType fieldValues,
                              HandleType& execHandle,
                              vtkm::cont::Token& token) const
    {
      using ExecutionType = ExecutionVelocityField<DeviceAdapter, FieldArrayType>;
      ExecutionType* execObject = new ExecutionType(fieldValues, token);
      execHandle.Reset(execObject);
      return true;
    }
  };

  VTKM_CONT
  const ExecutionField* PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId,
                                            vtkm::cont::Token& token) const override
  {
    const bool success = vtkm::cont::TryExecuteOnDevice(
      deviceId, VelocityFieldFunctor(), this->FieldValues, this->ExecHandle, token);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("SingleCellTypeInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId, token);
  }

private:
  FieldArrayType FieldValues;
  mutable HandleType ExecHandle;
};

template <typename FieldArrayType>
class ElectroMagneticField : public vtkm::worklet::particleadvection::Field
{
public:
  VTKM_CONT
  ElectroMagneticField(const FieldArrayType& electricField, const FieldArrayType& magneticField)
    : ElectricField(electricField)
    , MagneticField(magneticField)
  {
  }

  struct ElectroMagneticFieldFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT bool operator()(DeviceAdapter,
                              FieldArrayType electricField,
                              FieldArrayType magneticField,
                              HandleType& execHandle,
                              vtkm::cont::Token& token) const
    {
      using ExecutionType = ExecutionElectroMagneticField<DeviceAdapter, FieldArrayType>;
      ExecutionType* execObject = new ExecutionType(electricField, magneticField, token);
      execHandle.Reset(execObject);
      return true;
    }
  };

  VTKM_CONT
  const ExecutionField* PrepareForExecution(vtkm::cont::DeviceAdapterId deviceId,
                                            vtkm::cont::Token& token) const override
  {
    const bool success = vtkm::cont::TryExecuteOnDevice(deviceId,
                                                        ElectroMagneticFieldFunctor(),
                                                        this->ElectricField,
                                                        this->MagneticField,
                                                        this->ExecHandle,
                                                        token);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("SingleCellTypeInterpolationHelper", deviceId);
    }
    return this->ExecHandle.PrepareForExecution(deviceId, token);
  }

private:
  FieldArrayType ElectricField;
  FieldArrayType MagneticField;
  mutable HandleType ExecHandle;
};

} // namespace particleadvection
} // namespace worklet
} // namespace
#endif //vtkm_woklet_particleadvection_field_h
