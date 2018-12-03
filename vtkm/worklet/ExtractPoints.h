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
#ifndef vtkm_m_worklet_ExtractPoints_h
#define vtkm_m_worklet_ExtractPoints_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ImplicitFunctionHandle.h>

namespace vtkm
{
namespace worklet
{

class ExtractPoints
{
public:
  struct BoolType : vtkm::ListTagBase<bool>
  {
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify points within volume of interest
  class ExtractPointsByVOI : public vtkm::worklet::WorkletMapCellToPoint
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<Vec3> coordinates,
                                  ExecObject function,
                                  FieldOutPoint<BoolType> passFlags);
    using ExecutionSignature = _4(_2, _3);

    VTKM_CONT
    ExtractPointsByVOI(bool extractInside)
      : passValue(extractInside)
      , failValue(!extractInside)
    {
    }

    VTKM_EXEC
    bool operator()(const vtkm::Vec<vtkm::Float64, 3>& coordinate,
                    const vtkm::ImplicitFunction* function) const
    {
      bool pass = passValue;
      vtkm::Float64 value = function->Value(coordinate);
      if (value > 0)
      {
        pass = failValue;
      }
      return pass;
    }

  private:
    bool passValue;
    bool failValue;
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by id creates new cellset of vertex cells
  template <typename CellSetType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      const vtkm::cont::ArrayHandle<vtkm::Id>& pointIds)
  {
    vtkm::cont::ArrayCopy(pointIds, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType<> outCellSet(cellSet.GetName());
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), vtkm::CellShapeTagVertex::Id, 1, this->ValidPointIds);

    return outCellSet;
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by implicit function
  template <typename CellSetType, typename CoordinateType>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      const CoordinateType& coordinates,
                                      const vtkm::cont::ImplicitFunctionHandle& implicitFunction,
                                      bool extractInside)
  {
    // Worklet output will be a boolean passFlag array
    vtkm::cont::ArrayHandle<bool> passFlags;

    ExtractPointsByVOI worklet(extractInside);
    DispatcherMapTopology<ExtractPointsByVOI> dispatcher(worklet);
    dispatcher.Invoke(cellSet, coordinates, implicitFunction, passFlags);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::Algorithm::CopyIf(indices, passFlags, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType<> outCellSet(cellSet.GetName());
    outCellSet.Fill(
      cellSet.GetNumberOfPoints(), vtkm::CellShapeTagVertex::Id, 1, this->ValidPointIds);

    return outCellSet;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidPointIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractPoints_h
