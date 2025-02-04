//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>

////
//// BEGIN-EXAMPLE UseWorkletVisitPointsWithCells
////
class AverageCellField : public viskores::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInCell inputCellField,
                                FieldOut outputPointField);
  using ExecutionSignature = void(CellCount, _2, _3);

  using InputDomain = _1;

  template<typename InputCellFieldType, typename OutputFieldType>
  VISKORES_EXEC void operator()(viskores::IdComponent numCells,
                            const InputCellFieldType& inputCellField,
                            OutputFieldType& fieldAverage) const
  {
    fieldAverage = OutputFieldType(0);

    for (viskores::IdComponent cellIndex = 0; cellIndex < numCells; cellIndex++)
    {
      fieldAverage = fieldAverage + inputCellField[cellIndex];
    }

//// PAUSE-EXAMPLE
// The following line can create a warning when converting numCells to a
// float. However, casting it is tricky since OutputFieldType could be
// a vector, and that would unnecessarily complicate this example. Instead,
// just suppress the warning.
#ifdef VISKORES_MSVC
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
    //// RESUME-EXAMPLE
    fieldAverage = fieldAverage / OutputFieldType(numCells);
//// PAUSE-EXAMPLE
#ifdef VISKORES_MSVC
#pragma warning(pop)
#endif
    //// RESUME-EXAMPLE
  }
};

//
// Later in the associated Filter class...
//

//// PAUSE-EXAMPLE
struct DemoAverageCellField
{
  viskores::cont::Invoker Invoke;

  viskores::cont::DataSet Run(const viskores::cont::DataSet& inData)
  {
    viskores::cont::DataSet outData;

    // Copy parts of structure that should be passed through.
    outData.SetCellSet(inData.GetCellSet());
    for (viskores::Id coordSysIndex = 0;
         coordSysIndex < inData.GetNumberOfCoordinateSystems();
         coordSysIndex++)
    {
      outData.AddCoordinateSystem(inData.GetCoordinateSystem(coordSysIndex));
    }

    // Copy all fields, converting cell fields to point fields.
    for (viskores::Id fieldIndex = 0; fieldIndex < inData.GetNumberOfFields(); fieldIndex++)
    {
      viskores::cont::Field inField = inData.GetField(fieldIndex);
      if (inField.GetAssociation() == viskores::cont::Field::Association::Cells)
      {
        using T = viskores::Float32;
        viskores::cont::ArrayHandle<T> inFieldData;
        inField.GetData().AsArrayHandle(inFieldData);
        viskores::cont::UnknownCellSet inCellSet = inData.GetCellSet();

        //// RESUME-EXAMPLE
        viskores::cont::ArrayHandle<T> outFieldData;
        this->Invoke(AverageCellField{}, inCellSet, inFieldData, outFieldData);
        ////
        //// END-EXAMPLE UseWorkletVisitPointsWithCells
        ////

        outData.AddCellField(inField.GetName(), outFieldData);
      }
      else
      {
        outData.AddField(inField);
      }
    }

    return outData;
  }
};

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void Test()
{
  viskores::cont::testing::MakeTestDataSet makeTestDataSet;

  std::cout << "Making test data set." << std::endl;
  viskores::cont::DataSet inDataSet = makeTestDataSet.Make3DUniformDataSet0();

  std::cout << "Average cell data." << std::endl;
  viskores::cont::DataSet resultDataSet = DemoAverageCellField().Run(inDataSet);

  std::cout << "Checking cell data converted to points." << std::endl;
  viskores::cont::Field convertedField = resultDataSet.GetField("cellvar");
  VISKORES_TEST_ASSERT(convertedField.GetAssociation() ==
                     viskores::cont::Field::Association::Cells,
                   "Result field has wrong association.");

  const viskores::Id numPoints = 18;
  viskores::Float64 expectedData[numPoints] = { 100.1, 100.15, 100.2, 100.1, 100.15, 100.2,
                                            100.2, 100.25, 100.3, 100.2, 100.25, 100.3,
                                            100.3, 100.35, 100.4, 100.3, 100.35, 100.4 };

  viskores::cont::ArrayHandle<viskores::Float32> outData;
  convertedField.GetData().AsArrayHandle(outData);
  viskores::cont::ArrayHandle<viskores::Float32>::ReadPortalType outPortal =
    outData.ReadPortal();
  viskores::cont::printSummary_ArrayHandle(outData, std::cout);
  std::cout << std::endl;
  VISKORES_TEST_ASSERT(outPortal.GetNumberOfValues() == numPoints,
                   "Result array wrong size.");

  for (viskores::Id pointId = 0; pointId < numPoints; pointId++)
  {
    VISKORES_TEST_ASSERT(test_equal(outPortal.Get(pointId), expectedData[pointId]),
                     "Got wrong result.");
  }
}

} // anonymous namespace

int GuideExampleUseWorkletVisitPointsWithCells(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
