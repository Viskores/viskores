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

#include <vtkm/filter/Filter.h>
#include <vtkm/filter/vector_analysis/Gradient.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/io/VTKDataSetWriter.h>

#include <vtkm/worklet/WorkletMapField.h>

// Worklet that does the actual work on the device.
struct ComputeMagnitude : vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn inputVectors, FieldOut outputMagnitudes);

  VTKM_EXEC void operator()(const vtkm::Vec3f& inVector, vtkm::FloatDefault& outMagnitude) const
  {
    outMagnitude = vtkm::Magnitude(inVector);
  }
};

// The filter class used by external code to run the algorithm. Normally the class definition
// is in a separate header file.
class FieldMagnitude : public vtkm::filter::Filter
{
protected:
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override;
};

// Implementation for the filter. Normally this is in its own .cxx file.
VTKM_CONT vtkm::cont::DataSet FieldMagnitude::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  const vtkm::cont::Field& inField = this->GetFieldFromDataSet(inDataSet);
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> outArrayHandle;

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
