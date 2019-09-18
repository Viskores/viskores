//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
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
using FloatVec3 = vtkm::Vec3f;

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
