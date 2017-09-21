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
#ifndef vtk_m_cont_ImplicitFunction_h
#define vtk_m_cont_ImplicitFunction_h

#include <vtkm/internal/Configure.h>

#include <vtkm/cont/VirtualObjectCache.h>
#include <vtkm/exec/ImplicitFunction.h>

#include <memory>

namespace vtkm
{
namespace cont
{

class VTKM_CONT_EXPORT ImplicitFunction
{
public:
  virtual ~ImplicitFunction();

  template <typename DeviceAdapter>
  vtkm::exec::ImplicitFunction PrepareForExecution(DeviceAdapter device) const
  {
    if (!this->Cache->GetValid())
    {
      this->SetDefaultDevices();
    }
    return this->Cache->GetVirtualObject(device);
  }

  void Modified() { this->Cache->SetRefreshFlag(true); }

protected:
  using CacheType = vtkm::cont::VirtualObjectCache<vtkm::exec::ImplicitFunction>;

  ImplicitFunction()
    : Cache(new CacheType)
  {
  }

  ImplicitFunction(ImplicitFunction&& other)
    : Cache(std::move(other.Cache))
  {
  }

  ImplicitFunction& operator=(ImplicitFunction&& other)
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

template <typename Derived>
class VTKM_ALWAYS_EXPORT ImplicitFunctionImpl : public ImplicitFunction
{
public:
  template <typename DeviceAdapterList>
  void ResetDevices(DeviceAdapterList devices)
  {
    this->Cache->Bind(static_cast<const Derived*>(this), devices);
  }

protected:
  ImplicitFunctionImpl() = default;
  ImplicitFunctionImpl(const ImplicitFunctionImpl&)
    : ImplicitFunction()
  {
  }

  // Cannot default due to a bug in VS2013
  ImplicitFunctionImpl(ImplicitFunctionImpl&& other)
    : ImplicitFunction(std::move(other))
  {
  }

  ImplicitFunctionImpl& operator=(const ImplicitFunctionImpl&) { return *this; }

  // Cannot default due to a bug in VS2013
  ImplicitFunctionImpl& operator=(ImplicitFunctionImpl&& other)
  {
    ImplicitFunction::operator=(std::move(other));
    return *this;
  }

  void SetDefaultDevices() const override { this->Cache->Bind(static_cast<const Derived*>(this)); }
};

//============================================================================
// ImplicitFunctions:

//============================================================================
/// \brief Implicit function for a box
class VTKM_ALWAYS_EXPORT Box : public ImplicitFunctionImpl<Box>
{
public:
  Box();
  Box(vtkm::Vec<FloatDefault, 3> minPoint, vtkm::Vec<FloatDefault, 3> maxPoint);
  Box(FloatDefault xmin,
      FloatDefault xmax,
      FloatDefault ymin,
      FloatDefault ymax,
      FloatDefault zmin,
      FloatDefault zmax);

  void SetMinPoint(const vtkm::Vec<FloatDefault, 3>& point);
  void SetMaxPoint(const vtkm::Vec<FloatDefault, 3>& point);

  const vtkm::Vec<FloatDefault, 3>& GetMinPoint() const;
  const vtkm::Vec<FloatDefault, 3>& GetMaxPoint() const;

  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3>& x) const;
  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const;

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3>& x) const;
  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault x, FloatDefault y, FloatDefault z) const;

private:
  vtkm::Vec<FloatDefault, 3> MinPoint;
  vtkm::Vec<FloatDefault, 3> MaxPoint;
};

//============================================================================
/// \brief Implicit function for a cylinder
class VTKM_ALWAYS_EXPORT Cylinder : public ImplicitFunctionImpl<Cylinder>
{
public:
  Cylinder();
  Cylinder(const vtkm::Vec<FloatDefault, 3>& axis, FloatDefault radius);
  Cylinder(const vtkm::Vec<FloatDefault, 3>& center,
           const vtkm::Vec<FloatDefault, 3>& axis,
           FloatDefault radius);

  void SetCenter(const vtkm::Vec<FloatDefault, 3>& center);
  void SetAxis(const vtkm::Vec<FloatDefault, 3>& axis);
  void SetRadius(FloatDefault radius);

  const vtkm::Vec<FloatDefault, 3>& GetCenter() const;
  const vtkm::Vec<FloatDefault, 3>& GetAxis() const;
  FloatDefault GetRadius() const;

  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3>& x) const;
  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const;

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3>& x) const;
  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault x, FloatDefault y, FloatDefault z) const;

private:
  vtkm::Vec<FloatDefault, 3> Center;
  vtkm::Vec<FloatDefault, 3> Axis;
  FloatDefault Radius;
};

//============================================================================
/// \brief Implicit function for a frustum
class VTKM_ALWAYS_EXPORT Frustum : public ImplicitFunctionImpl<Frustum>
{
public:
  Frustum();
  Frustum(const vtkm::Vec<FloatDefault, 3> points[6], const vtkm::Vec<FloatDefault, 3> normals[6]);
  explicit Frustum(const vtkm::Vec<FloatDefault, 3> points[8]);

  void SetPlanes(const vtkm::Vec<FloatDefault, 3> points[6],
                 const vtkm::Vec<FloatDefault, 3> normals[6]);
  void SetPlane(int idx, vtkm::Vec<FloatDefault, 3>& point, vtkm::Vec<FloatDefault, 3>& normal);

  void GetPlanes(vtkm::Vec<FloatDefault, 3> points[6], vtkm::Vec<FloatDefault, 3> normals[6]) const;
  const vtkm::Vec<FloatDefault, 3>* GetPoints() const;
  const vtkm::Vec<FloatDefault, 3>* GetNormals() const;

  // The points should be specified in the order of hex-cell vertices
  void CreateFromPoints(const vtkm::Vec<FloatDefault, 3> points[8]);

  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const;
  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3>& x) const;

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault, FloatDefault, FloatDefault) const;
  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3>&) const;

private:
  vtkm::Vec<FloatDefault, 3> Points[6];
  vtkm::Vec<FloatDefault, 3> Normals[6];
};

//============================================================================
/// \brief Implicit function for a plane
class VTKM_ALWAYS_EXPORT Plane : public ImplicitFunctionImpl<Plane>
{
public:
  Plane();
  explicit Plane(const vtkm::Vec<FloatDefault, 3>& normal);
  Plane(const vtkm::Vec<FloatDefault, 3>& origin, const vtkm::Vec<FloatDefault, 3>& normal);

  void SetOrigin(const vtkm::Vec<FloatDefault, 3>& origin);
  void SetNormal(const vtkm::Vec<FloatDefault, 3>& normal);

  const vtkm::Vec<FloatDefault, 3>& GetOrigin() const;
  const vtkm::Vec<FloatDefault, 3>& GetNormal() const;

  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const;
  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3>& x) const;

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault, FloatDefault, FloatDefault) const;
  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3>&) const;

private:
  vtkm::Vec<FloatDefault, 3> Origin;
  vtkm::Vec<FloatDefault, 3> Normal;
};

//============================================================================
/// \brief Implicit function for a sphere
class VTKM_ALWAYS_EXPORT Sphere : public ImplicitFunctionImpl<Sphere>
{
public:
  Sphere();
  explicit Sphere(FloatDefault radius);
  Sphere(vtkm::Vec<FloatDefault, 3> center, FloatDefault radius);

  void SetRadius(FloatDefault radius);
  void SetCenter(const vtkm::Vec<FloatDefault, 3>& center);

  FloatDefault GetRadius() const;
  const vtkm::Vec<FloatDefault, 3>& GetCenter() const;

  VTKM_EXEC_CONT
  FloatDefault Value(FloatDefault x, FloatDefault y, FloatDefault z) const;
  VTKM_EXEC_CONT
  FloatDefault Value(const vtkm::Vec<FloatDefault, 3>& x) const;

  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(FloatDefault x, FloatDefault y, FloatDefault z) const;
  VTKM_EXEC_CONT
  vtkm::Vec<FloatDefault, 3> Gradient(const vtkm::Vec<FloatDefault, 3>& x) const;

private:
  FloatDefault Radius;
  vtkm::Vec<FloatDefault, 3> Center;
};
}
} // vtkm::cont

#include <vtkm/cont/ImplicitFunction.hxx>

#endif // vtk_m_cont_ImplicitFunction_h
