//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtkm_filter_flow_worklet_Field_h
#define vtkm_filter_flow_worklet_Field_h

#include <vtkm/Types.h>

#include <vtkm/VecVariable.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/CellInterpolate.h>

namespace vtkm
{
namespace worklet
{
namespace flow
{

template <typename FieldArrayType>
class ExecutionVelocityField
{
public:
  using FieldPortalType = typename FieldArrayType::ReadPortalType;
  using Association = vtkm::cont::Field::Association;
  using DelegateToField = std::false_type;

  VTKM_CONT
  ExecutionVelocityField(const FieldArrayType& velocityValues,
                         const Association assoc,
                         vtkm::cont::DeviceAdapterId device,
                         vtkm::cont::Token& token)
    : VelocityValues(velocityValues.PrepareForInput(device, token))
    , Assoc(assoc)
  {
  }

  VTKM_EXEC Association GetAssociation() const { return this->Assoc; }

  VTKM_EXEC void GetValue(const vtkm::Id cellId, vtkm::VecVariable<vtkm::Vec3f, 2>& value) const
  {
    VTKM_ASSERT(this->Assoc == Association::Cells);

    vtkm::Vec3f velocity = VelocityValues.Get(cellId);
    value = vtkm::make_Vec(velocity);
  }

  VTKM_EXEC void GetValue(const vtkm::VecVariable<vtkm::Id, 8>& indices,
                          const vtkm::Id vertices,
                          const vtkm::Vec3f& parametric,
                          const vtkm::UInt8 cellShape,
                          vtkm::VecVariable<vtkm::Vec3f, 2>& value) const
  {
    VTKM_ASSERT(this->Assoc == Association::Points);

    vtkm::Vec3f velocityInterp;
    vtkm::VecVariable<vtkm::Vec3f, 8> velocities;
    for (vtkm::IdComponent i = 0; i < vertices; i++)
      velocities.Append(VelocityValues.Get(indices[i]));
    vtkm::exec::CellInterpolate(velocities, parametric, cellShape, velocityInterp);
    value = vtkm::make_Vec(velocityInterp);
  }

  template <typename Point, typename Locator, typename Helper>
  VTKM_EXEC bool GetValue(const Point& vtkmNotUsed(point),
                          const vtkm::FloatDefault& vtkmNotUsed(time),
                          vtkm::VecVariable<Point, 2>& vtkmNotUsed(out),
                          const Locator& vtkmNotUsed(locator),
                          const Helper& vtkmNotUsed(helper)) const
  {
    //TODO Raise Error : Velocity Field should not allow this path
    return false;
  }

private:
  FieldPortalType VelocityValues;
  Association Assoc;
};

template <typename FieldArrayType>
class VelocityField : public vtkm::cont::ExecutionObjectBase
{
public:
  using ExecutionType = ExecutionVelocityField<FieldArrayType>;
  using Association = vtkm::cont::Field::Association;

  VTKM_CONT
  VelocityField() = default;

  VTKM_CONT
  VelocityField(const FieldArrayType& fieldValues)
    : FieldValues(fieldValues)
    , Assoc(vtkm::cont::Field::Association::Points)
  {
  }

  VTKM_CONT
  VelocityField(const FieldArrayType& fieldValues, const Association assoc)
    : FieldValues(fieldValues)
    , Assoc(assoc)
  {
    if (assoc != Association::Points && assoc != Association::Cells)
      throw("Unsupported field association");
  }

  VTKM_CONT
  const ExecutionType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                          vtkm::cont::Token& token) const
  {
    return ExecutionType(this->FieldValues, this->Assoc, device, token);
  }

private:
  FieldArrayType FieldValues;
  Association Assoc;
};

template <typename FieldArrayType>
class ExecutionElectroMagneticField
{
public:
  using FieldPortalType = typename FieldArrayType::ReadPortalType;
  using Association = vtkm::cont::Field::Association;
  using DelegateToField = std::false_type;

  VTKM_CONT
  ExecutionElectroMagneticField(const FieldArrayType& electricValues,
                                const FieldArrayType& magneticValues,
                                const Association assoc,
                                vtkm::cont::DeviceAdapterId device,
                                vtkm::cont::Token& token)
    : ElectricValues(electricValues.PrepareForInput(device, token))
    , MagneticValues(magneticValues.PrepareForInput(device, token))
    , Assoc(assoc)
  {
  }

  VTKM_EXEC Association GetAssociation() const { return this->Assoc; }

  VTKM_EXEC void GetValue(const vtkm::Id cellId, vtkm::VecVariable<vtkm::Vec3f, 2>& value) const
  {
    VTKM_ASSERT(this->Assoc == Association::Cells);

    auto electric = this->ElectricValues.Get(cellId);
    auto magnetic = this->MagneticValues.Get(cellId);
    value = vtkm::make_Vec(electric, magnetic);
  }

  VTKM_EXEC void GetValue(const vtkm::VecVariable<vtkm::Id, 8>& indices,
                          const vtkm::Id vertices,
                          const vtkm::Vec3f& parametric,
                          const vtkm::UInt8 cellShape,
                          vtkm::VecVariable<vtkm::Vec3f, 2>& value) const
  {
    VTKM_ASSERT(this->Assoc == Association::Points);

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

  template <typename Point, typename Locator, typename Helper>
  VTKM_EXEC bool GetValue(const Point& vtkmNotUsed(point),
                          const vtkm::FloatDefault& vtkmNotUsed(time),
                          vtkm::VecVariable<Point, 2>& vtkmNotUsed(out),
                          const Locator& vtkmNotUsed(locator),
                          const Helper& vtkmNotUsed(helper)) const
  {
    //TODO : Raise Error : Velocity Field should not allow this path
    return false;
  }

private:
  FieldPortalType ElectricValues;
  FieldPortalType MagneticValues;
  Association Assoc;
};

template <typename FieldArrayType>
class ElectroMagneticField : public vtkm::cont::ExecutionObjectBase
{
public:
  using ExecutionType = ExecutionElectroMagneticField<FieldArrayType>;
  using Association = vtkm::cont::Field::Association;

  VTKM_CONT
  ElectroMagneticField() = default;

  VTKM_CONT
  ElectroMagneticField(const FieldArrayType& electricField, const FieldArrayType& magneticField)
    : ElectricField(electricField)
    , MagneticField(magneticField)
    , Assoc(vtkm::cont::Field::Association::Points)
  {
  }

  VTKM_CONT
  ElectroMagneticField(const FieldArrayType& electricField,
                       const FieldArrayType& magneticField,
                       const Association assoc)
    : ElectricField(electricField)
    , MagneticField(magneticField)
    , Assoc(assoc)
  {
  }

  VTKM_CONT
  const ExecutionType PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                          vtkm::cont::Token& token) const
  {
    return ExecutionType(this->ElectricField, this->MagneticField, this->Assoc, device, token);
  }

private:
  FieldArrayType ElectricField;
  FieldArrayType MagneticField;
  Association Assoc;
};

}
}
} //vtkm::worklet::flow

#endif //vtkm_filter_flow_worklet_Field_h
