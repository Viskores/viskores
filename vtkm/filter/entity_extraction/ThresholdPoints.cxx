//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/clean_grid/CleanGrid.h>
#include <vtkm/filter/entity_extraction/ThresholdPoints.h>
#include <vtkm/filter/entity_extraction/worklet/ThresholdPoints.h>

namespace
{
// Predicate for values less than minimum
class ValuesBelow
{
public:
  VTKM_CONT
  explicit ValuesBelow(const vtkm::Float64& value)
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
  explicit ValuesAbove(const vtkm::Float64& value)
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

bool DoMapField(vtkm::cont::DataSet& result, const vtkm::cont::Field& field)
{
  // point data is copied as is because it was not collapsed
  if (field.IsFieldPoint())
  {
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldGlobal())
  {
    result.AddField(field);
    return true;
  }
  else
  {
    // cell data does not apply
    return false;
  }
}

} // anonymous namespace

namespace vtkm
{
namespace filter
{

namespace entity_extraction
{
//-----------------------------------------------------------------------------
VTKM_CONT void ThresholdPoints::SetThresholdBelow(const vtkm::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_BELOW;
}

VTKM_CONT void ThresholdPoints::SetThresholdAbove(const vtkm::Float64 value)
{
  this->SetLowerThreshold(value);
  this->SetUpperThreshold(value);
  this->ThresholdType = THRESHOLD_ABOVE;
}

VTKM_CONT void ThresholdPoints::SetThresholdBetween(const vtkm::Float64 value1,
                                                    const vtkm::Float64 value2)
{
  this->SetLowerThreshold(value1);
  this->SetUpperThreshold(value2);
  this->ThresholdType = THRESHOLD_BETWEEN;
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet ThresholdPoints::DoExecute(const vtkm::cont::DataSet& input)
{
  // extract the input cell set
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const auto& field = this->GetFieldFromDataSet(input);

  // field to threshold on must be a point field
  if (!field.IsFieldPoint())
  {
    throw vtkm::cont::ErrorFilterExecution("Point field expected.");
  }

  // run the worklet on the cell set and input field
  vtkm::cont::CellSetSingleType<> outCellSet;
  vtkm::worklet::ThresholdPoints worklet;

  auto resolveType = [&](const auto& concrete) {
    switch (this->ThresholdType)
    {
      case THRESHOLD_BELOW:
      {
        outCellSet = worklet.Run(cells, concrete, ValuesBelow(this->GetLowerThreshold()));
        break;
      }
      case THRESHOLD_ABOVE:
      {
        outCellSet = worklet.Run(cells, concrete, ValuesAbove(this->GetUpperThreshold()));
        break;
      }
      case THRESHOLD_BETWEEN:
      default:
      {
        outCellSet = worklet.Run(
          cells, concrete, ValuesBetween(this->GetLowerThreshold(), this->GetUpperThreshold()));
        break;
      }
    }
  };

  this->CastAndCallScalarField(field, resolveType);

  // create the output dataset
  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f); };
  vtkm::cont::DataSet output =
    this->CreateResult(input, outCellSet, input.GetCoordinateSystems(), mapper);

  // compact the unused points in the output dataset
  if (this->CompactPoints)
  {
    vtkm::filter::clean_grid::CleanGrid compactor;
    compactor.SetCompactPointFields(true);
    compactor.SetMergePoints(true);
    return compactor.Execute(output);
  }
  else
  {
    return output;
  }
}

} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
