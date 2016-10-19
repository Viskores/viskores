//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_ImplicitFunctions_h
#define vtk_m_ImplicitFunctions_h

#include <vtkm/Types.h>
#include <iostream>

namespace vtkm {

/// \brief Implicit function for a plane
class Plane
{
public:
  VTKM_CONT
  Plane()
    : Origin(FloatDefault(0)),
      Normal(FloatDefault(0), FloatDefault(0), FloatDefault(1))
  { }

  VTKM_CONT
  explicit Plane(const vtkm::Vec<FloatDefault, 3> &normal)
    : Origin(FloatDefault(0)),
      Normal(normal)
  { }

  VTKM_CONT
  Plane(const vtkm::Vec<FloatDefault, 3> &origin,
        const vtkm::Vec<FloatDefault, 3> &normal)
    : Origin(origin), Normal(normal)
  { }

  VTKM_EXEC_CONT
  const vtkm::Vec<FloatDefault, 3>& GetOrigin() const
  {
    return this->Origin;
  }

  VTKM_EXEC_CONT
  const vtkm::Vec<FloatDefault, 3>& GetNormal() const
  {
    return this->Normal;
  }

  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return ((x - this->Origin[0]) * this->Normal[0]) +
           ((y - this->Origin[1]) * this->Normal[1]) +
           ((z - this->Origin[2]) * this->Normal[2]);
  }

  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3> &x) const
  {
    return this->Value(x[0], x[1], x[2]);
  }

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault, FloatDefault, FloatDefault) const
  {
    return this->Normal;
  }

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3>&) const
  {
    return this->Normal;
  }

private:
  vtkm::Vec<FloatDefault, 3> Origin;
  vtkm::Vec<FloatDefault, 3> Normal;
};


/// \brief Implicit function for a sphere
class Sphere
{
public:
  VTKM_CONT
  Sphere() : Radius(FloatDefault(0.2)), Center(FloatDefault(0))
  { }

  VTKM_CONT
  explicit Sphere(FloatDefault radius) : Radius(radius), Center(FloatDefault(0))
  { }

  VTKM_CONT
  Sphere(vtkm::Vec<FloatDefault, 3> center, FloatDefault radius)
    : Radius(radius), Center(center)
  { }

  VTKM_EXEC_CONT
  FloatDefault GetRadius() const
  {
    return this->Radius;
  }

  VTKM_EXEC_CONT
  const vtkm::Vec<FloatDefault, 3>& GetCenter() const
  {
    return this->Center;
  }

  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return ((x - this->Center[0]) * (x - this->Center[0]) +
            (y - this->Center[1]) * (y - this->Center[1]) +
            (z - this->Center[2]) * (z - this->Center[2])) -
           (this->Radius * this->Radius);
  }

  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3> &x) const
  {
    return this->Value(x[0], x[1], x[2]);
  }

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault x, FloatDefault y, FloatDefault z)
    const
  {
    return this->Gradient(vtkm::Vec<FloatDefault, 3>(x, y, z));
  }

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3> &x) const
  {
    return FloatDefault(2) * (x - this->Center);
  }

private:
  FloatDefault Radius;
  vtkm::Vec<FloatDefault, 3> Center;
};

/// \brief A function object that evaluates the contained implicit function
template <typename ImplicitFunction>
class ImplicitFunctionValue
{
public:
  VTKM_CONT
  ImplicitFunctionValue()
    : Function()
  { }

  VTKM_CONT
  explicit ImplicitFunctionValue(const ImplicitFunction &func)
    : Function(func)
  { }

  VTKM_EXEC_CONT
  FloatDefault operator()(const vtkm::Vec<FloatDefault, 3> x) const
  {
    return this->Function.Value(x);
  }

  VTKM_EXEC_CONT
  FloatDefault operator()(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return this->Function.Value(x, y, z);
  }

private:
  ImplicitFunction Function;
};

/// \brief A function object that computes the gradient of the contained implicit
/// function and the specified point.
template <typename ImplicitFunction>
class ImplicitFunctionGradient
{
public:
  VTKM_CONT
  ImplicitFunctionGradient()
    : Function()
  { }

  VTKM_CONT
  explicit ImplicitFunctionGradient(const ImplicitFunction &func)
    : Function(func)
  { }

  VTKM_EXEC_CONT
  FloatDefault operator()(const vtkm::Vec<FloatDefault, 3> x) const
  {
    return this->Function.Gradient(x);
  }

  VTKM_EXEC_CONT
  FloatDefault operator()(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return this->Function.Gradient(x, y, z);
  }

private:
  ImplicitFunction Function;
};

} // namespace vtkm

#endif // vtk_m_ImplicitFunctions_h
