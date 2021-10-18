//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/MergePartitionedDataSet.h>

#include <numeric> // for std::accumulate
#include <vtkm/List.h>
#include <vtkm/TypeList.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/worklet/CellDeepCopy.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>


namespace vtkm
{
namespace cont
{

struct TransferCellsFunctor
{
  template <typename T>
  VTKM_CONT void operator()(const T& cellSetIn,
                            vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                            vtkm::cont::ArrayHandle<vtkm::Id>& numIndices,
                            vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                            vtkm::Id pointStartIndex) const
  {
    // allocate shapes and numIndices
    vtkm::Id cellStartIndex = shapes.GetNumberOfValues();
    shapes.Allocate(cellStartIndex + cellSetIn.GetNumberOfCells(), vtkm::CopyFlag::On);
    numIndices.Allocate(cellStartIndex + cellSetIn.GetNumberOfCells(), vtkm::CopyFlag::On);

    // fill the view of numIndices
    vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<vtkm::Id>> viewArrayNumIndices(
      numIndices, cellStartIndex, cellSetIn.GetNumberOfCells());
    vtkm::cont::Invoker invoke;
    invoke(vtkm::worklet::CellDeepCopy::CountCellPoints{}, cellSetIn, viewArrayNumIndices);

    // convert numIndices to offsets and derive numberOfConnectivity
    vtkm::cont::ArrayHandle<vtkm::Id> offsets;
    vtkm::Id numberOfConnectivity;
    vtkm::cont::ConvertNumComponentsToOffsets(viewArrayNumIndices, offsets, numberOfConnectivity);

    // allocate connectivity
    vtkm::Id connectivityStartIndex = connectivity.GetNumberOfValues();
    connectivity.Allocate(connectivityStartIndex + numberOfConnectivity, vtkm::CopyFlag::On);

    // fill the view of shapes and connectivity
    vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<vtkm::UInt8>> viewArrayShapes(
      shapes, cellStartIndex, cellSetIn.GetNumberOfCells());
    vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<vtkm::Id>> viewArrayConnectivity(
      connectivity, connectivityStartIndex, numberOfConnectivity);
    invoke(vtkm::worklet::CellDeepCopy::PassCellStructure{},
           cellSetIn,
           viewArrayShapes,
           vtkm::cont::make_ArrayHandleGroupVecVariable(viewArrayConnectivity, offsets));
    shapes.ReleaseResourcesExecution();
    offsets.ReleaseResourcesExecution();
    connectivity.ReleaseResourcesExecution();

    // point the connectivity to the point indices of this partition
    vtkm::cont::Algorithm::Transform(
      vtkm::cont::ArrayHandleConstant<vtkm::Id>(pointStartIndex, numberOfConnectivity),
      viewArrayConnectivity,
      viewArrayConnectivity,
      vtkm::Sum());
  }
};

void TransferCells(const vtkm::cont::DynamicCellSet& cellSetIn,
                   vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                   vtkm::cont::ArrayHandle<vtkm::Id>& numIndices,
                   vtkm::cont::ArrayHandle<vtkm::Id>& connectivity,
                   vtkm::Id startIndex)
{
  cellSetIn.CastAndCall(TransferCellsFunctor{}, shapes, numIndices, connectivity, startIndex);
}

struct TransferArrayFunctor
{
  template <typename T, typename S>
  VTKM_CONT void operator()(const vtkm::cont::ArrayHandle<T, S>& arrayIn,
                            vtkm::cont::UnknownArrayHandle& arrayOut,
                            vtkm::Id startIndex) const
  {
    vtkm::cont::ArrayHandleView<vtkm::cont::ArrayHandle<T>> viewArrayOut(
      arrayOut.AsArrayHandle<vtkm::cont::ArrayHandle<T>>(),
      startIndex,
      arrayIn.GetNumberOfValues());
    vtkm::cont::ArrayCopy(arrayIn, viewArrayOut);
  }
};

void TransferArray(const vtkm::cont::UnknownArrayHandle& arrayIn,
                   vtkm::cont::UnknownArrayHandle& arrayOut,
                   vtkm::Id startIndex)
{
  arrayIn.CastAndCallForTypes<
    VTKM_DEFAULT_TYPE_LIST,
    vtkm::List<vtkm::cont::StorageTagBasic, vtkm::cont::StorageTagUniformPoints>>(
    TransferArrayFunctor{}, arrayOut, startIndex);
}

//-----------------------------------------------------------------------------
VTKM_CONT
vtkm::cont::DataSet MergePartitionedDataSet(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet)
{
  // verify correctnees of data
  VTKM_ASSERT(partitionedDataSet.GetNumberOfPartitions() > 0);

  vtkm::cont::UnknownArrayHandle coordsOut;
  vtkm::cont::ArrayCopy(
    partitionedDataSet.GetPartition(0).GetCoordinateSystem().GetDataAsMultiplexer(), coordsOut);
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
  vtkm::cont::ArrayHandle<vtkm::Id> connectivity;
  int numberOfPointsSoFar = 0;
  for (unsigned int partitionId = 0; partitionId < partitionedDataSet.GetNumberOfPartitions();
       partitionId++)
  {
    auto partition = partitionedDataSet.GetPartition(partitionId);

    // Transfer points
    auto coordsIn = partition.GetCoordinateSystem().GetDataAsMultiplexer();
    coordsOut.Allocate(numberOfPointsSoFar + partition.GetNumberOfPoints(), vtkm::CopyFlag::On);
    TransferArray(coordsIn, coordsOut, numberOfPointsSoFar);

    // Transfer cells
    vtkm::cont::DynamicCellSet cellset;
    cellset = partition.GetCellSet();
    TransferCells(cellset, shapes, numIndices, connectivity, numberOfPointsSoFar);

    numberOfPointsSoFar += partition.GetNumberOfPoints();
  }

  // create dataset
  vtkm::cont::CellSetExplicit<> cellSet;
  vtkm::Id nPts = static_cast<vtkm::Id>(coordsOut.GetNumberOfValues());
  vtkm::cont::ArrayHandle<vtkm::Id> offsets;
  vtkm::cont::Algorithm::ScanExtended(numIndices, offsets);
  cellSet.Fill(nPts, shapes, connectivity, offsets);
  vtkm::cont::DataSet derivedDataSet;
  derivedDataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem(
    partitionedDataSet.GetPartition(0).GetCoordinateSystem().GetName(), coordsOut));
  derivedDataSet.SetCellSet(cellSet);

  // Transfer fields
  for (vtkm::IdComponent f = 0; f < partitionedDataSet.GetPartition(0).GetNumberOfFields(); f++)
  {
    std::string name = partitionedDataSet.GetPartition(0).GetField(f).GetName();
    vtkm::cont::UnknownArrayHandle outFieldHandle;
    vtkm::cont::ArrayCopy(partitionedDataSet.GetPartition(0).GetField(name).GetData(),
                          outFieldHandle);

    if (partitionedDataSet.GetPartition(0).GetField(name).IsFieldCell())
    {
      outFieldHandle.Allocate(derivedDataSet.GetNumberOfCells());
      unsigned int numberOfCellValuesSoFar = 0;
      for (unsigned int partitionId = 0; partitionId < partitionedDataSet.GetNumberOfPartitions();
           partitionId++)
      {
        try
        {
          auto cellField = partitionedDataSet.GetPartition(partitionId).GetField(name).GetData();
          TransferArray(cellField, outFieldHandle, numberOfCellValuesSoFar);
        }
        catch (const vtkm::cont::Error& error)
        {
          std::cout << "Partition 0 contains an array that partition " << partitionId
                    << " does not contain. The merged Dataset will have random values where values "
                       "were missing."
                    << std::endl;
          std::cout << error.GetMessage() << std::endl;
        }
        numberOfCellValuesSoFar += partitionedDataSet.GetPartition(partitionId).GetNumberOfCells();
      }
      derivedDataSet.AddCellField(name, outFieldHandle);
    }
    else
    {
      outFieldHandle.Allocate(derivedDataSet.GetNumberOfPoints());
      unsigned int numberOfPointValuesSoFar = 0;
      for (unsigned int partitionId = 0; partitionId < partitionedDataSet.GetNumberOfPartitions();
           partitionId++)
      {
        try
        {
          auto pointField = partitionedDataSet.GetPartition(partitionId).GetField(name).GetData();
          TransferArray(pointField, outFieldHandle, numberOfPointValuesSoFar);
        }
        //        catch (vtkm::cont::ErrorBadValue& error)
        catch (const vtkm::cont::Error& error)
        {
          std::cout << "Partition 0 contains an array that partition " << partitionId
                    << " does not contain. The merged Dataset will have random values where values "
                       "were missing."
                    << std::endl;
          std::cout << error.GetMessage() << std::endl;
        }
        numberOfPointValuesSoFar +=
          partitionedDataSet.GetPartition(partitionId).GetNumberOfPoints();
      }
      derivedDataSet.AddPointField(name, outFieldHandle);
    }
  }

  return derivedDataSet;
}

}
}
