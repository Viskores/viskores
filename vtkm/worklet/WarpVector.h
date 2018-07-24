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

#ifndef vtk_m_worklet_WarpVector_h
#define vtk_m_worklet_WarpVector_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/VecTraits.h>

namespace vtkm
{
namespace worklet
{
// A functor that modify points by moving points along a vector
// then timing a scale factor. It's a VTK-m version of the vtkWarpVector in VTK.
// Useful for showing flow profiles or mechanical deformation.
// This worklet does not modify the input points but generate new point coordinate
// instance that has been warped.
class WarpVector
{
public:
  class WarpVectorImp : public vtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn<Vec3>, FieldIn<Vec3>, FieldOut<Vec3>);
    using ExecutionSignature = _3(_1, _2);
    VTKM_CONT
    WarpVectorImp(vtkm::FloatDefault scale)
      : Scale(scale)
    {
    }

    VTKM_EXEC
    vtkm::Vec<vtkm::FloatDefault, 3> operator()(
      const vtkm::Vec<vtkm::FloatDefault, 3>& point,
      const vtkm::Vec<vtkm::FloatDefault, 3>& vector) const
    {
      return point + this->Scale * vector;
    }

    template <typename T>
    VTKM_EXEC vtkm::Vec<T, 3> operator()(const vtkm::Vec<T, 3>& point,
                                         const vtkm::Vec<T, 3>& vector) const
    {
      return point + static_cast<T>(this->Scale) * vector;
    }


  private:
    vtkm::FloatDefault Scale;
  };

  // Execute the WarpVector worklet given the points, vector and a scale factor.
  // Returns:
  // warped points
  template <typename PointType, typename VectorType, typename ResultType, typename DeviceAdapter>
  void Run(PointType point,
           VectorType vector,
           vtkm::FloatDefault scale,
           ResultType warpedPoint,
           DeviceAdapter vtkmNotUsed(adapter))
  {
    WarpVectorImp warpVectorImp(scale);
    vtkm::worklet::DispatcherMapField<WarpVectorImp, DeviceAdapter> dispatcher(warpVectorImp);
    dispatcher.Invoke(point, vector, warpedPoint);
  }
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_WarpVector_h
