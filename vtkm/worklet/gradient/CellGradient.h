//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_gradient_CellGradient_h
#define vtk_m_worklet_gradient_CellGradient_h

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ParametricCoordinates.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/worklet/gradient/GradientOutput.h>

namespace vtkm
{
namespace worklet
{
namespace gradient
{

template <typename T>
using CellGradientInType = vtkm::List<T>;

template <typename T>
struct CellGradient : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint pointCoordinates,
                                FieldInPoint inputField,
                                GradientOutputs outputFields);

  using ExecutionSignature = void(CellShape, PointCount, _2, _3, _4);
  using InputDomain = _1;

  template <typename CellTagType,
            typename PointCoordVecType,
            typename FieldInVecType,
            typename GradientOutType>
  VTKM_EXEC void operator()(CellTagType shape,
                            vtkm::IdComponent pointCount,
                            const PointCoordVecType& wCoords,
                            const FieldInVecType& field,
                            GradientOutType& outputGradient) const
  {
    vtkm::Vec3f center = vtkm::exec::ParametricCoordinatesCenter(pointCount, shape, *this);

    outputGradient = vtkm::exec::CellDerivative(field, wCoords, center, shape, *this);
  }
};
}
}
}

#endif
