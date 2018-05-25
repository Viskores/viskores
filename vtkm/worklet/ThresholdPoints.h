//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_m_worklet_ThresholdPoints_h
#define vtkm_m_worklet_ThresholdPoints_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm
{
namespace worklet
{

class ThresholdPoints
{
public:
  struct BoolType : vtkm::ListTagBase<bool>
  {
  };

  template <typename UnaryPredicate>
  class ThresholdPointField : public vtkm::worklet::WorkletMapCellToPoint
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  FieldInPoint<ScalarAll> scalars,
                                  FieldOutPoint<BoolType> passFlags);
    using ExecutionSignature = _3(_2);

    VTKM_CONT
    ThresholdPointField()
      : Predicate()
    {
    }

    VTKM_CONT
    explicit ThresholdPointField(const UnaryPredicate& predicate)
      : Predicate(predicate)
    {
    }

    template <typename ScalarType>
    VTKM_EXEC bool operator()(const ScalarType& scalar) const
    {
      return this->Predicate(scalar);
    }

  private:
    UnaryPredicate Predicate;
  };

  template <typename CellSetType,
            typename ScalarsArrayHandle,
            typename UnaryPredicate,
            typename DeviceAdapter>
  vtkm::cont::CellSetSingleType<> Run(const CellSetType& cellSet,
                                      const ScalarsArrayHandle& scalars,
                                      const UnaryPredicate& predicate,
                                      DeviceAdapter)
  {
    using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

    vtkm::cont::ArrayHandle<bool> passFlags;

    using ThresholdWorklet = ThresholdPointField<UnaryPredicate>;

    ThresholdWorklet worklet(predicate);
    DispatcherMapTopology<ThresholdWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet, scalars, passFlags);

    vtkm::cont::ArrayHandle<vtkm::Id> pointIds;
    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    DeviceAlgorithm::CopyIf(indices, passFlags, pointIds);

    // Make CellSetSingleType with VERTEX at each point id
    vtkm::cont::CellSetSingleType<> outCellSet(cellSet.GetName());
    outCellSet.Fill(cellSet.GetNumberOfPoints(), vtkm::CellShapeTagVertex::Id, 1, pointIds);

    return outCellSet;
  }
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ThresholdPoints_h
