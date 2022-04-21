//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/image_processing/ImageMedian.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

// NOTE BIEN!!! This line has to come last!!! Otherwise CUDA complains!!! NEIB ETON
#include <vtkm/Swap.h>

namespace vtkm
{
namespace worklet
{
// An implementation of the quickselect/Hoare's selection algorithm to find medians
// inplace, generally fairly fast for reasonable sized data.
//
template <typename T>
VTKM_EXEC inline T find_median(T* values, std::size_t mid, std::size_t size)
{
  std::size_t begin = 0;
  std::size_t end = size - 1;
  while (begin < end)
  {
    T x = values[mid];
    std::size_t i = begin;
    std::size_t j = end;
    do
    {
      for (; values[i] < x; i++)
      {
      }
      for (; x < values[j]; j--)
      {
      }
      if (i <= j)
      {
        vtkm::Swap(values[i], values[j]);
        i++;
        j--;
      }
    } while (i <= j);

    begin = (j < mid) ? i : begin;
    end = (mid < i) ? j : end;
  }
  return values[mid];
}
struct ImageMedian : public vtkm::worklet::WorkletPointNeighborhood
{
  int Neighborhood;
  ImageMedian(int neighborhoodSize)
    : Neighborhood(neighborhoodSize)
  {
  }
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, _3);

  template <typename InNeighborhoodT, typename T>
  VTKM_EXEC void operator()(const InNeighborhoodT& input, T& out) const
  {
    vtkm::Vec<T, 25> values;
    int index = 0;
    for (int x = -this->Neighborhood; x <= this->Neighborhood; ++x)
    {
      for (int y = -this->Neighborhood; y <= this->Neighborhood; ++y)
      {
        values[index++] = input.Get(x, y, 0);
      }
    }

    std::size_t len =
      static_cast<std::size_t>((this->Neighborhood * 2 + 1) * (this->Neighborhood * 2 + 1));
    std::size_t mid = len / 2;
    out = find_median(&values[0], mid, len);
  }
};
}

namespace filter
{
namespace image_processing
{
VTKM_CONT vtkm::cont::DataSet ImageMedian::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);
  if (!field.IsFieldPoint())
  {
    throw vtkm::cont::ErrorBadValue("Active field for ImageMedian must be a point field.");
  }

  const vtkm::cont::UnknownCellSet& inputCellSet = input.GetCellSet();
  vtkm::cont::UnknownArrayHandle outArray;

  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    vtkm::cont::ArrayHandle<T> result;

    VTKM_ASSERT((this->Neighborhood == 1) || (this->Neighborhood == 2));
    this->Invoke(worklet::ImageMedian{ this->Neighborhood }, inputCellSet, concrete, result);

    outArray = result;
  };
  this->CastAndCallScalarField(field, resolveType);

  std::string name = this->GetOutputFieldName();
  if (name.empty())
  {
    name = field.GetName();
  }
  return this->CreateResultFieldPoint(input, name, outArray);
}
} // namespace image_processing
} // namespace filter
} // namespace vtkm
