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

class ExtractPoints : public vtkm::worklet::WorkletMapPointToCell
{
public:
  ExtractPoints() {}

  template<typename ImplicitFunction>
  class ExtractPointsWithImplicitFunction : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<Vec3> coordinates,
                                  FieldOut<IdComponentType> mask);
    typedef   _2 ExecutionSignature(_1);

    VTKM_CONT
    ExtractPointsWithImplicitFunction(const ImplicitFunction &function) : 
                                         Function(function) {}

    VTKM_CONT
    vtkm::IdComponent operator()(const vtkm::Vec<vtkm::Float64,3> &coordinate) const
    {
      vtkm::Float64 value = this->Function.Value(coordinate);
      vtkm::Float64 gradient = this->Function.Value(coordinate);
      vtkm::IdComponent mask = 0;
      if (value <= 0)
        mask = 1;
std::cout << "Coord " << coordinate[0] << " , " << coordinate[1] << " , " << coordinate[2] << "  value " << value << " mask " << mask << "  GRADIENT " << gradient << std::endl;
      return mask;
    }

  private:
    ImplicitFunction Function;
  };

  template <typename CellSetType,
            typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(
                                    const CellSetType &cellSet,
                                    const ImplicitFunction &implicitFunction,
                                    const vtkm::cont::CoordinateSystem &coordinates,
                                    DeviceAdapter device)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> maskArray;
    vtkm::Id numberOfInputPoints = cellSet.GetNumberOfPoints();
    DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, numberOfInputPoints),
                          maskArray);

    // Worklet output will be a boolean passFlag array
    typedef ExtractPointsWithImplicitFunction<ImplicitFunction> ExtractPointsWorklet;
    ExtractPointsWorklet worklet(implicitFunction);
    DispatcherMapField<ExtractPointsWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(coordinates, maskArray);

    vtkm::worklet::ScatterCounting PointScatter(maskArray, DeviceAdapter(), true);
    vtkm::cont::ArrayHandle<vtkm::Id> pointIds = PointScatter.GetOutputToInputMap();

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType< > outCellSet("cells");
    outCellSet.Fill(numberOfInputPoints,
                    vtkm::CellShapeTagVertex::Id,
                    1,
                    pointIds);

    return outCellSet;
  }
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractPoints_h
