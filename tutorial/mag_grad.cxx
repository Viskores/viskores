//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/VectorAnalysis.h>

#include <vtkm/cont/Initialize.h>
#include <vtkm/cont/Invoker.h>

#include <vtkm/filter/vector_analysis/Gradient.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/worklet/WorkletMapField.h>

struct ComputeMagnitude : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputVectors, FieldOut outputMagnitudes);

  VTKM_EXEC void operator()(const vtkm::Vec3f& inVector, vtkm::FloatDefault& outMagnitude) const
  {
    outMagnitude = vtkm::Magnitude(inVector);
  }
};

#include <vtkm/filter/NewFilterField.h>

class FieldMagnitude : public vtkm::filter::NewFilterField
{
public:
  using SupportedTypes = vtkm::List<vtkm::Vec3f>;

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override
  {
    const auto& inField = this->GetFieldFromDataSet(inDataSet);
    vtkm::cont::ArrayHandle<vtkm::FloatDefault> outField;

    auto resolveType = [&](const auto& concrete) {
      this->Invoke(ComputeMagnitude{}, concrete, outField);
    };
    this->CastAndCallVecField<3>(inField, resolveType);

    std::string outFieldName = this->GetOutputFieldName();
    if (outFieldName == "")
    {
      outFieldName = inField.GetName() + "_magnitude";
    }

    return this->CreateResultFieldCell(inDataSet, outFieldName, outField);
  }
};

int main(int argc, char** argv)
{
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  vtkm::cont::InitializeResult config = vtkm::cont::Initialize(argc, argv, opts);

  vtkm::io::VTKDataSetReader reader("data/kitchen.vtk");
  vtkm::cont::DataSet ds_from_file = reader.ReadDataSet();

  vtkm::filter::vector_analysis::Gradient grad;
  grad.SetActiveField("c1");
  vtkm::cont::DataSet ds_from_grad = grad.Execute(ds_from_file);

  FieldMagnitude mag;
  mag.SetActiveField("Gradients");
  vtkm::cont::DataSet mag_grad = mag.Execute(ds_from_grad);

  vtkm::io::VTKDataSetWriter writer("out_mag_grad.vtk");
  writer.WriteDataSet(mag_grad);

  return 0;
}
