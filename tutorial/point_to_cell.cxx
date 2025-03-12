//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/cont/Initialize.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/worklet/WorkletMapTopology.h>

namespace viskores
{
namespace worklet
{

struct ConvertPointFieldToCells : viskores::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn topology,
                                FieldInPoint inPointField,
                                FieldOutCell outCellField);
  using ExecutionSignature = void(_2 inPointField, _3 outCellField);
  using InputDomain = _1;

  template <typename InPointFieldVecType, typename OutCellFieldType>
  VISKORES_EXEC void operator()(const InPointFieldVecType& inPointFieldVec,
                            OutCellFieldType& outCellField) const
  {
    viskores::IdComponent numPoints = inPointFieldVec.GetNumberOfComponents();

    outCellField = OutCellFieldType(0);
    for (viskores::IdComponent pointIndex = 0; pointIndex < numPoints; ++pointIndex)
    {
      outCellField = outCellField + inPointFieldVec[pointIndex];
    }
    outCellField =
      static_cast<OutCellFieldType>(outCellField / static_cast<viskores::FloatDefault>(numPoints));
  }
};

} // namespace worklet
} // namespace viskores

#include <viskores/filter/Filter.h>

namespace viskores
{
namespace filter
{

struct ConvertPointFieldToCells : viskores::filter::Filter
{
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inDataSet) override;
};

VISKORES_CONT cont::DataSet ConvertPointFieldToCells::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  const auto& inField = this->GetFieldFromDataSet(inDataSet);

  viskores::cont::UnknownArrayHandle outArray;
  auto resolveType = [&](const auto& inConcrete) {
    using ValueType = typename std::decay_t<decltype(inConcrete)>::ValueType;

    viskores::cont::ArrayHandle<ValueType> outConcrete;
    this->Invoke(
      viskores::worklet::ConvertPointFieldToCells{}, inDataSet.GetCellSet(), inConcrete, outConcrete);
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
} // namespace viskores


int main(int argc, char** argv)
{
  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  viskores::cont::InitializeResult config = viskores::cont::Initialize(argc, argv, opts);

  const char* input = "data/kitchen.vtk";
  viskores::io::VTKDataSetReader reader(input);
  viskores::cont::DataSet ds_from_file = reader.ReadDataSet();

  viskores::filter::ConvertPointFieldToCells pointToCell;
  pointToCell.SetActiveField("c1");
  viskores::cont::DataSet ds_from_convert = pointToCell.Execute(ds_from_file);

  viskores::io::VTKDataSetWriter writer("out_point_to_cell.vtk");
  writer.WriteDataSet(ds_from_convert);

  return 0;
}
