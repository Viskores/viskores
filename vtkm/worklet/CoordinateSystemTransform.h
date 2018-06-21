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

#ifndef vtk_m_worklet_CoordinateSystemTransform_h
#define vtk_m_worklet_CoordinateSystemTransform_h

#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/Transform3D.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{

template <typename T>
class CylindricalCoordinateTransform
{
public:
  struct CylToCar : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<Vec3>, FieldOut<Vec3>);
    using ExecutionSignature = _2(_1);


    //Functor
    VTKM_EXEC vtkm::Vec<T, 3> operator()(const vtkm::Vec<T, 3>& vec) const
    {
      vtkm::Vec<T, 3> res(vec[0] * static_cast<T>(vtkm::Cos(vec[1])),
                          vec[0] * static_cast<T>(vtkm::Sin(vec[1])),
                          vec[2]);
      return res;
    }
  };

  struct CarToCyl : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<Vec3>, FieldOut<Vec3>);
    using ExecutionSignature = _2(_1);

    //Functor
    VTKM_EXEC vtkm::Vec<T, 3> operator()(const vtkm::Vec<T, 3>& vec) const
    {
      T R = vtkm::Sqrt(vec[0] * vec[0] + vec[1] * vec[1]);
      T Theta = 0;

      if (vec[0] == 0 && vec[1] == 0)
        Theta = 0;
      else if (vec[0] < 0)
        Theta = -vtkm::ASin(vec[1] / R) + static_cast<T>(vtkm::Pi());
      else
        Theta = vtkm::ASin(vec[1] / R);

      vtkm::Vec<T, 3> res(R, Theta, vec[2]);
      return res;
    }
  };

  VTKM_CONT
  CylindricalCoordinateTransform()
    : cartesianToCylindrical(true)
  {
  }

  VTKM_CONT void SetCartesianToCylindrical() { cartesianToCylindrical = true; }
  VTKM_CONT void SetCylindricalToCartesian() { cartesianToCylindrical = false; }

  template <typename CoordsStorageType, typename DeviceAdapterTag>
  void Run(const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, CoordsStorageType>& inPoints,
           vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, CoordsStorageType>& outPoints,
           DeviceAdapterTag) const
  {
    if (cartesianToCylindrical)
    {
      vtkm::worklet::DispatcherMapField<CarToCyl> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<CylToCar> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

  template <typename CoordsStorageType, typename DeviceAdapterTag>
  void Run(const vtkm::cont::CoordinateSystem& inPoints,
           vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, CoordsStorageType>& outPoints,
           DeviceAdapterTag) const
  {
    if (cartesianToCylindrical)
    {
      vtkm::worklet::DispatcherMapField<CarToCyl> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<CylToCar> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

private:
  bool cartesianToCylindrical;
};

template <typename T>
class SphericalCoordinateTransform
{
public:
  struct SphereToCar : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<Vec3>, FieldOut<Vec3>);
    using ExecutionSignature = _2(_1);

    //Functor
    VTKM_EXEC vtkm::Vec<T, 3> operator()(const vtkm::Vec<T, 3>& vec) const
    {
      T R = vec[0];
      T Theta = vec[1];
      T Phi = vec[2];

      T sinTheta = static_cast<T>(vtkm::Sin(Theta));
      T cosTheta = static_cast<T>(vtkm::Cos(Theta));
      T sinPhi = static_cast<T>(vtkm::Sin(Phi));
      T cosPhi = static_cast<T>(vtkm::Cos(Phi));

      T x = R * sinTheta * cosPhi;
      T y = R * sinTheta * sinPhi;
      T z = R * cosTheta;

      vtkm::Vec<T, 3> r(x, y, z);
      return r;
    }
  };

  struct CarToSphere : public vtkm::worklet::WorkletMapField
  {
    using ControlSignature = void(FieldIn<Vec3>, FieldOut<Vec3>);
    using ExecutionSignature = _2(_1);

    //Functor
    VTKM_EXEC vtkm::Vec<T, 3> operator()(const vtkm::Vec<T, 3>& vec) const
    {
      T x = vec[0];
      T y = vec[1];
      T z = vec[2];

      T R = vtkm::Sqrt(x * x + y * y + z * z);
      T Theta = 0;
      if (R > 0)
        Theta = vtkm::ACos(z / R);
      T Phi = vtkm::ATan2(y, x);
      if (Phi < 0)
        Phi += static_cast<T>(vtkm::TwoPi());

      return vtkm::Vec<T, 3>(R, Theta, Phi);
    }
  };

  VTKM_CONT
  SphericalCoordinateTransform()
    : CartesianToSpherical(true)
  {
  }

  VTKM_CONT void SetCartesianToSpherical() { CartesianToSpherical = true; }
  VTKM_CONT void SetSphericalToCartesian() { CartesianToSpherical = false; }

  template <typename CoordsStorageType, typename DeviceAdapterTag>
  void Run(const vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, CoordsStorageType>& inPoints,
           vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, CoordsStorageType>& outPoints,
           DeviceAdapterTag) const
  {
    if (CartesianToSpherical)
    {
      vtkm::worklet::DispatcherMapField<CarToSphere, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<SphereToCar, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

  template <typename CoordsStorageType, typename DeviceAdapterTag>
  void Run(const vtkm::cont::CoordinateSystem& inPoints,
           vtkm::cont::ArrayHandle<vtkm::Vec<T, 3>, CoordsStorageType>& outPoints,
           DeviceAdapterTag) const
  {
    if (CartesianToSpherical)
    {
      vtkm::worklet::DispatcherMapField<CarToSphere, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
    else
    {
      vtkm::worklet::DispatcherMapField<SphereToCar, DeviceAdapterTag> dispatcher;
      dispatcher.Invoke(inPoints, outPoints);
    }
  }

private:
  bool CartesianToSpherical;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_CoordinateSystemTransform_h
