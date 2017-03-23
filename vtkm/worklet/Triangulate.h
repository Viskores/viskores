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
#ifndef vtkm_m_worklet_Triangulate_h
#define vtkm_m_worklet_Triangulate_h

#include <vtkm/worklet/triangulate/TriangulateExplicit.h>
#include <vtkm/worklet/triangulate/TriangulateStructured.h>

namespace vtkm {
namespace worklet {

template <typename DeviceAdapter>
class Triangulate
{
public:
  Triangulate() {}

  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetExplicit<> &cellSet,
                                      vtkm::cont::ArrayHandle<vtkm::IdComponent> &outCellsPerCell)
  {
    TriangulateExplicit<DeviceAdapter> worklet;
    return worklet.Run(cellSet, outCellsPerCell); 
  }

  vtkm::cont::CellSetSingleType<> Run(const vtkm::cont::CellSetStructured<2> &cellSet)
  {
    TriangulateStructured<DeviceAdapter> worklet;
    return worklet.Run(cellSet); 
  }
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_Triangulate_h
