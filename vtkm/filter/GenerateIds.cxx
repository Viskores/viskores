//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/GenerateIds.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleIndex.h>

namespace
{

vtkm::cont::UnknownArrayHandle GenerateArray(const vtkm::filter::GenerateIds& self, vtkm::Id size)
{
  vtkm::cont::ArrayHandleIndex indexArray(size);

  if (self.GetUseFloat())
  {
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> output;
    vtkm::cont::ArrayCopy(indexArray, output);
    return output;
  }
  else
  {
    vtkm::cont::ArrayHandle<vtkm::Id> output;
    vtkm::cont::ArrayCopy(indexArray, output);
    return output;
  }
}

} // anonymous namespace

namespace vtkm
{
namespace filter
{

vtkm::cont::DataSet GenerateIds::DoExecute(const vtkm::cont::DataSet& input) const
{
  vtkm::cont::DataSet output = input;

  if (this->GetGeneratePointIds())
  {
    output.AddPointField(this->GetPointFieldName(),
                         GenerateArray(*this, input.GetNumberOfPoints()));
  }

  if (this->GetGenerateCellIds())
  {
    output.AddCellField(this->GetCellFieldName(), GenerateArray(*this, input.GetNumberOfCells()));
  }

  return output;
}

} // namespace vtkm::filter
} // namespace vtkm
