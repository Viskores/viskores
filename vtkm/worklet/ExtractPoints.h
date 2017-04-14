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
#ifndef vtkm_m_worklet_ExtractPoints_h
#define vtkm_m_worklet_ExtractPoints_h

#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/ImplicitFunctions.h>

namespace vtkm {
namespace worklet {

class ExtractPoints
{
public:
  struct BoolType : vtkm::ListTagBase<bool> {};

  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify points within volume of interest
  template<typename ImplicitFunction>
  class ExtractPointsByVOI : public vtkm::worklet::WorkletMapCellToPoint
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  FieldInPoint<Vec3> coordinates,
                                  FieldOutPoint<BoolType> passFlags);
    typedef   _3 ExecutionSignature(_2);

    VTKM_CONT
    ExtractPointsByVOI(const ImplicitFunction &function) :
                                     Function(function) {}

    VTKM_EXEC
    bool operator()(const vtkm::Vec<vtkm::Float64,3> &coordinate) const
    {
      bool pass = true;
      vtkm::Float64 value = this->Function.Value(coordinate);
      if (value > 0)
        pass = false;
      return pass;
    }

  private:
    ImplicitFunction Function;
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by id creates new cellset of vertex cells
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(
                                    const CellSetType &cellSet,
                                    const vtkm::cont::ArrayHandle<vtkm::Id> &pointIds,
                                    DeviceAdapter)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;
    DeviceAlgorithm::Copy(pointIds, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType< > outCellSet(cellSet.GetName());
    outCellSet.Fill(cellSet.GetNumberOfPoints(),
                    vtkm::CellShapeTagVertex::Id,
                    1,
                    this->ValidPointIds);

    return outCellSet;
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract points by implicit function
  template <typename CellSetType,
            typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(
                                    const CellSetType &cellSet,
                                    const vtkm::cont::CoordinateSystem &coordinates,
                                    const ImplicitFunction &implicitFunction,
                                    DeviceAdapter)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;

    vtkm::cont::ArrayHandle<bool> passFlags;

    // Worklet output will be a boolean passFlag array
    typedef ExtractPointsByVOI<ImplicitFunction> ExtractPointsWorklet;

    ExtractPointsWorklet worklet(implicitFunction);
    DispatcherMapTopology<ExtractPointsWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet, coordinates, passFlags);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    DeviceAlgorithm::CopyIf(indices, passFlags, this->ValidPointIds);

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType< > outCellSet(cellSet.GetName());
    outCellSet.Fill(cellSet.GetNumberOfPoints(),
                    vtkm::CellShapeTagVertex::Id,
                    1,
                    this->ValidPointIds);

    return outCellSet;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidPointIds;
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractPoints_h
