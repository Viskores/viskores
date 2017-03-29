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

#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/DispatcherMapTopology.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/CellSetSingleType.h>
#include <vtkm/cont/CellSetPermutation.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/ImplicitFunctions.h>

namespace vtkm {
namespace worklet {

class ExtractGeometry
{
public:
  struct BoolType : vtkm::ListTagBase<bool> {};

  ////////////////////////////////////////////////////////////////////////////////////
  // Worklet to identify cells within volume of interest
  template <typename ImplicitFunction>
  class ExtractCellsByVOI : public vtkm::worklet::WorkletMapPointToCell
  {
  public:
    typedef void ControlSignature(CellSetIn cellset,
                                  WholeArrayIn<Vec3> coordinates,
                                  FieldOut<BoolType> passFlags);
    typedef   _3 ExecutionSignature(PointCount, PointIndices, _2);

    VTKM_CONT
    explicit ExtractCellsByVOI(const ImplicitFunction &function)
                                           : Function(function) {}

    template <typename ConnectivityInVec, typename InVecFieldPortalType>
    VTKM_CONT
    bool operator()(const vtkm::IdComponent &numIndices,
                    const ConnectivityInVec &connectivityIn,
                    const InVecFieldPortalType &coordinates) const
    {
      // If any point is outside volume of interest, cell is also
      bool pass = true;
      for (vtkm::IdComponent indx = 0; indx < numIndices; indx++)
      {
        vtkm::Id ptId = connectivityIn[indx];
        vtkm::Vec<FloatDefault,3> coordinate = coordinates.Get(ptId);
        vtkm::FloatDefault value = this->Function.Value(coordinate);
        if (value > 0)
          pass = false;
      }
      return pass;
    }

  private:
    ImplicitFunction Function;
  };

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract cells by ids permutes input data
  template <typename CellSetType,
            typename DeviceAdapter>
  vtkm::cont::CellSetPermutation<CellSetType> Run(
                                    const CellSetType &cellSet,
                                    const vtkm::cont::ArrayHandle<vtkm::Id> &cellIds,
                                    DeviceAdapter)
  {
    typedef typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> DeviceAlgorithm;
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutputType;

    DeviceAlgorithm::Copy(cellIds, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet, cellSet.GetName());
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Extract cells by implicit function permutes input data
  template <typename CellSetType,
            typename ImplicitFunction,
            typename DeviceAdapter>
  vtkm::cont::CellSetPermutation<CellSetType> Run(
                                    const CellSetType &cellSet,
                                    const ImplicitFunction &implicitFunction,
                                    const vtkm::cont::CoordinateSystem &coordinates,
                                    DeviceAdapter)
  {
    typedef vtkm::cont::CellSetPermutation<CellSetType> OutputType;

    vtkm::cont::ArrayHandle<bool> passFlags;

    // Worklet output will be a boolean passFlag array
    typedef ExtractCellsByVOI<ImplicitFunction> ExtractCellsWorklet;

    ExtractCellsWorklet worklet(implicitFunction);
    DispatcherMapTopology<ExtractCellsWorklet, DeviceAdapter> dispatcher(worklet);
    dispatcher.Invoke(cellSet,
                      coordinates,
                      passFlags);

    vtkm::cont::ArrayHandleCounting<vtkm::Id> indices =
      vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0), vtkm::Id(1), passFlags.GetNumberOfValues());
    vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>
        ::CopyIf(indices, passFlags, this->ValidCellIds);

    return OutputType(this->ValidCellIds, cellSet, cellSet.GetName());
  }

  ////////////////////////////////////////////////////////////////////////////////////
  // Permute cell data to match permuted cells
  class PermuteCellData
  {
  public:
    PermuteCellData(const vtkm::cont::ArrayHandle<vtkm::Id> validCellIds,
                    vtkm::cont::DynamicArrayHandle &data)
      : ValidCellIds(validCellIds), Data(&data)
    { }

    template <typename ArrayHandleType>
    void operator()(const ArrayHandleType &input) const
    {
      *(this->Data) = vtkm::cont::DynamicArrayHandle(
        vtkm::cont::make_ArrayHandlePermutation(this->ValidCellIds, input));
    }

  private:
    vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
    vtkm::cont::DynamicArrayHandle *Data;
  };

  vtkm::cont::Field ProcessCellField(const vtkm::cont::Field field) const
  {
    if (field.GetAssociation() != vtkm::cont::Field::ASSOC_CELL_SET)
    {
      throw vtkm::cont::ErrorBadValue("Expecting cell field.");
    }

    vtkm::cont::DynamicArrayHandle data;
    CastAndCall(field, PermuteCellData(this->ValidCellIds, data));

    return vtkm::cont::Field(field.GetName(), field.GetAssociation(),
                             field.GetAssocCellSet(), data);
  }

private:
  vtkm::cont::ArrayHandle<vtkm::Id> ValidCellIds;
};

}
} // namespace vtkm::worklet

#endif // vtkm_m_worklet_ExtractGeometry_h
