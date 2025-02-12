//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/VectorAnalysis.h>

#include <viskores/cont/Initialize.h>
#include <viskores/cont/Invoker.h>

#include <viskores/filter/Filter.h>
#include <viskores/filter/vector_analysis/Gradient.h>
#include <viskores/io/VTKDataSetReader.h>
#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/worklet/WorkletMapField.h>

// Worklet that does the actual work on the device.
struct ComputeMagnitude : viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputVectors, FieldOut outputMagnitudes);

  VISKORES_EXEC void operator()(const viskores::Vec3f& inVector, viskores::FloatDefault& outMagnitude) const
  {
    outMagnitude = viskores::Magnitude(inVector);
  }
};

// The filter class used by external code to run the algorithm. Normally the class definition
// is in a separate header file.
class FieldMagnitude : public viskores::filter::Filter
{
protected:
  VISKORES_CONT viskores::cont::DataSet DoExecute(const viskores::cont::DataSet& inDataSet) override;
};

// Implementation for the filter. Normally this is in its own .cxx file.
VISKORES_CONT viskores::cont::DataSet FieldMagnitude::DoExecute(const viskores::cont::DataSet& inDataSet)
{
  const viskores::cont::Field& inField = this->GetFieldFromDataSet(inDataSet);
  viskores::cont::ArrayHandle<viskores::FloatDefault> outArrayHandle;

  auto resolveType = [&](const auto& inArrayHandle) {
    this->Invoke(ComputeMagnitude{}, inArrayHandle, outArrayHandle);
  };
  this->CastAndCallVecField<3>(inField, resolveType);

  std::string outFieldName = this->GetOutputFieldName();
  if (outFieldName == "")
  {
    outFieldName = inField.GetName() + "_magnitude";
  }

  return this->CreateResultField(inDataSet, outFieldName, inField.GetAssociation(), outArrayHandle);
}

int main(int argc, char** argv)
{
  auto opts = viskores::cont::InitializeOptions::DefaultAnyDevice;
  viskores::cont::InitializeResult config = viskores::cont::Initialize(argc, argv, opts);

  viskores::io::VTKDataSetReader reader("data/kitchen.vtk");
  viskores::cont::DataSet ds_from_file = reader.ReadDataSet();

  viskores::filter::vector_analysis::Gradient grad;
  grad.SetActiveField("c1");
  viskores::cont::DataSet ds_from_grad = grad.Execute(ds_from_file);

  FieldMagnitude mag;
  mag.SetActiveField("Gradients");
  viskores::cont::DataSet mag_grad = mag.Execute(ds_from_grad);

  viskores::io::VTKDataSetWriter writer("out_mag_grad.vtk");
  writer.WriteDataSet(mag_grad);

  return 0;
}
