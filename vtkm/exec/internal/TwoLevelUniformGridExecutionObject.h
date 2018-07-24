//
// Created by Matthew Letter on 6/8/18.
//
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
#ifndef vtk_m_exec_TwoLevelUniformGridExecutonObject_h
#define vtk_m_exec_TwoLevelUniformGridExecutonObject_h

#include <vtkm/cont/ArrayHandle.h>



namespace vtkm
{
namespace exec
{
namespace twolevelgrid
{
using DimensionType = vtkm::Int16;
using DimVec3 = vtkm::Vec<DimensionType, 3>;
using FloatVec3 = vtkm::Vec<vtkm::FloatDefault, 3>;

struct Grid
{
  DimVec3 Dimensions;
  FloatVec3 Origin;
  FloatVec3 BinSize;
};
template <typename Device>
struct TwoLevelUniformGridExecutionObject
{


  template <typename T>
  using ArrayPortalConst =
    typename vtkm::cont::ArrayHandle<T>::template ExecutionTypes<Device>::PortalConst;

  Grid TopLevel;

  ArrayPortalConst<DimVec3> LeafDimensions;
  ArrayPortalConst<vtkm::Id> LeafStartIndex;

  ArrayPortalConst<vtkm::Id> CellStartIndex;
  ArrayPortalConst<vtkm::Id> CellCount;
  ArrayPortalConst<vtkm::Id> CellIds;
};
}
}
}
#endif // vtk_m_cont_TwoLevelUniformGridExecutonObject_h
