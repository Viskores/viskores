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

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/UnknownCellSet.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>

namespace
{

void CountPointsAndCells(const vtkm::cont::PartitionedDataSet& partitionedDataSet,
                         vtkm::Id& numPointsTotal,
                         vtkm::Id& numCellsTotal)
{
  numPointsTotal = 0;
  numCellsTotal = 0;

  for (vtkm::Id partitionId = 0; partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    vtkm::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    numPointsTotal += partition.GetNumberOfPoints();
    numCellsTotal += partition.GetNumberOfCells();
  }
}

struct PassCellShapesNumIndices : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn inputTopology, FieldOut shapes, FieldOut numIndices);
  using ExecutionSignature = void(CellShape, PointCount, _2, _3);

  template <typename CellShape>
  VTKM_EXEC void operator()(const CellShape& inShape,
                            vtkm::IdComponent inNumIndices,
                            vtkm::UInt8& outShape,
                            vtkm::IdComponent& outNumIndices) const
  {
    outShape = inShape.Id;
    outNumIndices = inNumIndices;
  }
};

void MergeShapes(const vtkm::cont::PartitionedDataSet& partitionedDataSet,
                 vtkm::Id numCellsTotal,
                 vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                 vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices)
{
  vtkm::cont::Invoker invoke;

  shapes.Allocate(numCellsTotal);
  numIndices.Allocate(numCellsTotal);

  vtkm::Id cellStartIndex = 0;
  for (vtkm::Id partitionId = 0; partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    vtkm::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    vtkm::Id numCellsPartition = partition.GetNumberOfCells();

    auto shapesView = vtkm::cont::make_ArrayHandleView(shapes, cellStartIndex, numCellsPartition);
    auto numIndicesView =
      vtkm::cont::make_ArrayHandleView(numIndices, cellStartIndex, numCellsPartition);

    invoke(PassCellShapesNumIndices{}, partition.GetCellSet(), shapesView, numIndicesView);

    cellStartIndex += numCellsPartition;
  }
  VTKM_ASSERT(cellStartIndex == numCellsTotal);
}

struct PassCellIndices : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn inputTopology, FieldOut pointIndices);
  using ExecutionSignature = void(PointIndices, _2);

  vtkm::Id IndexOffset;

  PassCellIndices(vtkm::Id indexOffset)
    : IndexOffset(indexOffset)
  {
  }

  template <typename InPointIndexType, typename OutPointIndexType>
  VTKM_EXEC void operator()(const InPointIndexType& inPoints, OutPointIndexType& outPoints) const
  {
    vtkm::IdComponent numPoints = inPoints.GetNumberOfComponents();
    VTKM_ASSERT(numPoints == outPoints.GetNumberOfComponents());
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; pointIndex++)
    {
      outPoints[pointIndex] = inPoints[pointIndex] + this->IndexOffset;
    }
  }
};

void MergeIndices(const vtkm::cont::PartitionedDataSet& partitionedDataSet,
                  const vtkm::cont::ArrayHandle<vtkm::Id>& offsets,
                  vtkm::Id numIndicesTotal,
                  vtkm::cont::ArrayHandle<vtkm::Id>& indices)
{
  vtkm::cont::Invoker invoke;

  indices.Allocate(numIndicesTotal);

  vtkm::Id pointStartIndex = 0;
  vtkm::Id cellStartIndex = 0;
  for (vtkm::Id partitionId = 0; partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    vtkm::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    vtkm::Id numCellsPartition = partition.GetNumberOfCells();

    auto offsetsView =
      vtkm::cont::make_ArrayHandleView(offsets, cellStartIndex, numCellsPartition + 1);
    auto indicesGroupView = vtkm::cont::make_ArrayHandleGroupVecVariable(indices, offsetsView);

    invoke(PassCellIndices{ pointStartIndex }, partition.GetCellSet(), indicesGroupView);

    pointStartIndex += partition.GetNumberOfPoints();
    cellStartIndex += numCellsPartition;
  }
  VTKM_ASSERT(cellStartIndex == (offsets.GetNumberOfValues() - 1));
}

vtkm::cont::CellSetExplicit<> MergeCellSets(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet,
  vtkm::Id numPointsTotal,
  vtkm::Id numCellsTotal)
{
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  MergeShapes(partitionedDataSet, numCellsTotal, shapes, numIndices);

  vtkm::cont::ArrayHandle<vtkm::Id> offsets;
  vtkm::Id numIndicesTotal;
  vtkm::cont::ConvertNumComponentsToOffsets(numIndices, offsets, numIndicesTotal);
  numIndices.ReleaseResources();

  vtkm::cont::ArrayHandle<vtkm::Id> indices;
  MergeIndices(partitionedDataSet, offsets, numIndicesTotal, indices);

  vtkm::cont::CellSetExplicit<> outCells;
  outCells.Fill(numPointsTotal, shapes, indices, offsets);
  return outCells;
}

struct ClearPartitionWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn indices, WholeArrayInOut array);
  using ExecutionSignature = void(WorkIndex, _2);

  vtkm::Id IndexOffset;

  ClearPartitionWorklet(vtkm::Id indexOffset)
    : IndexOffset(indexOffset)
  {
  }

  template <typename OutPortalType>
  VTKM_EXEC void operator()(vtkm::Id index, OutPortalType& outPortal) const
  {
    // It's weird to get a value from a portal only to override it, but the expect type
    // is weird (a variable-sized Vec), so this is the only practical way to set it.
    auto outVec = outPortal.Get(index + this->IndexOffset);
    for (vtkm::IdComponent comp = 0; comp < outVec.GetNumberOfComponents(); ++comp)
    {
      outVec[comp] = 0;
    }
    // Shouldn't really do anything.
    outPortal.Set(index + this->IndexOffset, outVec);
  }
};

template <typename OutArrayHandle>
void ClearPartition(OutArrayHandle& outArray, vtkm::Id startIndex, vtkm::Id numValues)
{
  vtkm::cont::Invoker invoke;
  invoke(ClearPartitionWorklet{ startIndex }, vtkm::cont::ArrayHandleIndex(numValues), outArray);
}

struct CopyPartitionWorklet : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn sourceArray, WholeArrayInOut mergedArray);
  using ExecutionSignature = void(WorkIndex, _1, _2);

  vtkm::Id IndexOffset;

  CopyPartitionWorklet(vtkm::Id indexOffset)
    : IndexOffset(indexOffset)
  {
  }

  template <typename InVecType, typename OutPortalType>
  VTKM_EXEC void operator()(vtkm::Id index, const InVecType& inVec, OutPortalType& outPortal) const
  {
    // It's weird to get a value from a portal only to override it, but the expect type
    // is weird (a variable-sized Vec), so this is the only practical way to set it.
    auto outVec = outPortal.Get(index + this->IndexOffset);
    VTKM_ASSERT(inVec.GetNumberOfComponents() == outVec.GetNumberOfComponents());
    for (vtkm::IdComponent comp = 0; comp < outVec.GetNumberOfComponents(); ++comp)
    {
      outVec[comp] = static_cast<typename decltype(outVec)::ComponentType>(inVec[comp]);
    }
    // Shouldn't really do anything.
    outPortal.Set(index + this->IndexOffset, outVec);
  }
};

template <typename OutArrayHandle>
void CopyPartition(const vtkm::cont::Field& inField, OutArrayHandle& outArray, vtkm::Id startIndex)
{
  vtkm::cont::Invoker invoke;
  using ComponentType = typename OutArrayHandle::ValueType::ComponentType;
  if (inField.GetData().IsBaseComponentType<ComponentType>())
  {
    invoke(CopyPartitionWorklet{ startIndex },
           inField.GetData().ExtractArrayFromComponents<ComponentType>(),
           outArray);
  }
  else
  {
    VTKM_LOG_S(vtkm::cont::LogLevel::Info,
               "Discovered mismatched types for field " << inField.GetName()
                                                        << ". Requires extra copy.");
    invoke(CopyPartitionWorklet{ startIndex },
           inField.GetDataAsDefaultFloat().ExtractArrayFromComponents<vtkm::FloatDefault>(),
           outArray);
  }
}

template <typename HasFieldFunctor, typename GetFieldFunctor>
vtkm::cont::UnknownArrayHandle MergeArray(vtkm::Id numPartitions,
                                          HasFieldFunctor&& hasField,
                                          GetFieldFunctor&& getField,
                                          vtkm::Id totalSize)
{
  vtkm::cont::UnknownArrayHandle mergedArray = getField(0).GetData().NewInstanceBasic();
  mergedArray.Allocate(totalSize);

  vtkm::Id startIndex = 0;
  for (vtkm::Id partitionId = 0; partitionId < numPartitions; ++partitionId)
  {
    vtkm::Id partitionSize;
    if (hasField(partitionId, partitionSize))
    {
      vtkm::cont::Field sourceField = getField(partitionId);
      mergedArray.CastAndCallWithExtractedArray(
        [=](auto array) { CopyPartition(sourceField, array, startIndex); });
    }
    else
    {
      mergedArray.CastAndCallWithExtractedArray(
        [=](auto array) { ClearPartition(array, startIndex, partitionSize); });
    }
    startIndex += partitionSize;
  }
  VTKM_ASSERT(startIndex == totalSize);

  return mergedArray;
}

vtkm::cont::CoordinateSystem MergeCoordinateSystem(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet,
  vtkm::IdComponent coordId,
  vtkm::Id numPointsTotal)
{
  std::string coordName = partitionedDataSet.GetPartition(0).GetCoordinateSystem(coordId).GetName();
  auto hasField = [&](vtkm::Id partitionId, vtkm::Id& partitionSize) -> bool {
    vtkm::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    partitionSize = partition.GetNumberOfPoints();
    // Should partitions match coordinates on name or coordinate id? They both should match, but
    // for now let's go by id and check the name.
    if (partition.GetNumberOfCoordinateSystems() <= coordId)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "When merging partitions, partition "
                   << partitionId << " is missing coordinate system with index " << coordId);
      return false;
    }
    if (partition.GetCoordinateSystem(coordId).GetName() != coordName)
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Warn,
                 "When merging partitions, partition "
                   << partitionId << " reported a coordinate system with name '"
                   << partition.GetCoordinateSystem(coordId).GetName()
                   << "' instead of expected name '" << coordName << "'");
    }
    return true;
  };
  auto getField = [&](vtkm::Id partitionId) -> vtkm::cont::Field {
    return partitionedDataSet.GetPartition(partitionId).GetCoordinateSystem(coordId);
  };
  vtkm::cont::UnknownArrayHandle mergedArray =
    MergeArray(partitionedDataSet.GetNumberOfPartitions(), hasField, getField, numPointsTotal);
  return vtkm::cont::CoordinateSystem{ coordName, mergedArray };
}

vtkm::cont::Field MergeField(const vtkm::cont::PartitionedDataSet& partitionedDataSet,
                             vtkm::IdComponent fieldId,
                             vtkm::Id numPointsTotal,
                             vtkm::Id numCellsTotal)
{
  vtkm::cont::Field referenceField = partitionedDataSet.GetPartition(0).GetField(fieldId);
  vtkm::Id totalSize = 0;
  switch (referenceField.GetAssociation())
  {
    case vtkm::cont::Field::Association::Points:
      totalSize = numPointsTotal;
      break;
    case vtkm::cont::Field::Association::Cells:
      totalSize = numCellsTotal;
      break;
    default:
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Skipping merge of field '" << referenceField.GetName()
                                             << "' because it has an unsupported association.");
      return referenceField;
  }

  auto hasField = [&](vtkm::Id partitionId, vtkm::Id& partitionSize) -> bool {
    vtkm::cont::DataSet partition = partitionedDataSet.GetPartition(partitionId);
    if (partition.HasField(referenceField.GetName(), referenceField.GetAssociation()))
    {
      partitionSize = partition.GetField(referenceField.GetName(), referenceField.GetAssociation())
                        .GetData()
                        .GetNumberOfValues();
      return true;
    }
    else
    {
      VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                 "Partition " << partitionId << " does not have field "
                              << referenceField.GetName());
      switch (referenceField.GetAssociation())
      {
        case vtkm::cont::Field::Association::Points:
          partitionSize = partition.GetNumberOfPoints();
          break;
        case vtkm::cont::Field::Association::Cells:
          partitionSize = partition.GetNumberOfCells();
          break;
        default:
          partitionSize = 0;
          break;
      }
      return false;
    }
  };
  auto getField = [&](vtkm::Id partitionId) -> vtkm::cont::Field {
    return partitionedDataSet.GetPartition(partitionId)
      .GetField(referenceField.GetName(), referenceField.GetAssociation());
  };
  vtkm::cont::UnknownArrayHandle mergedArray =
    MergeArray(partitionedDataSet.GetNumberOfPartitions(), hasField, getField, totalSize);
  return vtkm::cont::Field{ referenceField.GetName(),
                            referenceField.GetAssociation(),
                            mergedArray };
}

} // anonymous namespace

//-----------------------------------------------------------------------------

namespace vtkm
{
namespace cont
{

VTKM_CONT
vtkm::cont::DataSet MergePartitionedDataSet(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet)
{
  // verify correctnees of data
  VTKM_ASSERT(partitionedDataSet.GetNumberOfPartitions() > 0);

  vtkm::Id numPointsTotal;
  vtkm::Id numCellsTotal;
  CountPointsAndCells(partitionedDataSet, numPointsTotal, numCellsTotal);

  vtkm::cont::DataSet outputData;
  outputData.SetCellSet(MergeCellSets(partitionedDataSet, numPointsTotal, numCellsTotal));

  vtkm::cont::DataSet partition0 = partitionedDataSet.GetPartition(0);
  for (vtkm::IdComponent coordId = 0; coordId < partition0.GetNumberOfCoordinateSystems();
       ++coordId)
  {
    outputData.AddCoordinateSystem(
      MergeCoordinateSystem(partitionedDataSet, coordId, numPointsTotal));
  }

  for (vtkm::IdComponent fieldId = 0; fieldId < partition0.GetNumberOfFields(); ++fieldId)
  {
    outputData.AddField(MergeField(partitionedDataSet, fieldId, numPointsTotal, numCellsTotal));
  }

  return outputData;
}

}
}
