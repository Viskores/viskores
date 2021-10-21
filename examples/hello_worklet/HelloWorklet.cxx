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

#include <vtkm/filter/CreateResult.h>
#include <vtkm/filter/FilterField.h>

#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/cont/Initialize.h>

#include <vtkm/VectorAnalysis.h>

#include <cstdlib>
#include <iostream>

namespace vtkm
{
namespace worklet
{

struct HelloWorklet : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inVector, FieldOut outMagnitude);

  VTKM_EXEC void operator()(const vtkm::Vec3f& inVector, vtkm::FloatDefault& outMagnitude) const
  {
    outMagnitude = vtkm::Magnitude(inVector);
  }
};
}
} // namespace vtkm::worklet

namespace vtkm
{
namespace filter
{

class HelloField : public vtkm::filter::FilterField<HelloField>
{
public:
  // Specify that this filter operates on 3-vectors
  using SupportedTypes = vtkm::TypeListFieldVec3;

  template <typename FieldType, typename Policy>
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet,
                                          const FieldType& inField,
                                          const vtkm::filter::FieldMetadata& fieldMetadata,
                                          vtkm::filter::PolicyBase<Policy>)
  {
    VTKM_IS_ARRAY_HANDLE(FieldType);

    //construct our output
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> outField;

    //construct our invoker to launch worklets
    vtkm::worklet::HelloWorklet mag;
    this->Invoke(mag, inField, outField); //launch mag worklets

    //construct output field information
    if (this->GetOutputFieldName().empty())
    {
      this->SetOutputFieldName(fieldMetadata.GetName() + "_magnitude");
    }

    //return the result, which is the input data with the computed field added to it
    return vtkm::filter::CreateResult(
      inDataSet, outField, this->GetOutputFieldName(), fieldMetadata);
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
