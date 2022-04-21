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

#include <vtkm/filter/vector_analysis/worklet/gradient/Divergence.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/QCriterion.h>
#include <vtkm/filter/vector_analysis/worklet/gradient/Vorticity.h>

namespace vtkm
{
namespace exec
{

template <typename T>
struct GradientScalarOutputExecutionObject
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  using HandleType = vtkm::cont::ArrayHandle<ValueType>;
  using PortalType = typename HandleType::WritePortalType;

  GradientScalarOutputExecutionObject() = default;

  GradientScalarOutputExecutionObject(vtkm::cont::ArrayHandle<ValueType> gradient,
                                      vtkm::Id size,
                                      vtkm::cont::DeviceAdapterId device,
                                      vtkm::cont::Token& token)
  {
    this->GradientPortal = gradient.PrepareForOutput(size, device, token);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  void Set(vtkm::Id index, const vtkm::Vec<T, 3>& value) const
  {
    this->GradientPortal.Set(index, value);
  }

  PortalType GradientPortal;
};

template <typename T>
struct GradientScalarOutput : public vtkm::cont::ExecutionObjectBase
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  VTKM_CONT vtkm::exec::GradientScalarOutputExecutionObject<T> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return vtkm::exec::GradientScalarOutputExecutionObject<T>(
      this->Gradient, this->Size, device, token);
  }

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

template <typename T>
struct GradientVecOutputExecutionObject
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  template <typename FieldType>
  using PortalType = typename vtkm::cont::ArrayHandle<FieldType>::WritePortalType;

  GradientVecOutputExecutionObject() = default;

  GradientVecOutputExecutionObject(bool g,
                                   bool d,
                                   bool v,
                                   bool q,
                                   vtkm::cont::ArrayHandle<ValueType> gradient,
                                   vtkm::cont::ArrayHandle<BaseTType> divergence,
                                   vtkm::cont::ArrayHandle<vtkm::Vec<BaseTType, 3>> vorticity,
                                   vtkm::cont::ArrayHandle<BaseTType> qcriterion,
                                   vtkm::Id size,
                                   vtkm::cont::DeviceAdapterId device,
                                   vtkm::cont::Token& token)
  {
    this->SetGradient = g;
    this->SetDivergence = d;
    this->SetVorticity = v;
    this->SetQCriterion = q;

    if (g)
    {
      this->GradientPortal = gradient.PrepareForOutput(size, device, token);
    }
    if (d)
    {
      this->DivergencePortal = divergence.PrepareForOutput(size, device, token);
    }
    if (v)
    {
      this->VorticityPortal = vorticity.PrepareForOutput(size, device, token);
    }
    if (q)
    {
      this->QCriterionPortal = qcriterion.PrepareForOutput(size, device, token);
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

  PortalType<ValueType> GradientPortal;
  PortalType<BaseTType> DivergencePortal;
  PortalType<vtkm::Vec<BaseTType, 3>> VorticityPortal;
  PortalType<BaseTType> QCriterionPortal;
};

template <typename T>
struct GradientVecOutput : public vtkm::cont::ExecutionObjectBase
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::VecTraits<T>::BaseComponentType;

  VTKM_CONT vtkm::exec::GradientVecOutputExecutionObject<T> PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const
  {
    return vtkm::exec::GradientVecOutputExecutionObject<T>(this->G,
                                                           this->D,
                                                           this->V,
                                                           this->Q,
                                                           this->Gradient,
                                                           this->Divergence,
                                                           this->Vorticity,
                                                           this->Qcriterion,
                                                           this->Size,
                                                           device,
                                                           token);
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
  using ExecObjectFactoryType = vtkm::exec::GradientOutput<typename ContObjectType::ValueType>;
  using ExecObjectType = decltype(
    std::declval<ExecObjectFactoryType>().PrepareForExecution(Device(),
                                                              std::declval<vtkm::cont::Token&>()));

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const InputDomainType& vtkmNotUsed(inputDomain),
                                      vtkm::Id vtkmNotUsed(inputRange),
                                      vtkm::Id outputRange,
                                      vtkm::cont::Token& token) const
  {
    ExecObjectFactoryType ExecutionObjectFactory = object.PrepareForOutput(outputRange);
    return ExecutionObjectFactory.PrepareForExecution(Device(), token);
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
