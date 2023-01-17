//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/worklet/WorkletMapField.h>

#include <vtkm/filter/FilterField.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/cont/Initialize.h>

#include <vtkm/VectorAnalysis.h>

#include <cstdlib>
#include <iostream>

namespace hello_worklet_example
{

struct HelloWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inVector, FieldOut outMagnitude);

  template <typename T>
  VTKM_EXEC void operator()(const vtkm::Vec<T, 3>& inVector, T& outMagnitude) const
  {
    outMagnitude = vtkm::Magnitude(inVector);
  }
};

} // namespace hello_worklet_example

namespace vtkm
{
namespace filter
{

class HelloField : public vtkm::filter::FilterField
{
public:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet)
  {
    // Input field
    vtkm::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

    // Holder for output
    vtkm::cont::UnknownArrayHandle outArray;

    hello_worklet_example::HelloWorklet mag;
    auto resolveType = [&](const auto& inputArray) {
      // use std::decay to remove const ref from the decltype of concrete.
      using T = typename std::decay_t<decltype(inputArray)>::ValueType::ComponentType;
      vtkm::cont::ArrayHandle<T> result;
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
} // vtkm::filter


int main(int argc, char** argv)
{
  vtkm::cont::Initialize(argc, argv);

  if ((argc < 3) || (argc > 4))
  {
    std::cerr << "Usage: " << argv[0] << " in_data.vtk field_name [out_data.vtk]\n\n";
    std::cerr << "For example, you could use the simple_unstructured_bin.vtk that comes with the "
                 "VTK-m source:\n\n";
    std::cerr
      << "  " << argv[0]
      << " <path-to-vtkm-source>/data/data/unstructured/simple_unstructured_bin.vtk vectors\n";
    return 1;
  }
  std::string infilename = argv[1];
  std::string infield = argv[2];
  std::string outfilename = "out_data.vtk";
  if (argc == 4)
  {
    outfilename = argv[3];
  }

  vtkm::io::VTKDataSetReader reader(infilename);
  vtkm::cont::DataSet inputData = reader.ReadDataSet();

  vtkm::filter::HelloField helloField;
  helloField.SetActiveField(infield);
  vtkm::cont::DataSet outputData = helloField.Execute(inputData);

  vtkm::io::VTKDataSetWriter writer(outfilename);
  writer.WriteDataSet(outputData);

  return 0;
}
