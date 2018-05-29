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
struct CellGradientInType : vtkm::ListTagBase<T>
{
};

template <typename T>
struct CellGradient : vtkm::worklet::WorkletMapPointToCell
{
  using ControlSignature = void(CellSetIn,
                                FieldInPoint<Vec3> pointCoordinates,
                                FieldInPoint<CellGradientInType<T>> inputField,
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
    vtkm::Vec<vtkm::FloatDefault, 3> center =
      vtkm::exec::ParametricCoordinatesCenter(pointCount, shape, *this);

    outputGradient = vtkm::exec::CellDerivative(field, wCoords, center, shape, *this);
  }
};
}
}
}

#endif
