//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleGroupVecVariable.h>
#include <vtkm/cont/ArrayHandleView.h>
#include <vtkm/cont/ConvertNumComponentsToOffsets.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Logging.h>
#include <vtkm/cont/MergePartitionedDataSet.h>
#include <vtkm/cont/PartitionedDataSet.h>
#include <vtkm/cont/UnknownCellSet.h>
#include <vtkm/cont/internal/CastInvalidValue.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/WorkletMapTopology.h>


namespace
{

struct CopyWithOffsetWorklet : public vtkm::worklet::WorkletMapField
{
  vtkm::Id OffsetValue;
  VTKM_CONT
  CopyWithOffsetWorklet(const vtkm::Id offset)
    : OffsetValue(offset)
  {
  }
  typedef void ControlSignature(FieldIn, FieldInOut);
  typedef void ExecutionSignature(_1, _2);
  VTKM_EXEC void operator()(const vtkm::Id originalValue, vtkm::Id& outputValue) const
  {
    outputValue = originalValue + this->OffsetValue;
  }
};

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
                 vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
                 const vtkm::Id firstNonEmptyPartitionId)
{
  vtkm::cont::Invoker invoke;

  shapes.Allocate(numCellsTotal);
  numIndices.Allocate(numCellsTotal);

  vtkm::Id cellStartIndex = 0;
  for (vtkm::Id partitionId = firstNonEmptyPartitionId;
       partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    if (partitionedDataSet.GetPartition(partitionId).GetNumberOfPoints() == 0)
    {
      continue;
    }
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
                  vtkm::cont::ArrayHandle<vtkm::Id>& indices,
                  const vtkm::Id firstNonEmptyPartitionId)
{
  vtkm::cont::Invoker invoke;

  indices.Allocate(numIndicesTotal);

  vtkm::Id pointStartIndex = 0;
  vtkm::Id cellStartIndex = 0;
  for (vtkm::Id partitionId = firstNonEmptyPartitionId;
       partitionId < partitionedDataSet.GetNumberOfPartitions();
       ++partitionId)
  {
    if (partitionedDataSet.GetPartition(partitionId).GetNumberOfPoints() == 0)
    {
      continue;
    }
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

vtkm::cont::CellSetSingleType<> MergeCellSetsSingleType(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet,
  const vtkm::Id firstNonEmptyPartitionId)
{
  vtkm::Id numCells = 0;
  vtkm::Id numPoints = 0;
  vtkm::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  std::vector<vtkm::Id> cellOffsets(numOfDataSet);
  std::vector<vtkm::Id> pointOffsets(numOfDataSet);
  //Mering cell set into single type
  //checking the cell type to make sure how many points per cell
  vtkm::IdComponent numberOfPointsPerCell =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId)
      .GetCellSet()
      .GetNumberOfPointsInCell(0);
  for (vtkm::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    cellOffsets[partitionIndex] = numCells;
    numCells += partitionedDataSet.GetPartition(partitionIndex).GetNumberOfCells();
    pointOffsets[partitionIndex] = numPoints;
    numPoints += partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints();
  }
  //We assume all cells have same type, which should have been previously checked.
  const vtkm::Id mergedConnSize = numCells * numberOfPointsPerCell;
  // Calculating merged offsets for all domains
  vtkm::cont::ArrayHandle<vtkm::Id> mergedConn;
  mergedConn.Allocate(mergedConnSize);
  for (vtkm::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    auto cellSet = partitionedDataSet.GetPartition(partitionIndex).GetCellSet();
    // Grabing the connectivity and copy it into the larger connectivity array
    vtkm::cont::CellSetSingleType<> singleType =
      cellSet.AsCellSet<vtkm::cont::CellSetSingleType<>>();
    const vtkm::cont::ArrayHandle<vtkm::Id> connPerDataSet = singleType.GetConnectivityArray(
      vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
    vtkm::Id copySize = connPerDataSet.GetNumberOfValues();
    VTKM_ASSERT(copySize == cellSet.GetNumberOfCells() * numberOfPointsPerCell);
    // Mapping connectivity array per data into proper region of merged connectivity array
    // and also adjust the value in merged connectivity array
    vtkm::cont::Invoker invoker;
    invoker(CopyWithOffsetWorklet{ pointOffsets[partitionIndex] },
            connPerDataSet,
            vtkm::cont::make_ArrayHandleView(
              mergedConn, cellOffsets[partitionIndex] * numberOfPointsPerCell, copySize));
  }
  vtkm::cont::CellSetSingleType<> cellSet;
  //Filling in the cellSet according to shapeId and number of points per cell.
  vtkm::UInt8 cellShapeId =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetCellSet().GetCellShape(0);
  cellSet.Fill(numPoints, cellShapeId, numberOfPointsPerCell, mergedConn);
  return cellSet;
}

vtkm::cont::CellSetExplicit<> MergeCellSetsExplicit(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet,
  vtkm::Id numPointsTotal,
  vtkm::Id numCellsTotal,
  const vtkm::Id firstNonEmptyPartitionId)
{
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  MergeShapes(partitionedDataSet, numCellsTotal, shapes, numIndices, firstNonEmptyPartitionId);

  vtkm::cont::ArrayHandle<vtkm::Id> offsets;
  vtkm::Id numIndicesTotal;
  vtkm::cont::ConvertNumComponentsToOffsets(numIndices, offsets, numIndicesTotal);
  numIndices.ReleaseResources();

  //Merging connectivity/indicies array
  vtkm::cont::ArrayHandle<vtkm::Id> indices;
  MergeIndices(partitionedDataSet, offsets, numIndicesTotal, indices, firstNonEmptyPartitionId);

  vtkm::cont::CellSetExplicit<> outCells;
  outCells.Fill(numPointsTotal, shapes, indices, offsets);
  return outCells;
}

vtkm::Id GetFirstEmptyPartition(const vtkm::cont::PartitionedDataSet& partitionedDataSet)
{
  vtkm::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  vtkm::Id firstNonEmptyPartitionId = -1;
  for (vtkm::Id partitionIndex = 0; partitionIndex < numOfDataSet; partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() != 0)
    {
      firstNonEmptyPartitionId = partitionIndex;
      break;
    }
  }
  return firstNonEmptyPartitionId;
}

bool PartitionsAreSingleType(const vtkm::cont::PartitionedDataSet partitionedDataSet,
                             const vtkm::Id firstNonEmptyPartitionId)
{
  vtkm::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  for (vtkm::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    auto cellSet = partitionedDataSet.GetPartition(partitionIndex).GetCellSet();
    if (!cellSet.IsType<vtkm::cont::CellSetSingleType<>>())
    {
      return false;
    }
  }

  //Make sure the cell type of each non-empty partition is same
  //with tested one, and they also have the same number of points.
  //We know that all cell sets are of type `CellSetSingleType<>` at this point.
  //Polygons may have different number of points even with the same shape id.
  //It is more efficient to `GetCellShape(0)` from `CellSetSingleType` compared with `CellSetExplicit`.
  auto cellSetFirst = partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetCellSet();
  vtkm::UInt8 cellShapeId = cellSetFirst.GetCellShape(0);
  vtkm::IdComponent numPointsInCell = cellSetFirst.GetNumberOfPointsInCell(0);
  for (vtkm::Id partitionIndex = firstNonEmptyPartitionId + 1; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    auto cellSet = partitionedDataSet.GetPartition(partitionIndex).GetCellSet();
    if (cellSet.GetCellShape(0) != cellShapeId ||
        cellSet.GetNumberOfPointsInCell(0) != numPointsInCell)
    {
      return false;
    }
  }
  return true;
}

void CheckCoordsNames(const vtkm::cont::PartitionedDataSet partitionedDataSet,
                      const vtkm::Id firstNonEmptyPartitionId)
{
  vtkm::IdComponent numCoords =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetNumberOfCoordinateSystems();
  std::vector<std::string> coordsNames;
  for (vtkm::IdComponent coordsIndex = 0; coordsIndex < numCoords; coordsIndex++)
  {
    coordsNames.push_back(partitionedDataSet.GetPartition(firstNonEmptyPartitionId)
                            .GetCoordinateSystemName(coordsIndex));
  }
  vtkm::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  for (vtkm::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      //Skip the empty data sets in the partitioned data sets
      continue;
    }
    if (numCoords != partitionedDataSet.GetPartition(partitionIndex).GetNumberOfCoordinateSystems())
    {
      throw vtkm::cont::ErrorExecution("Data sets have different number of coordinate systems");
    }
    for (vtkm::IdComponent coordsIndex = 0; coordsIndex < numCoords; coordsIndex++)
    {
      if (!partitionedDataSet.GetPartition(partitionIndex)
             .HasCoordinateSystem(coordsNames[coordsIndex]))
      {
        throw vtkm::cont::ErrorExecution(
          "Coordinates system name: " + coordsNames[coordsIndex] +
          " in the first partition does not exist in other partitions");
      }
    }
  }
}


void MergeFieldsAndAddIntoDataSet(vtkm::cont::DataSet& outputDataSet,
                                  const vtkm::cont::PartitionedDataSet partitionedDataSet,
                                  const vtkm::Id numPoints,
                                  const vtkm::Id numCells,
                                  const vtkm::Float64 invalidValue,
                                  const vtkm::Id firstNonEmptyPartitionId)
{
  // Merging selected fields and coordinates
  // We get fields names in all partitions firstly
  // The inserted map stores the field name and a index of the first partition that owns that field
  vtkm::cont::Invoker invoke;
  auto associationHash = [](vtkm::cont::Field::Association const& association) {
    return static_cast<std::size_t>(association);
  };
  std::unordered_map<vtkm::cont::Field::Association,
                     std::unordered_map<std::string, vtkm::Id>,
                     decltype(associationHash)>
    fieldsMap(2, associationHash);

  vtkm::Id numOfDataSet = partitionedDataSet.GetNumberOfPartitions();
  for (vtkm::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
       partitionIndex++)
  {
    if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
    {
      continue;
    }
    vtkm::IdComponent numFields =
      partitionedDataSet.GetPartition(partitionIndex).GetNumberOfFields();
    for (vtkm::IdComponent fieldIndex = 0; fieldIndex < numFields; fieldIndex++)
    {
      const vtkm::cont::Field& field =
        partitionedDataSet.GetPartition(partitionIndex).GetField(fieldIndex);
      auto association = field.GetAssociation();
      bool isSupported = (association == vtkm::cont::Field::Association::Points ||
                          association == vtkm::cont::Field::Association::Cells);
      if (!isSupported)
      {
        VTKM_LOG_S(vtkm::cont::LogLevel::Info,
                   "Skipping merge of field '" << field.GetName()
                                               << "' because it has an unsupported association.");
      }
      //Do not store the field index again if it exists in fieldMap
      if (fieldsMap[association].find(field.GetName()) != fieldsMap[association].end())
      {
        continue;
      }
      fieldsMap[association][field.GetName()] = partitionIndex;
    }
  }
  // Iterate all fields and create merged field arrays
  for (auto fieldMapIter = fieldsMap.begin(); fieldMapIter != fieldsMap.end(); ++fieldMapIter)
  {
    auto fieldAssociation = fieldMapIter->first;
    auto fieldNamesMap = fieldMapIter->second;
    for (auto fieldNameIter = fieldNamesMap.begin(); fieldNameIter != fieldNamesMap.end();
         ++fieldNameIter)
    {
      std::string fieldName = fieldNameIter->first;
      vtkm::Id partitionOwnsField = fieldNameIter->second;
      const vtkm::cont::Field& field =
        partitionedDataSet.GetPartition(partitionOwnsField).GetField(fieldName, fieldAssociation);

      vtkm::cont::UnknownArrayHandle mergedFieldArray = field.GetData().NewInstanceBasic();
      if (fieldAssociation == vtkm::cont::Field::Association::Points)
      {
        mergedFieldArray.Allocate(numPoints);
      }
      else
      {
        //We may add a new association (such as edges or faces) in future
        VTKM_ASSERT(fieldAssociation == vtkm::cont::Field::Association::Cells);
        mergedFieldArray.Allocate(numCells);
      }
      //Merging each field into the mergedField array
      auto resolveType = [&](auto& concreteOut) {
        vtkm::Id offset = 0;
        for (vtkm::Id partitionIndex = firstNonEmptyPartitionId; partitionIndex < numOfDataSet;
             ++partitionIndex)
        {
          if (partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints() == 0)
          {
            continue;
          }
          if (partitionedDataSet.GetPartition(partitionIndex).HasField(fieldName, fieldAssociation))
          {
            vtkm::cont::UnknownArrayHandle in = partitionedDataSet.GetPartition(partitionIndex)
                                                  .GetField(fieldName, fieldAssociation)
                                                  .GetData();
            vtkm::Id copySize = in.GetNumberOfValues();
            auto viewOut = vtkm::cont::make_ArrayHandleView(concreteOut, offset, copySize);
            vtkm::cont::ArrayCopy(in, viewOut);
            offset += copySize;
          }
          else
          {
            //Creating invalid values for the partition that does not have the field
            using ComponentType =
              typename std::decay_t<decltype(concreteOut)>::ValueType::ComponentType;
            ComponentType castInvalid =
              vtkm::cont::internal::CastInvalidValue<ComponentType>(invalidValue);
            vtkm::Id copySize = 0;
            if (fieldAssociation == vtkm::cont::Field::Association::Points)
            {
              copySize = partitionedDataSet.GetPartition(partitionIndex).GetNumberOfPoints();
            }
            else
            {
              copySize = partitionedDataSet.GetPartition(partitionIndex).GetNumberOfCells();
            }
            for (vtkm::IdComponent component = 0; component < concreteOut.GetNumberOfComponents();
                 ++component)
            {
              //Extracting each component from RecombineVec and copy invalid value into it
              //Avoid using invoke to call worklet on ArrayHandleRecombineVec (it may cause long compiling issue on CUDA 12.x).
              concreteOut.GetComponentArray(component).Fill(castInvalid, offset, offset + copySize);
            }
            offset += copySize;
          }
        }
      };
      mergedFieldArray.CastAndCallWithExtractedArray(resolveType);
      outputDataSet.AddField(vtkm::cont::Field(fieldName, fieldAssociation, mergedFieldArray));
    }
  }
  return;
}

} // anonymous namespace

//-----------------------------------------------------------------------------

namespace vtkm
{
namespace cont
{

VTKM_CONT
vtkm::cont::DataSet MergePartitionedDataSet(
  const vtkm::cont::PartitionedDataSet& partitionedDataSet,
  vtkm::Float64 invalidValue)
{
  vtkm::cont::DataSet outputData;
  //The name of coordinates system in the first non-empty partition will be used in merged data set
  vtkm::Id firstNonEmptyPartitionId = GetFirstEmptyPartition(partitionedDataSet);
  if (firstNonEmptyPartitionId == -1)
  {
    return outputData;
  }

  //Checking the name of coordinates system, if all partitions have different name with the firstNonEmptyPartitionId
  //just throw the exception now
  CheckCoordsNames(partitionedDataSet, firstNonEmptyPartitionId);

  //Checking if all partitions have CellSetSingleType with the same cell type
  bool allPartitionsAreSingleType =
    PartitionsAreSingleType(partitionedDataSet, firstNonEmptyPartitionId);

  vtkm::Id numPointsTotal;
  vtkm::Id numCellsTotal;
  CountPointsAndCells(partitionedDataSet, numPointsTotal, numCellsTotal);

  if (allPartitionsAreSingleType)
  {
    outputData.SetCellSet(MergeCellSetsSingleType(partitionedDataSet, firstNonEmptyPartitionId));
  }
  else
  {
    outputData.SetCellSet(MergeCellSetsExplicit(
      partitionedDataSet, numPointsTotal, numCellsTotal, firstNonEmptyPartitionId));
  }
  //Merging fields and coordinate systems
  MergeFieldsAndAddIntoDataSet(outputData,
                               partitionedDataSet,
                               numPointsTotal,
                               numCellsTotal,
                               invalidValue,
                               firstNonEmptyPartitionId);
  //Labeling fields that belong to the coordinate system.
  //There might be multiple coordinates systems, assuming all partitions have the same name of the coordinates system
  vtkm::IdComponent numCoordsNames =
    partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetNumberOfCoordinateSystems();
  for (vtkm::IdComponent i = 0; i < numCoordsNames; i++)
  {
    outputData.AddCoordinateSystem(
      partitionedDataSet.GetPartition(firstNonEmptyPartitionId).GetCoordinateSystemName(i));
  }
  return outputData;
}

}
}
