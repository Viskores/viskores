//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ArrayCopyDevice.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/filter/contour/Slice.h>
#include <vtkm/filter/contour/SliceMultiple.h>
#include <vtkm/filter/multi_block/AmrArrays.h>
#include <vtkm/worklet/WorkletMapField.h>
namespace vtkm
{
namespace filter
{
namespace contour
{
class OffsetWorklet : public vtkm::worklet::WorkletMapField
{
protected:
  vtkm::Id OffsetValue;

public:
  VTKM_CONT
  OffsetWorklet(const vtkm::Id offset)
    : OffsetValue(offset)
  {
  }
  typedef void ControlSignature(FieldInOut);
  typedef void ExecutionSignature(_1);
  VTKM_EXEC void operator()(vtkm::Id& value) const { value += this->OffsetValue; }
};
// Original MergeContours code come from
// https://github.com/Alpine-DAV/ascent/blob/develop/src/libs/vtkh/filters/Slice.cpp#L517
class MergeContours
{
  vtkm::cont::PartitionedDataSet DataSets;

public:
  MergeContours(vtkm::cont::PartitionedDataSet& dataSets)
    : DataSets(dataSets)
  {
  }
  vtkm::cont::DataSet MergeDataSets()
  {
    vtkm::cont::DataSet res;
    vtkm::Id numOfDataSet = this->DataSets.GetNumberOfPartitions();
    vtkm::Id numCells = 0;
    vtkm::Id numPoints = 0;
    std::vector<vtkm::Id> cellOffsets(numOfDataSet);
    std::vector<vtkm::Id> pointOffsets(numOfDataSet);
    for (vtkm::Id i = 0; i < numOfDataSet; i++)
    {
      auto cellSet = this->DataSets.GetPartition(i).GetCellSet();
      //We assume all cells are triangles here
      if (!cellSet.IsType<vtkm::cont::CellSetSingleType<>>())
      {
        throw vtkm::cont::ErrorFilterExecution(
          "Expected singletype cell set as the result of contour.");
      }
      cellOffsets[i] = numCells;
      numCells += cellSet.GetNumberOfCells();
      pointOffsets[i] = numPoints;
      numPoints += this->DataSets.GetPartition(i).GetNumberOfPoints();
    }
    const vtkm::Id connSize = numCells * 3;
    // Calculating merged offsets for all domains
    vtkm::cont::ArrayHandle<vtkm::Id> conn;
    conn.Allocate(connSize);
    for (vtkm::Id i = 0; i < numOfDataSet; i++)
    {
      auto cellSet = this->DataSets.GetPartition(i).GetCellSet();
      if (!cellSet.IsType<vtkm::cont::CellSetSingleType<>>())
      {
        throw vtkm::cont::ErrorFilterExecution(
          "Expected singletype cell set as the result of contour.");
      }
      // Grabing the connectivity and copy it into the larger array
      vtkm::cont::CellSetSingleType<> singleType =
        cellSet.AsCellSet<vtkm::cont::CellSetSingleType<>>();
      const vtkm::cont::ArrayHandle<vtkm::Id> connPerDataSet = singleType.GetConnectivityArray(
        vtkm::TopologyElementTagCell(), vtkm::TopologyElementTagPoint());
      vtkm::Id copySize = connPerDataSet.GetNumberOfValues();
      vtkm::Id start = 0;
      vtkm::cont::Algorithm::CopySubRange(
        connPerDataSet, start, copySize, conn, cellOffsets[i] * 3);
      // We offset the connectiviy we just copied in so we references the
      // correct points
      if (cellOffsets[i] != 0)
      {
        vtkm::cont::Invoker invoker;
        invoker(OffsetWorklet{ pointOffsets[i] },
                vtkm::cont::make_ArrayHandleView(conn, cellOffsets[i] * 3, copySize));
      }
    }
    vtkm::cont::CellSetSingleType<> cellSet;
    cellSet.Fill(numPoints, vtkm::CELL_SHAPE_TRIANGLE, 3, conn);
    res.SetCellSet(cellSet);
    // Merging selected fields and coordinates
    vtkm::IdComponent numFields = this->DataSets.GetPartition(0).GetNumberOfFields();
    for (vtkm::IdComponent i = 0; i < numFields; i++)
    {
      const vtkm::cont::Field& field = this->DataSets.GetPartition(0).GetField(i);
      bool isSupported = (field.GetAssociation() == vtkm::cont::Field::Association::Points ||
                          field.GetAssociation() == vtkm::cont::Field::Association::Cells);
      if (!isSupported)
      {
        continue;
      }
      vtkm::cont::UnknownArrayHandle outFieldArray = field.GetData().NewInstanceBasic();
      bool assocPoints = field.GetAssociation() == vtkm::cont::Field::Association::Points;
      if (assocPoints)
      {
        outFieldArray.Allocate(numPoints);
      }
      else
      {
        outFieldArray.Allocate(numCells);
      }
      auto resolveType = [&](auto& concreteOut) {
        vtkm::Id offset = 0;
        for (vtkm::Id partitionId = 0; partitionId < this->DataSets.GetNumberOfPartitions();
             ++partitionId)
        {
          vtkm::cont::UnknownArrayHandle in = this->DataSets.GetPartition(partitionId)
                                                .GetField(field.GetName(), field.GetAssociation())
                                                .GetData();
          vtkm::Id copySize = in.GetNumberOfValues();
          auto viewOut = vtkm::cont::make_ArrayHandleView(concreteOut, offset, copySize);
          vtkm::cont::ArrayCopy(in, viewOut);
          offset += copySize;
        }
      };
      outFieldArray.CastAndCallWithExtractedArray(resolveType);
      res.AddField(vtkm::cont::Field(field.GetName(), field.GetAssociation(), outFieldArray));
    }
    //Labeling fields that belong to the coordinate system.
    //There might be multiple coordinates systems, assuming all partitions have the same name of the coordinates system
    vtkm::IdComponent numCoordsNames =
      this->DataSets.GetPartition(0).GetNumberOfCoordinateSystems();
    for (vtkm::IdComponent i = 0; i < numCoordsNames; i++)
    {
      res.AddCoordinateSystem(this->DataSets.GetPartition(0).GetCoordinateSystemName(i));
    }
    return res;
  }
};
vtkm::cont::DataSet SliceMultiple::DoExecute(const vtkm::cont::DataSet& input)
{
  vtkm::cont::PartitionedDataSet slices;
  //Executing Slice filter several times and merge results together
  for (vtkm::IdComponent i = 0; i < static_cast<vtkm::IdComponent>(this->FunctionList.size()); i++)
  {
    vtkm::filter::contour::Slice slice;
    slice.SetImplicitFunction(this->GetImplicitFunction(i));
    slice.SetFieldsToPass(this->GetFieldsToPass());
    auto result = slice.Execute(input);
    slices.AppendPartition(result);
  }
  if (slices.GetNumberOfPartitions() > 1)
  {
    //Since the slice filter have already selected fields
    //the mergeCountours will copy all existing fields
    MergeContours merger(slices);
    vtkm::cont::DataSet mergedResults = merger.MergeDataSets();
    return mergedResults;
  }
  return slices.GetPartition(0);
}
} // namespace contour
} // namespace filter
} // namespace vtkm
