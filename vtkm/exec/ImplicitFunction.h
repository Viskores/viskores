//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_ImplicitFunction_h
#define vtk_m_exec_ImplicitFunction_h

#include <vtkm/Types.h>

namespace vtkm
{
namespace exec
{

class VTKM_ALWAYS_EXPORT ImplicitFunction
{
public:
  ImplicitFunction()
    : Function(nullptr)
    , ValueCaller(nullptr)
    , GradientCaller(nullptr)
  {
  }

  VTKM_EXEC
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return this->ValueCaller(this->Function, x, y, z);
  }

  VTKM_EXEC
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3>& x) const
  {
    return this->ValueCaller(this->Function, x[0], x[1], x[2]);
  }

  VTKM_EXEC
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return this->GradientCaller(this->Function, x, y, z);
  }

  VTKM_EXEC
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3>& x) const
  {
    return this->GradientCaller(this->Function, x[0], x[1], x[2]);
  }

  template <typename T>
  VTKM_EXEC void Bind(const T* function)
  {
    this->Function = function;
    this->ValueCaller = [](const void* t, FloatDefault x, FloatDefault y, FloatDefault z) {
      return static_cast<const T*>(t)->Value(x, y, z);
    };
    this->GradientCaller = [](const void* t, FloatDefault x, FloatDefault y, FloatDefault z) {
      return static_cast<const T*>(t)->Gradient(x, y, z);
    };
  }

private:
  using ValueCallerSig = FloatDefault(const void*, FloatDefault, FloatDefault, FloatDefault);
  using GradientCallerSig = vtkm::Vec<FloatDefault, 3>(const void*,
                                                       FloatDefault,
                                                       FloatDefault,
                                                       FloatDefault);

  const void* Function;
  ValueCallerSig* ValueCaller;
  GradientCallerSig* GradientCaller;
};

/// \brief A function object that evaluates the contained implicit function
class VTKM_ALWAYS_EXPORT ImplicitFunctionValue
{
public:
  ImplicitFunctionValue() = default;

  explicit ImplicitFunctionValue(const vtkm::exec::ImplicitFunction& func)
    : Function(func)
  {
  }

  VTKM_EXEC
  FloatDefault operator()(const vtkm::Vec<FloatDefault, 3> x) const
  {
    return this->Function.Value(x);
  }

  VTKM_EXEC
  FloatDefault operator()(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return this->Function.Value(x, y, z);
  }

private:
  vtkm::exec::ImplicitFunction Function;
};

/// \brief A function object that computes the gradient of the contained implicit
/// function and the specified point.
class VTKM_ALWAYS_EXPORT ImplicitFunctionGradient
{
public:
  ImplicitFunctionGradient() = default;

  explicit ImplicitFunctionGradient(const vtkm::exec::ImplicitFunction& func)
    : Function(func)
  {
  }

  VTKM_EXEC
  vtkm::Vec<FloatDefault, 3> operator()(const vtkm::Vec<FloatDefault, 3> x) const
  {
    return this->Function.Gradient(x);
  }

  VTKM_EXEC
  vtkm::Vec<FloatDefault, 3> operator()(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return this->Function.Gradient(x, y, z);
  }

private:
  vtkm::exec::ImplicitFunction Function;
};
}
} // vtkm::exec

#endif // vtk_m_exec_ImplicitFunction_h
