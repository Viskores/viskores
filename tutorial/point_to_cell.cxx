//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/Initialize.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/worklet/WorkletMapTopology.h>

namespace vtkm
{
namespace worklet
{

struct ConvertPointFieldToCells : vtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn topology,
                                FieldInPoint inPointField,
                                FieldOutCell outCellField);
  using ExecutionSignature = void(_2 inPointField, _3 outCellField);
  using InputDomain = _1;

  template <typename InPointFieldVecType, typename OutCellFieldType>
  VTKM_EXEC void operator()(const InPointFieldVecType& inPointFieldVec,
                            OutCellFieldType& outCellField) const
  {
    vtkm::IdComponent numPoints = inPointFieldVec.GetNumberOfComponents();

    outCellField = OutCellFieldType(0);
    for (vtkm::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      outCellField = outCellField + inPointFieldVec[pointIndex];
    }
    outCellField =
      static_cast<OutCellFieldType>(outCellField / static_cast<vtkm::FloatDefault>(numPoints));
  }
};

} // namespace worklet
} // namespace vtkm

#include <vtkm/filter/Filter.h>

namespace vtkm
{
namespace filter
{

struct ConvertPointFieldToCells : vtkm::filter::Filter
{
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override;
};

VTKM_CONT cont::DataSet ConvertPointFieldToCells::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  const auto& inField = this->GetFieldFromDataSet(inDataSet);

  vtkm::cont::UnknownArrayHandle outArray;
  auto resolveType = [&](const auto& inConcrete) {
    using ValueType = typename std::decay_t<decltype(inConcrete)>::ValueType;

    vtkm::cont::ArrayHandle<ValueType> outConcrete;
    this->Invoke(
      vtkm::worklet::ConvertPointFieldToCells{}, inDataSet.GetCellSet(), inConcrete, outConcrete);
    outArray = outConcrete;
  };
  this->CastAndCallScalarField(inField, resolveType);

  std::string outFieldName = this->GetOutputFieldName();
  if (outFieldName == "")
  {
    outFieldName = inField.GetName();
  }
  return this->CreateResultFieldCell(inDataSet, outFieldName, outArray);
}

} // namespace filter
} // namespace vtkm


int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  const char* input = "data/kitchen.vtk";
  vtkm::io::VTKDataSetReader reader(input);
  vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();

  vtkm::filter::ConvertPointFieldToCells pointToCell;
  pointToCell.SetActiveField("c1");
  vtkm::cont::DataSet ds_from_convert = pointToCell.Execute(ds_from_file);

  vtkm::io::VTKDataSetWriter writer("out_point_to_cell.vtk");
  writer.WriteDataSet(ds_from_convert);

  return 0;
}
