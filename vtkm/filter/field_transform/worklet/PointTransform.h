//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_PointTransform_h
#define vtk_m_worklet_PointTransform_h

#include <vtkm/Math.h>
#include <vtkm/Matrix.h>
#include <vtkm/Transform3D.h>
#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{

class PointTransform : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1);

  VTKM_CONT
  explicit PointTransform(const vtkm::Matrix<vtkm::FloatDefault, 4, 4>& m)
    : matrix(m)
  {
  }

  //Functor
  VTKM_EXEC
  vtkm::Vec<vtkm::FloatDefault, 3> operator()(const vtkm::Vec<vtkm::FloatDefault, 3>& vec) const
  {
    return vtkm::Transform3DPoint(matrix, vec);
  }

private:
  const vtkm::Matrix<vtkm::FloatDefault, 4, 4> matrix;
};
}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_PointTransform_h
