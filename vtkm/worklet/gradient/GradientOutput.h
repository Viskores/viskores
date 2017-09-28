//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_gradient_GradientOutput_h
#define vtk_m_worklet_gradient_GradientOutput_h

#include <vtkm/BaseComponent.h>

#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagExecObject.h>

#include <vtkm/exec/ExecutionObjectBase.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

#include <vtkm/worklet/gradient/Divergence.h>
#include <vtkm/worklet/gradient/QCriterion.h>
#include <vtkm/worklet/gradient/Vorticity.h>

namespace vtkm
{
namespace exec
{

template <typename T, typename DeviceAdapter>
struct GradientScalarOutput : public vtkm::exec::ExecutionObjectBase
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::BaseComponent<T>::Type;

  struct PortalTypes
  {
    using HandleType = vtkm::cont::ArrayHandle<ValueType>;
    using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;
    using Portal = typename ExecutionTypes::Portal;
  };

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

template <typename T, typename DeviceAdapter>
struct GradientVecOutput : public vtkm::exec::ExecutionObjectBase
{
  using ValueType = vtkm::Vec<T, 3>;
  using BaseTType = typename vtkm::BaseComponent<T>::Type;

  template <typename FieldType>
  struct PortalTypes
  {
    using HandleType = vtkm::cont::ArrayHandle<FieldType>;
    using ExecutionTypes = typename HandleType::template ExecutionTypes<DeviceAdapter>;
    using Portal = typename ExecutionTypes::Portal;
  };

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

template <typename T, typename DeviceAdapter>
struct GradientOutput : public GradientScalarOutput<T, DeviceAdapter>
{
#if defined(VTKM_MSVC) && (_MSC_VER == 1800) // workaround for VS2013
  template <typename... Args>
  GradientOutput(Args&&... args)
    : GradientScalarOutput<T, DeviceAdapter>::GradientScalarOutput(std::forward<Args>(args)...)
  {
  }
#else
  using GradientScalarOutput<T, DeviceAdapter>::GradientScalarOutput;
#endif
};

template <typename DeviceAdapter>
struct GradientOutput<vtkm::Vec<vtkm::Float32, 3>, DeviceAdapter>
  : public GradientVecOutput<vtkm::Vec<vtkm::Float32, 3>, DeviceAdapter>
{

#if defined(VTKM_MSVC) && (_MSC_VER == 1800) // workaround for VS2013
  template <typename... Args>
  GradientOutput(Args&&... args)
    : GradientVecOutput<vtkm::Vec<vtkm::Float32, 3>, DeviceAdapter>::GradientVecOutput(
        std::forward<Args>(args)...)
  {
  }
#else
  using GradientVecOutput<vtkm::Vec<vtkm::Float32, 3>, DeviceAdapter>::GradientVecOutput;
#endif
};

template <typename DeviceAdapter>
struct GradientOutput<vtkm::Vec<vtkm::Float64, 3>, DeviceAdapter>
  : public GradientVecOutput<vtkm::Vec<vtkm::Float64, 3>, DeviceAdapter>
{

#if defined(VTKM_MSVC) && (_MSC_VER == 1800) // workaround for VS2013
  template <typename... Args>
  GradientOutput(Args&&... args)
    : GradientVecOutput<vtkm::Vec<vtkm::Float64, 3>, DeviceAdapter>::GradientVecOutput(
        std::forward<Args>(args)...)
  {
  }
#else
  using GradientVecOutput<vtkm::Vec<vtkm::Float64, 3>, DeviceAdapter>::GradientVecOutput;
#endif
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
  using ExecObjectType = vtkm::exec::GradientOutput<typename ContObjectType::ValueType, Device>;

  template <typename InputDomainType>
  VTKM_CONT ExecObjectType operator()(ContObjectType object,
                                      const InputDomainType& vtkmNotUsed(inputDomain),
                                      vtkm::Id vtkmNotUsed(inputRange),
                                      vtkm::Id outputRange) const
  {
    return object.PrepareForOutput(outputRange, Device());
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
