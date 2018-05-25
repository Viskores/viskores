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
#ifndef vtkm_m_worklet_ExtractGeometry_h
#define vtkm_m_worklet_ExtractGeometry_h

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/ImplicitFunctionHandle.h>

namespace vtkm
{
namespace worklet
{

class ExtractGeometry
{
public:
  struct BoolType : vtkm::ListTagBase<bool>
  {
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify cells within volume of interest
  class ExtractCellsByVOI : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    using ControlSignature = void(CellSetIn cellset,
                                  WholeArrayIn<Vec3> coordinates,
                                  FieldOutCell<BoolType> passFlags);
    using ExecutionSignature = _3(PointCount, PointIndices, _2);

    VTKM_CONT
    ExtractCellsByVOI()
      : Function()
    {
    }

    VTKM_CONT
    ExtractCellsByVOI(const vtkm::ImplicitFunction* function,
                      bool extractInside,
                      bool extractBoundaryCells,
                      bool extractOnlyBoundaryCells)
      : Function(function)
      , ExtractInside(extractInside)
      , ExtractBoundaryCells(extractBoundaryCells)
      , ExtractOnlyBoundaryCells(extractOnlyBoundaryCells)
    {
    }

    template <typename ConnectivityInVec, typename InVecFieldPortalType>
    VTKM_EXEC bool operator()(vtkm::Id numIndices,
                              const ConnectivityInVec& connectivityIn,
                              const InVecFieldPortalType& coordinates) const
    {
      // Count points inside/outside volume of interest
      vtkm::IdComponent inCnt = 0;
      vtkm::IdComponent outCnt = 0;
      vtkm::Id indx;
      for (indx = 0; indx < numIndices; indx++)
      {
        vtkm::Id ptId = connectivityIn[static_cast<vtkm::IdComponent>(indx)];
        vtkm::Vec<FloatDefault, 3> coordinate = coordinates.Get(ptId);
        vtkm::FloatDefault value = this->Function->Value(coordinate);
        if (value <= 0)
          inCnt++;
        if (value >= 0)
          outCnt++;
      }

      // Decide if cell is extracted
      bool passFlag = false;
      if (inCnt == numIndices && ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (outCnt == numIndices && !ExtractInside && !ExtractOnlyBoundaryCells)
      {
        passFlag = true;
      }
      else if (inCnt > 0 && outCnt > 0 && (ExtractBoundaryCells || ExtractOnlyBoundaryCells))
      {
        passFlag = true;
      }
      return passFlag;
    }

  private:
    const vtkm::ImplicitFunction* Function;
    bool ExtractInside;
    bool ExtractBoundaryCells;
    bool ExtractOnlyBoundaryCells;
  };

  class AddPermutationCellSet
  {
    vtkm::cont::DynamicCellSet* Output;
    vtkm::cont::ArrayHandle<vtkm::Id>* ValidIds;

  public:
    AddPermutationCellSet(vtkm::cont::DynamicCellSet& cellOut,
                          vtkm::cont::ArrayHandle<vtkm::Id>& validIds)
      : Output(&cellOut)
      , ValidIds(&validIds)
    {
    }

    template <typename CellSetType>
    void operator()(const CellSetType& cellset) const
    {
      vtkm::cont::CellSetPermutation<CellSetType> permCellSet(
        *this->ValidIds, cellset, cellset.GetName());
      *this->Output = permCellSet;
    }
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract cells by ids permutes input data
  template <typename CellSetType, typename DeviceAdapter>
  vtkm::cont::CellSetPermutation<CellSetType> Run(const CellSetType& cellSet,
                                                  const vtkm::cont::ArrayHandle<vtkm::Id>& cellIds,
                                                  DeviceAdapter)
  {
    using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;
    using OutputType = vtkm::cont::CellSetPermutation<CellSetType>;

    DeviceAlgorithm::Copy(cellIds, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet, cellSet.GetName());
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract cells by implicit function permutes input data
  template <typename CellSetType, typename DeviceAdapter>
  vtkm::cont::CellSetPermutation<CellSetType> Run(
    const CellSetType& cellSet,
    const vtkm::cont::CoordinateSystem& coordinates,
    const vtkm::cont::ImplicitFunctionHandle& implicitFunction,
    bool extractInside,
    bool extractBoundaryCells,
    bool extractOnlyBoundaryCells,
    DeviceAdapter device)
  {
    // Worklet output will be a boolean passFlag array
    vtkm::cont::ArrayHandle<bool> passFlags;

    ExtractCellsByVOI worklet(implicitFunction.PrepareForExecution(device),
                              extractInside,
                              extractBoundaryCells,
                              extractOnlyBoundaryCells);
    DispatcherMapTopology<ExtractCellsByVOI, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet, coordinates, passFlags);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::CopyIf(
      indices, passFlags, this->ValidCellIds);

    // generate the cellset
    return vtkm::cont::CellSetPermutation<CellSetType>(
      this->ValidCellIds, cellSet, cellSet.GetName());
  }

  template <typename ValueType, typename StorageTagIn, typename DeviceTag>
  vtkm::cont::ArrayHandle<ValueType> ProcessCellField(
    const vtkm::cont::ArrayHandle<ValueType, StorageTagIn>& input,
    const DeviceTag&)
  {
    using Algo = vtkm::cont::DeviceAdapterAlgorithm<DeviceTag>;

    // Use a temporary permutation array to simplify the mapping:
    auto tmp = vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input);

    // Copy into an array with default storage:
    vtkm::cont::ArrayHandle<ValueType> result;
    Algo::Copy(tmp, result);

    return result;
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};
}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractGeometry_h
