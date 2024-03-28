//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/Field.h>

////
//// BEGIN-EXAMPLE UseWorkletVisitPointsWithCells
////
class AverageCellField : public vtkm::worklet::WorkletVisitPointsWithCells
{
public:
  using ControlSignature = void(CellSetIn cellSet,
                                FieldInCell inputCellField,
                                FieldOut outputPointField);
  using ExecutionSignature = void(CellCount, _2, _3);

  using InputDomain = _1;

  template<typename InputCellFieldType, typename OutputFieldType>
  VTKM_EXEC void operator()(vtkm::IdComponent numCells,
                            const InputCellFieldType& inputCellField,
                            OutputFieldType& fieldAverage) const
  {
    fieldAverage = OutputFieldType(0);

    for (vtkm::IdComponent cellIndex = 0; cellIndex < numCells; cellIndex++)
    {
      fieldAverage = fieldAverage + inputCellField[cellIndex];
    }

//// PAUSE-EXAMPLE
// The following line can create a warning when converting numCells to a
// float. However, casting it is tricky since OutputFieldType could be
// a vector, and that would unnecessarily complicate this example. Instead,
// just suppress the warning.
#ifdef VTKM_MSVC
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
    //// RESUME-EXAMPLE
    fieldAverage = fieldAverage / OutputFieldType(numCells);
//// PAUSE-EXAMPLE
#ifdef VTKM_MSVC
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
  vtkm::cont::Invoker Invoke;

  vtkm::cont::DataSet Run(const vtkm::cont::DataSet& inData)
  {
    vtkm::cont::DataSet outData;

    // Copy parts of structure that should be passed through.
    outData.SetCellSet(inData.GetCellSet());
    for (vtkm::Id coordSysIndex = 0;
         coordSysIndex < inData.GetNumberOfCoordinateSystems();
         coordSysIndex++)
    {
      outData.AddCoordinateSystem(inData.GetCoordinateSystem(coordSysIndex));
    }

    // Copy all fields, converting cell fields to point fields.
    for (vtkm::Id fieldIndex = 0; fieldIndex < inData.GetNumberOfFields(); fieldIndex++)
    {
      vtkm::cont::Field inField = inData.GetField(fieldIndex);
      if (inField.GetAssociation() == vtkm::cont::Field::Association::Cells)
      {
        using T = vtkm::Float32;
        vtkm::cont::ArrayHandle<T> inFieldData;
        inField.GetData().AsArrayHandle(inFieldData);
        vtkm::cont::UnknownCellSet inCellSet = inData.GetCellSet();

        //// RESUME-EXAMPLE
        vtkm::cont::ArrayHandle<T> outFieldData;
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

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

void Test()
{
  vtkm::cont::testing::MakeTestDataSet makeTestDataSet;

  std::cout << "Making test data set." << std::endl;
  vtkm::cont::DataSet inDataSet = makeTestDataSet.Make3DUniformDataSet0();

  std::cout << "Average cell data." << std::endl;
  vtkm::cont::DataSet resultDataSet = DemoAverageCellField().Run(inDataSet);

  std::cout << "Checking cell data converted to points." << std::endl;
  vtkm::cont::Field convertedField = resultDataSet.GetField("cellvar");
  VTKM_TEST_ASSERT(convertedField.GetAssociation() ==
                     vtkm::cont::Field::Association::Cells,
                   "Result field has wrong association.");

  const vtkm::Id numPoints = 18;
  vtkm::Float64 expectedData[numPoints] = { 100.1, 100.15, 100.2, 100.1, 100.15, 100.2,
                                            100.2, 100.25, 100.3, 100.2, 100.25, 100.3,
                                            100.3, 100.35, 100.4, 100.3, 100.35, 100.4 };

  vtkm::cont::ArrayHandle<vtkm::Float32> outData;
  convertedField.GetData().AsArrayHandle(outData);
  vtkm::cont::ArrayHandle<vtkm::Float32>::ReadPortalType outPortal =
    outData.ReadPortal();
  vtkm::cont::printSummary_ArrayHandle(outData, std::cout);
  std::cout << std::endl;
  VTKM_TEST_ASSERT(outPortal.GetNumberOfValues() == numPoints,
                   "Result array wrong size.");

  for (vtkm::Id pointId = 0; pointId < numPoints; pointId++)
  {
    VTKM_TEST_ASSERT(test_equal(outPortal.Get(pointId), expectedData[pointId]),
                     "Got wrong result.");
  }
}

} // anonymous namespace

int GuideExampleUseWorkletVisitPointsWithCells(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
