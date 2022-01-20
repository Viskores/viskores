//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/MapFieldPermutation.h>
#include <vtkm/filter/entity_extraction/Threshold.h>
#include <vtkm/filter/entity_extraction/worklet/Threshold.h>

namespace vtkm
{
namespace filter
{
namespace entity_extraction
{
namespace
{

class ThresholdRange
{
public:
  VTKM_CONT
  ThresholdRange(const vtkm::Float64& lower, const vtkm::Float64& upper)
    : Lower(lower)
    , Upper(upper)
  {
  }

  template <typename T>
  VTKM_EXEC bool operator()(const T& value) const
  {

    return value >= static_cast<T>(this->Lower) && value <= static_cast<T>(this->Upper);
  }

  //Needed to work with ArrayHandleVirtual
  template <typename PortalType>
  VTKM_EXEC bool operator()(
    const vtkm::internal::ArrayPortalValueReference<PortalType>& value) const
  {
    using T = typename PortalType::ValueType;
    return value.Get() >= static_cast<T>(this->Lower) && value.Get() <= static_cast<T>(this->Upper);
  }

private:
  vtkm::Float64 Lower;
  vtkm::Float64 Upper;
};

bool DoMapField(vtkm::cont::DataSet& result,
                const vtkm::cont::Field& field,
                const vtkm::worklet::Threshold& Worklet)
{
  if (field.IsFieldPoint() || field.IsFieldGlobal())
  {
    //we copy the input handle to the result dataset, reusing the metadata
    result.AddField(field);
    return true;
  }
  else if (field.IsFieldCell())
  {
    return vtkm::filter::MapFieldPermutation(field, Worklet.GetValidCellIds(), result);
  }
  else
  {
    return false;
  }
}

} // end anon namespace

//-----------------------------------------------------------------------------
vtkm::cont::DataSet Threshold::DoExecute(const vtkm::cont::DataSet& input)
{
  //get the cells and coordinates of the dataset
  const vtkm::cont::UnknownCellSet& cells = input.GetCellSet();
  const auto& field = this->GetFieldFromDataSet(input);

  ThresholdRange predicate(this->GetLowerThreshold(), this->GetUpperThreshold());
  vtkm::worklet::Threshold Worklet;
  vtkm::cont::UnknownCellSet cellOut;

  auto ResolveArrayType = [&, this](const auto& concrete) {
    // TODO: document the reason a .ResetCellSetList is needed here.
    cellOut = Worklet.Run(cells.ResetCellSetList<VTKM_DEFAULT_CELL_SET_LIST>(),
                          concrete,
                          field.GetAssociation(),
                          predicate,
                          this->GetAllInRange());
  };

  const auto& fieldArray = field.GetData();
  fieldArray.CastAndCallForTypes<vtkm::TypeListScalarAll, VTKM_DEFAULT_STORAGE_LIST>(
    ResolveArrayType);

  vtkm::cont::DataSet output;
  output.SetCellSet(cellOut);
  output.AddCoordinateSystem(input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()));

  auto mapper = [&](auto& result, const auto& f) { DoMapField(result, f, Worklet); };
  MapFieldsOntoOutput(input, output, mapper);

  return output;
}
} // namespace entity_extraction
} // namespace filter
} // namespace vtkm
