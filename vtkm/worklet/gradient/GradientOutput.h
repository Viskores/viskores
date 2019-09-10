//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_gradient_GradientOutput_h
#define vtk_m_worklet_gradient_GradientOutput_h

#include <vtkm/VecTraits.h>

#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagExecObject.h>

#include <vtkm/cont/ExecutionObjectBase.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

#include <vtkm/worklet/gradient/Divergence.h>
#include <vtkm/worklet/gradient/QCriterion.h>
#include <vtkm/worklet/gradient/Vorticity.h>

namespace vtkm
{
namespace exec
{
template <typename T, typename DeviceAdapter>
struct GradientScalarOutputExecutionObject
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  struct PortalTypes
  {
    using HandleType = vtkm::cont::ArrayHandle<ValueType>;
    using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;
    using Portal = typename ExecutionTypes::Portal;
  };

  GradientScalarOutputExecutionObject() = default;

  GradientScalarOutputExecutionObject(vtkm::cont::ArrayHandle<ValueType> gradient, vtkm::Id size)
  {
    this->GradientPortal = gradient.PrepareForOutput(size, DeviceAdapter());
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void Set(vtkm::Id index, const vtkm::Vec<T, 3>& value) const
  {
    this->GradientPortal.Set(index, value);
  }

  typename PortalTypes::Portal GradientPortal;
};

template <typename T>
struct GradientScalarOutput : public vtkm::cont::ExecutionObjectBase
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;
  template <typename Device>

  VTKM_CONT vtkm::exec::GradientScalarOutputExecutionObject<T, Device> PrepareForExecution(
    Device) const
  {
    return vtkm::exec::GradientScalarOutputExecutionObject<T, Device>(this->Gradient, this->Size);
  }

  GradientScalarOutput() = default;

  GradientScalarOutput(bool,
                       bool,
                       bool,
                       bool,
                       vtkm::cont::ArrayHandle<ValueType>& gradient,
                       vtkm::cont::ArrayHandle<BaseTType>&,
                       vtkm::cont::ArrayHandle<vtkm::Vec<BaseTType, 3>>&,
                       vtkm::cont::ArrayHandle<BaseTType>&,
                       vtkm::Id size)
    : Size(size)
    , Gradient(gradient)
  {
  }
  vtkm::Id Size;
  vtkm::cont::ArrayHandle<ValueType> Gradient;
};

template <typename T, typename DeviceAdapter>
struct GradientVecOutputExecutionObject
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  template <typename FieldType>
  struct PortalTypes
  {
    using HandleType = vtkm::cont::ArrayHandle<FieldType>;
    using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;
    using Portal = typename ExecutionTypes::Portal;
  };

  GradientVecOutputExecutionObject() = default;

  GradientVecOutputExecutionObject(bool g,
                                   bool d,
                                   bool v,
                                   bool q,
                                   vtkm::cont::ArrayHandle<ValueType> gradient,
                                   vtkm::cont::ArrayHandle<BaseTType> divergence,
                                   vtkm::cont::ArrayHandle<vtkm::Vec<BaseTType, 3>> vorticity,
                                   vtkm::cont::ArrayHandle<BaseTType> qcriterion,
                                   vtkm::Id size)
  {
    this->SetGradient = g;
    this->SetDivergence = d;
    this->SetVorticity = v;
    this->SetQCriterion = q;

    DeviceAdapter device;
    if (g)
    {
      this->GradientPortal = gradient.PrepareForOutput(size, device);
    }
    if (d)
    {
      this->DivergencePortal = divergence.PrepareForOutput(size, device);
    }
    if (v)
    {
      this->VorticityPortal = vorticity.PrepareForOutput(size, device);
    }
    if (q)
    {
      this->QCriterionPortal = qcriterion.PrepareForOutput(size, device);
    }
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void Set(vtkm::Id index, const vtkm::Vec<T, 3>& value) const
  {
    if (this->SetGradient)
    {
      this->GradientPortal.Set(index, value);
    }
    if (this->SetDivergence)
    {
      vtkm::worklet::gradient::Divergence divergence;
      BaseTType output;
      divergence(value, output);
      this->DivergencePortal.Set(index, output);
    }
    if (this->SetVorticity)
    {
      vtkm::worklet::gradient::Vorticity vorticity;
      T output;
      vorticity(value, output);
      this->VorticityPortal.Set(index, output);
    }
    if (this->SetQCriterion)
    {
      vtkm::worklet::gradient::QCriterion qc;
      BaseTType output;
      qc(value, output);
      this->QCriterionPortal.Set(index, output);
    }
  }

  bool SetGradient;
  bool SetDivergence;
  bool SetVorticity;
  bool SetQCriterion;

  typename PortalTypes<ValueType>::Portal GradientPortal;
  typename PortalTypes<BaseTType>::Portal DivergencePortal;
  typename PortalTypes<vtkm::Vec<BaseTType, 3>>::Portal VorticityPortal;
  typename PortalTypes<BaseTType>::Portal QCriterionPortal;
};

template <typename T>
struct GradientVecOutput : public vtkm::cont::ExecutionObjectBase
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  template <typename Device>
  VTKM_CONT vtkm::exec::GradientVecOutputExecutionObject<T, Device> PrepareForExecution(
    Device) const
  {
    return vtkm::exec::GradientVecOutputExecutionObject<T, Device>(this->G,
                                                                   this->D,
                                                                   this->V,
                                                                   this->Q,
                                                                   this->Gradient,
                                                                   this->Divergence,
                                                                   this->Vorticity,
                                                                   this->Qcriterion,
                                                                   this->Size);
  }

  GradientVecOutput() = default;

  GradientVecOutput(bool g,
                    bool d,
                    bool v,
                    bool q,
                    vtkm::cont::ArrayHandle<ValueType>& gradient,
                    vtkm::cont::ArrayHandle<BaseTType>& divergence,
                    vtkm::cont::ArrayHandle<vtkm::Vec<BaseTType, 3>>& vorticity,
                    vtkm::cont::ArrayHandle<BaseTType>& qcriterion,
                    vtkm::Id size)
  {
    this->G = g;
    this->D = d;
    this->V = v;
    this->Q = q;
    this->Gradient = gradient;
    this->Divergence = divergence;
    this->Vorticity = vorticity;
    this->Qcriterion = qcriterion;
    this->Size = size;
  }

  bool G;
  bool D;
  bool V;
  bool Q;
  vtkm::cont::ArrayHandle<ValueType> Gradient;
  vtkm::cont::ArrayHandle<BaseTType> Divergence;
  vtkm::cont::ArrayHandle<vtkm::Vec<BaseTType, 3>> Vorticity;
  vtkm::cont::ArrayHandle<BaseTType> Qcriterion;
  vtkm::Id Size;
};

template <typename T>
struct GradientOutput : public GradientScalarOutput<T>
{
  using GradientScalarOutput<T>::GradientScalarOutput;
};

template <>
struct GradientOutput<vtkm::Vec3f_32> : public GradientVecOutput<vtkm::Vec3f_32>
{
  using GradientVecOutput<vtkm::Vec3f_32>::GradientVecOutput;
};

template <>
struct GradientOutput<vtkm::Vec3f_64> : public GradientVecOutput<vtkm::Vec3f_64>
{
  using GradientVecOutput<vtkm::Vec3f_64>::GradientVecOutput;
};
}
} // namespace vtkm::exec


namespace vtkm
{
namespace cont
{
namespace arg
{

/// \brief \c Transport tag for output arrays.
///
/// \c TransportTagArrayOut is a tag used with the \c Transport class to
/// transport \c ArrayHandle objects for output data.
///
struct TransportTagGradientOut
{
};

template <typename ContObjectType, typename Device>
struct Transport<vtkm::cont::arg::TransportTagGradientOut, ContObjectType, Device>
{
  using ExecObjectFacotryType = vtkm::exec::GradientOutput<typename ContObjectType::ValueType>;
  using ExecObjectType =
    decltype(std::declval<ExecObjectFacotryType>().PrepareForExecution(Device()));

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const InputDomainType& vtkmNotUsed(inputDomain),
                                      vtkm::Id vtkmNotUsed(inputRange),
                                      vtkm::Id outputRange) const
  {
    ExecObjectFacotryType ExecutionObjectFacotry = object.PrepareForOutput(outputRange);
    return ExecutionObjectFacotry.PrepareForExecution(Device());
  }
};
}
}
} // namespace vtkm::cont::arg


namespace vtkm
{
namespace worklet
{
namespace gradient
{


struct GradientOutputs : vtkm::cont::arg::ControlSignatureTagBase
{
  using TypeCheckTag = vtkm::cont::arg::TypeCheckTagExecObject;
  using TransportTag = vtkm::cont::arg::TransportTagGradientOut;
  using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
};
}
}
} // namespace vtkm::worklet::gradient

#endif
