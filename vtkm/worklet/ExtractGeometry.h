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
#ifndef vtkm_m_worklet_ExtractGeometry_h
#define vtkm_m_worklet_ExtractGeometry_h

#include <vtkm/worklet/extraction/ExtractPoints.h>
#include <vtkm/worklet/extraction/ExtractCellsExplicit.h>

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/ImplicitFunctions.h>

namespace vtkm {
namespace worklet {

class ExtractGeometry : public vtkm::worklet::WorkletMapPointToCell
{
public:
  ExtractGeometry() {}

  // Extract points by id
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> RunExtractPoints(
                                    const CellSetType &cellSet,
                                    const vtkm::cont::ArrayHandle<vtkm::Id> &pointIds,
                                    const vtkm::cont::CoordinateSystem &coordinates,
                                    DeviceAdapter device)
  {
    ExtractPoints extractPoints;
    return extractPoints.Run(cellSet,
                             pointIds,
                             coordinates,
                             device);
  }

  // Extract points by implicit function
  template <typename CellSetType,
            typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> RunExtractPoints(
                                    const CellSetType &cellSet,
                                    const ImplicitFunction &implicitFunction,
                                    const vtkm::cont::CoordinateSystem &coordinates,
                                    DeviceAdapter device)
  {
    ExtractPoints extractPoints;
    return extractPoints.Run(cellSet,
                             implicitFunction,
                             coordinates,
                             device);
  }

  // Extract by cell ids
  template <typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> RunExtractCellsExplicit(
                                    const vtkm::cont::CellSetExplicit<> &cellSet,
                                    const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                    DeviceAdapter device)
  {
    ExtractCellsExplicit extractCells;
    return extractCells.Run(cellSet, 
                            cellIds, 
                            device); 
  }

  // Extract by ImplicitFunction volume of interest
  template <typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetExplicit<> RunExtractCellsExplicit(
                                    const vtkm::cont::CellSetExplicit<> &cellSet,
                                    const ImplicitFunction &implicitFunction,
                                    const vtkm::cont::CoordinateSystem &coordinates,
                                    DeviceAdapter device)
  {
    ExtractCellsExplicit extractCells;
    return extractCells.Run(cellSet,
                            implicitFunction,
                            coordinates,
                            device);
  }
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractGeometry_h
