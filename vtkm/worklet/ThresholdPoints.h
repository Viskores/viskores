//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_ThresholdPoints_h
#define vtkm_m_worklet_ThresholdPoints_h

#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace worklet {

// Threshold points on predicate producing CellSetSingleType<VERTEX> with
// resulting subset of points
class ThresholdPoints : public vtkm::worklet::WorkletMapPointToCell
{
public:
  template <typename UnaryPredicate>
  class ThresholdPointField : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> scalars,
                                  FieldOut<IdComponentType> mask);
    typedef _2 ExecutionSignature(_1);

    VTKM_CONT
    ThresholdPointField() : Predicate() { }

    VTKM_CONT
    explicit ThresholdPointField(const UnaryPredicate &predicate)
      : Predicate(predicate)
    { }

    template<typename ScalarType>
    VTKM_EXEC
    vtkm::IdComponent operator()(const ScalarType &scalar) const
    {
      bool pass = this->Predicate(scalar);
      vtkm::IdComponent mask = 0;
      if (pass == true)
        mask = 1;
      return mask;
    }

  private:
    UnaryPredicate Predicate;
  };

  template <typename CellSetType, 
            typename UnaryPredicate, 
            typename ValueType, 
            typename StorageType, 
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(
                          const CellSetType &cellSet,
                          const vtkm::cont::ArrayHandle<ValueType, StorageType>& fieldArray,
                          const UnaryPredicate &predicate,
                          DeviceAdapter device)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> maskArray;
    vtkm::Id numberOfInputPoints = cellSet.GetNumberOfPoints();
    DeviceAlgorithm::Copy(vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>(0, numberOfInputPoints),
                          maskArray);
    
    // Worklet output will be a boolean passFlag array
    typedef ThresholdPointField<UnaryPredicate> ThresholdWorklet;
    ThresholdWorklet worklet(predicate);
    DispatcherMapField<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(fieldArray, maskArray);

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

#endif // vtkm_m_worklet_ThresholdPoints_h
