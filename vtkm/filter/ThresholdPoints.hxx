//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_filter_ThresholdPoints_hxx
#define vtk_m_filter_ThresholdPoints_hxx

#include <vtkm/cont/ErrorFilterExecution.h>

namespace
{
// Predicate for values less than minimum
class ValuesBelow
{
public:
  VTKM_CONT
  ValuesBelow(const vtkm::Float64& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  VTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return value <= static_cast<ScalarType>(this->Value);
  }

private:
  vtkm::Float64 Value;
};

// Predicate for values greater than maximum
class ValuesAbove
{
public:
  VTKM_CONT
  ValuesAbove(const vtkm::Float64& value)
    : Value(value)
  {
  }

  template <typename ScalarType>
  VTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return value >= static_cast<ScalarType>(this->Value);
  }

private:
  vtkm::Float64 Value;
};

// Predicate for values between minimum and maximum

class ValuesBetween
{
public:
  VTKM_CONT
  ValuesBetween(const vtkm::Float64& lower, const vtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename ScalarType>
  VTKM_EXEC bool operator()(const ScalarType& value) const
  {
    return value >= static_cast<ScalarType>(this->Lower) &&
      value <= static_cast<ScalarType>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};
}

namespace vtkm
{
namespace filter
{

const int THRESHOLD_BELOW = 0;
const int THRESHOLD_ABOVE = 1;
const int THRESHOLD_BETWEEN = 2;

//-----------------------------------------------------------------------------
inline VTKM_CONT ThresholdPoints::ThresholdPoints()
  : vtkm::filter::FilterDataSetWithField<ThresholdPoints>()
  , LowerValue(0)
  , UpperValue(0)
  , ThresholdType(THRESHOLD_BETWEEN)
  , CompactPoints(false)
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void ThresholdPoints::SetThresholdBelow(const vtkm::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_BELOW;
}

inline VTKM_CONT void ThresholdPoints::SetThresholdAbove(const vtkm::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_ABOVE;
}

inline VTKM_CONT void ThresholdPoints::SetThresholdBetween(const vtkm::Float64 value1,
                                                           const vtkm::Float64 value2)
{
  this->SetLowerThreshold(value1);
  this->SetUpperThreshold(value2);
  this->ThresholdType = THRESHOLD_BETWEEN;
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet ThresholdPoints::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& field,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // extract the input cell set
  const vtkm::cont::DynamicCellSet& cells = input.GetCellSet();

  // field to threshold on must be a point field
  if (fieldMeta.IsPointField() == false)
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  // run the worklet on the cell set and input field
  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::ThresholdPoints worklet;

  switch (this->ThresholdType)
  {
    case THRESHOLD_BELOW:
    {
      outCellSet = worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy),
                               field,
                               ValuesBelow(this->GetLowerThreshold()));
      break;
    }
    case THRESHOLD_ABOVE:
    {
      outCellSet = worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy),
                               field,
                               ValuesAbove(this->GetUpperThreshold()));
      break;
    }
    case THRESHOLD_BETWEEN:
    default:
    {
      outCellSet = worklet.Run(vtkm::filter::ApplyPolicyCellSet(cells, policy),
                               field,
                               ValuesBetween(this->GetLowerThreshold(), this->GetUpperThreshold()));
      break;
    }
  }

  // create the output dataset
  vtkm::cont::DataSet output;
  output.SetCellSet(outCellSet);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    this->Compactor.SetCompactPointFields(true);
    this->Compactor.SetMergePoints(true);
    return this->Compactor.Execute(output, PolicyDefault{});
  }
  else
  {
    return output;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT bool ThresholdPoints::DoMapField(
  vtkm::cont::DataSet& result,
  const vtkm::cont::ArrayHandle<T, StorageType>& input,
  const vtkm::filter::FieldMetadata& fieldMeta,
  vtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  // point data is copied as is because it was not collapsed
  if (fieldMeta.IsPointField())
  {
    if (this->CompactPoints)
    {
      return this->Compactor.DoMapField(result, input, fieldMeta, policy);
    }
    else
    {
      result.AddField(fieldMeta.AsField(input));
      return true;
    }
  }

  // cell data does not apply
  return false;
}
}
}
#endif
