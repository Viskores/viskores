//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_ImageMedian_hxx
#define vtk_m_filter_ImageMedian_hxx

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

VTKM_CONT ImageMedian::ImageMedian()
{
  this->SetOutputFieldName("median");
}

template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ImageMedian::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>& policy)
{
  if (!fieldMetadata.IsPointField())
  {
    throw vtkm::cont::ErrorBadValue("Active field for ImageMedian must be a point field.");
  }

  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();
  vtkm::cont::ArrayHandle<T> result;
  if (this->Neighborhood == 1 || this->Neighborhood == 2)
  {
    this->Invoke(worklet::ImageMedian{ this->Neighborhood },
                 vtkm::filter::ApplyPolicyCellSetStructured(cells, policy),
                 field,
                 result);
  }
  else
  {
    throw vtkm::cont::ErrorBadValue("ImageMedian only support a 3x3 or 5x5 stencil.");
  }

  std::string name = this->GetOutputFieldName();
  if (name.empty())
  {
    name = fieldMetadata.GetName();
  }

  return CreateResult(input, fieldMetadata.AsField(name, result));
}
}
}

#endif
