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

#include <vtkm/filter/Filter.h>

#include <vtkm/cont/Invoker.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

constexpr vtkm::Id ARRAY_SIZE = 10;

////
//// BEGIN-EXAMPLE SimpleWorklet
////
//// LABEL Inherit
struct PoundsPerSquareInchToNewtonsPerSquareMeterWorklet : vtkm::worklet::WorkletMapField
{
  //// LABEL ControlSignature
  //// BEGIN-EXAMPLE ControlSignature
  using ControlSignature = void(FieldIn psi, FieldOut nsm);
  //// END-EXAMPLE ControlSignature
  //// LABEL ExecutionSignature
  //// BEGIN-EXAMPLE ExecutionSignature
  using ExecutionSignature = void(_1, _2);
  //// END-EXAMPLE ExecutionSignature
  //// LABEL InputDomain
  //// BEGIN-EXAMPLE InputDomain
  using InputDomain = _1;
  //// END-EXAMPLE InputDomain

  //// LABEL OperatorStart
  //// BEGIN-EXAMPLE WorkletOperator
  template<typename T>
  VTKM_EXEC void operator()(const T& psi, T& nsm) const
  {
    //// END-EXAMPLE WorkletOperator
    // 1 psi = 6894.76 N/m^2
    nsm = T(6894.76f) * psi;
    //// LABEL OperatorEnd
  }
};
////
//// END-EXAMPLE SimpleWorklet
////

void DemoWorklet()
{
  ////
  //// BEGIN-EXAMPLE WorkletInvoke
  ////
  vtkm::cont::ArrayHandle<vtkm::FloatDefault> psiArray;
  // Fill psiArray with values...
  //// PAUSE-EXAMPLE
  psiArray.Allocate(ARRAY_SIZE);
  SetPortal(psiArray.WritePortal());
  //// RESUME-EXAMPLE

  //// LABEL Construct
  vtkm::cont::Invoker invoke;

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> nsmArray;
  //// LABEL Call
  invoke(PoundsPerSquareInchToNewtonsPerSquareMeterWorklet{}, psiArray, nsmArray);
  ////
  //// END-EXAMPLE WorkletInvoke
  ////
}

} // anonymous namespace

#define VTKM_FILTER_UNIT_CONVERSION_EXPORT

////
//// BEGIN-EXAMPLE SimpleField
////
namespace vtkm
{
namespace filter
{
namespace unit_conversion
{

class VTKM_FILTER_UNIT_CONVERSION_EXPORT PoundsPerSquareInchToNewtonsPerSquareMeterFilter
  : public vtkm::filter::Filter
{
public:
  VTKM_CONT PoundsPerSquareInchToNewtonsPerSquareMeterFilter();

  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inDataSet) override;
};

}
}
} // namespace vtkm::filter::unit_conversion
////
//// END-EXAMPLE SimpleField
////

namespace vtkm
{
namespace filter
{
namespace unit_conversion
{

////
//// BEGIN-EXAMPLE SimpleFieldConstructor
////
VTKM_CONT PoundsPerSquareInchToNewtonsPerSquareMeterFilter::
  PoundsPerSquareInchToNewtonsPerSquareMeterFilter()
{
  this->SetOutputFieldName("");
}
////
//// END-EXAMPLE SimpleFieldConstructor
////

////
//// BEGIN-EXAMPLE SimpleFieldDoExecute
////
VTKM_CONT vtkm::cont::DataSet
PoundsPerSquareInchToNewtonsPerSquareMeterFilter::DoExecute(
  const vtkm::cont::DataSet& inDataSet)
{
  //// LABEL InputField
  vtkm::cont::Field inField = this->GetFieldFromDataSet(inDataSet);

  vtkm::cont::UnknownArrayHandle outArray;

  //// LABEL Lambda
  auto resolveType = [&](const auto& inputArray) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(inputArray)>::ValueType;
    //// LABEL CreateOutputArray
    vtkm::cont::ArrayHandle<T> result;
    //// LABEL Invoke
    this->Invoke(
      PoundsPerSquareInchToNewtonsPerSquareMeterWorklet{}, inputArray, result);
    outArray = result;
  };

  //// LABEL CastAndCall
  this->CastAndCallScalarField(inField, resolveType);

  //// LABEL OutputName
  std::string outFieldName = this->GetOutputFieldName();
  if (outFieldName == "")
  {
    outFieldName = inField.GetName() + "_N/m^2";
  }

  //// LABEL CreateResult
  return this->CreateResultField(
    inDataSet, outFieldName, inField.GetAssociation(), outArray);
}
////
//// END-EXAMPLE SimpleFieldDoExecute
////

}
}
} // namespace vtkm::filter::unit_conversion

namespace
{

void DemoFilter()
{
  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet inData = makeData.Make3DExplicitDataSet0();

  vtkm::filter::unit_conversion::PoundsPerSquareInchToNewtonsPerSquareMeterFilter
    convertFilter;
  convertFilter.SetActiveField("pointvar");
  vtkm::cont::DataSet outData = convertFilter.Execute(inData);

  outData.PrintSummary(std::cout);
  VTKM_TEST_ASSERT(outData.HasPointField("pointvar_N/m^2"));
}

void Run()
{
  DemoWorklet();
  DemoFilter();
}

} // anonymous namespace

int GuideExampleSimpleAlgorithm(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Run, argc, argv);
}
