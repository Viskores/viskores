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
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/cont/MergePartitionedDataSet.h>
#include <vtkm/filter/contour/Slice.h>
#include <vtkm/filter/contour/SliceMultiple.h>
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
    vtkm::cont::DataSet mergedResults =
      vtkm::cont::MergePartitionedDataSet(slices, vtkm::Float64(0));
    return mergedResults;
  }
  return slices.GetPartition(0);
}
} // namespace contour
} // namespace filter
} // namespace vtkm
