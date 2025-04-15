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

#include <viskores/worklet/WorkletMapField.h>

#include <viskores/filter/Filter.h>

#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/cont/Initialize.h>

#include <viskores/VectorAnalysis.h>

#include <cstdlib>
#include <iostream>

namespace hello_worklet_example
{

struct HelloWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inVector, FieldOut outMagnitude);

  template <typename T>
  VISKORES_EXEC void operator()(const viskores::Vec<T, 3>& inVector, T& outMagnitude) const
  {
    outMagnitude = viskores::Magnitude(inVector);
  }
};

} // namespace hello_worklet_example

namespace viskores
{
namespace filter
{

class HelloField : public viskores::filter::Filter
{
public:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inDataSet)
  {
    // Input field
    viskores::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

    // Holder for output
    viskores::cont::UnknownArrayHandle outArray;

    hello_worklet_example::HelloWorklet mag;
    auto resolveType = [&](const auto& inputArray)
    {
      // use std::decay to remove const ref from the decltype of concrete.
      using T = typename std::decay_t<decltype(inputArray)>::ValueType::ComponentType;
      viskores::cont::ArrayHandle<T> result;
      this->Invoke(mag, inputArray, result);
      outArray = result;
    };

    this->CastAndCallVecField<3>(inField, resolveType);

    std::string outFieldName = this->GetOutputFieldName();
    if (outFieldName.empty())
    {
      outFieldName = inField.GetName() + "_magnitude";
    }

    return this->CreateResultField(inDataSet, outFieldName, inField.GetAssociation(), outArray);
  }
};
}
} // viskores::filter


int main(int argc, char** argv)
{
  viskores::cont::Initialize(argc, argv);

  if ((argc < 3) || (argc > 4))
  {
    std::cerr << "Usage: " << argv[0] << " in_data.vtk field_name [out_data.vtk]\n\n";
    std::cerr << "For example, you could use the simple_unstructured_bin.vtk that comes with the "
                 "Viskores source:\n\n";
    std::cerr
      << "  " << argv[0]
      << " <path-to-viskores-source>/data/data/unstructured/simple_unstructured_bin.vtk vectors\n";
    return 1;
  }
  std::string infilename = argv[1];
  std::string infield = argv[2];
  std::string outfilename = "out_data.vtk";
  if (argc == 4)
  {
    outfilename = argv[3];
  }

  viskores::io::VTKDataSetReader reader(infilename);
  viskores::cont::DataSet inputData = reader.ReadDataSet();

  viskores::filter::HelloField helloField;
  helloField.SetActiveField(infield);
  viskores::cont::DataSet outputData = helloField.Execute(inputData);

  viskores::io::VTKDataSetWriter writer(outfilename);
  writer.WriteDataSet(outputData);

  return 0;
}
