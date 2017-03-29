//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2017 Sandia Corporation.
//  Copyright 2017 UT-Battelle, LLC.
//  Copyright 2017 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_ImplicitFunction_h
#define vtk_m_cont_ImplicitFunction_h

#include <vtkm/exec/ImplicitFunction.h>
#include <vtkm/cont/VirtualObjectCache.h>

#include <memory>


namespace vtkm {
namespace cont {

class VTKM_CONT_EXPORT ImplicitFunction
{
public:
  virtual ~ImplicitFunction();

  template<typename DeviceAdapter>
  vtkm::exec::ImplicitFunction PrepareForExecution(DeviceAdapter device) const
  {
    if (!this->Cache->GetValid())
    {
      this->SetDefaultDevices();
    }
    return this->Cache->GetVirtualObject(device);
  }

  void Modified()
  {
    this->Cache->SetRefreshFlag(true);
  }

protected:
  using CacheType = vtkm::cont::VirtualObjectCache<vtkm::exec::ImplicitFunction>;

  ImplicitFunction() : Cache(new CacheType)
  {
  }

  ImplicitFunction(ImplicitFunction &&other)
    : Cache(std::move(other.Cache))
  {
  }

  ImplicitFunction& operator=(ImplicitFunction &&other)
  {
    if (this != &other)
    {
      this->Cache = std::move(other.Cache);
    }
    return *this;
  }

  virtual void SetDefaultDevices() const = 0;

  std::unique_ptr<CacheType> Cache;
};


template<typename Derived>
class VTKM_ALWAYS_EXPORT ImplicitFunctionImpl : public ImplicitFunction
{
public:
  template<typename DeviceAdapterList>
  void ResetDevices(DeviceAdapterList devices)
  {
    this->Cache->Bind(static_cast<const Derived*>(this), devices);
  }

protected:
  ImplicitFunctionImpl() = default;
  ImplicitFunctionImpl(const ImplicitFunctionImpl &) : ImplicitFunction()
  {
  }

  // Cannot default due to a bug in VS2013
  ImplicitFunctionImpl(ImplicitFunctionImpl &&other)
    : ImplicitFunction(std::move(other))
  {
  }

  ImplicitFunctionImpl& operator=(const ImplicitFunctionImpl &)
  {
    return *this;
  }

  // Cannot default due to a bug in VS2013
  ImplicitFunctionImpl& operator=(ImplicitFunctionImpl &&other)
  {
    ImplicitFunction::operator=(std::move(other));
    return *this;
  }

  void SetDefaultDevices() const override
  {
    this->Cache->Bind(static_cast<const Derived*>(this));
  }
};


//============================================================================
// ImplicitFunctions:

//============================================================================
/// \brief Implicit function for a box
class VTKM_ALWAYS_EXPORT Box : public ImplicitFunctionImpl<Box>
{
public:
  Box() : MinPoint(vtkm::Vec<FloatDefault,3>(FloatDefault(0), FloatDefault(0), FloatDefault(0))),
          MaxPoint(vtkm::Vec<FloatDefault,3>(FloatDefault(1), FloatDefault(1), FloatDefault(1)))
  { }

  Box(vtkm::Vec<FloatDefault, 3> minPoint, vtkm::Vec<FloatDefault, 3> maxPoint)
    : MinPoint(minPoint), MaxPoint(maxPoint)
  { }

  Box(FloatDefault xmin, FloatDefault xmax,
      FloatDefault ymin, FloatDefault ymax,
      FloatDefault zmin, FloatDefault zmax)
  {
    MinPoint[0] = xmin;  MaxPoint[0] = xmax;
    MinPoint[1] = ymin;  MaxPoint[1] = ymax;
    MinPoint[2] = zmin;  MaxPoint[2] = zmax;
  }

  void SetMinPoint(const vtkm::Vec<FloatDefault, 3> &point)
  {
    this->MinPoint = point;
    this->Modified();
  }

  void SetMaxPoint(const vtkm::Vec<FloatDefault, 3> &point)
  {
    this->MaxPoint = point;
    this->Modified();
  }

  const vtkm::Vec<FloatDefault, 3>& GetMinPoint() const
  {
    return this->MinPoint;
  }

  const vtkm::Vec<FloatDefault, 3>& GetMaxPoint() const
  {
    return this->MaxPoint;
  }

  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3> &x) const;

  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const
  {
    return this->Value(vtkm::Vec<vtkm::FloatDefault,3>(x, y, z));
  }

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3> &x) const;

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault x, FloatDefault y, FloatDefault z)
    const
  {
    return this->Gradient(vtkm::Vec<FloatDefault, 3>(x, y, z));
  }

private:
  vtkm::Vec<FloatDefault, 3> MinPoint;
  vtkm::Vec<FloatDefault, 3> MaxPoint;
};


//============================================================================
/// \brief Implicit function for a plane
class VTKM_ALWAYS_EXPORT Plane : public ImplicitFunctionImpl<Plane>
{
public:
  Plane()
    : Origin(FloatDefault(0)),
      Normal(FloatDefault(0), FloatDefault(0), FloatDefault(1))
  { }

  explicit Plane(const vtkm::Vec<FloatDefault, 3> &normal)
    : Origin(FloatDefault(0)),
      Normal(normal)
  { }

  Plane(const vtkm::Vec<FloatDefault, 3> &origin,
        const vtkm::Vec<FloatDefault, 3> &normal)
    : Origin(origin), Normal(normal)
  { }

  void SetOrigin(const vtkm::Vec<FloatDefault, 3> &origin)
  {
    this->Origin = origin;
    this->Modified();
  }

  void SetNormal(const vtkm::Vec<FloatDefault, 3> &normal)
  {
    this->Normal = normal;
    this->Modified();
  }

  const vtkm::Vec<FloatDefault, 3>& GetOrigin() const
  {
    return this->Origin;
  }

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


//============================================================================
/// \brief Implicit function for a sphere
class VTKM_ALWAYS_EXPORT Sphere : public ImplicitFunctionImpl<Sphere>
{
public:
  Sphere() : Radius(FloatDefault(0.2)), Center(FloatDefault(0))
  { }

  explicit Sphere(FloatDefault radius) : Radius(radius), Center(FloatDefault(0))
  { }

  Sphere(vtkm::Vec<FloatDefault, 3> center, FloatDefault radius)
    : Radius(radius), Center(center)
  { }

  void SetRadius(FloatDefault radius)
  {
    this->Radius = radius;
    this->Modified();
  }

  void GetCenter(const vtkm::Vec<FloatDefault, 3> &center)
  {
    this->Center = center;
    this->Modified();
  }

  FloatDefault GetRadius() const
  {
    return this->Radius;
  }

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

}
} // vtkm::cont

#include <vtkm/cont/ImplicitFunction.hxx>

#endif // vtk_m_cont_ImplicitFunction_h
