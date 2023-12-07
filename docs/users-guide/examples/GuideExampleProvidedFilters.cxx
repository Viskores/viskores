//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/filter/contour/ClipWithField.h>
#include <vtkm/filter/contour/ClipWithImplicitFunction.h>
#include <vtkm/filter/contour/Contour.h>
#include <vtkm/filter/field_transform/PointElevation.h>
#include <vtkm/filter/flow/Pathline.h>
#include <vtkm/filter/flow/StreamSurface.h>
#include <vtkm/filter/flow/Streamline.h>
#include <vtkm/filter/geometry_refinement/Tube.h>
#include <vtkm/filter/geometry_refinement/VertexClustering.h>

#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderUniform.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE PointElevation
////
VTKM_CONT
vtkm::cont::DataSet ComputeAirPressure(vtkm::cont::DataSet dataSet)
{
  //// LABEL Construct
  vtkm::filter::field_transform::PointElevation elevationFilter;

  // Use the elevation filter to estimate atmospheric pressure based on the
  // height of the point coordinates. Atmospheric pressure is 101325 Pa at
  // sea level and drops about 12 Pa per meter.
  //// LABEL SetStateStart
  elevationFilter.SetLowPoint(0.0, 0.0, 0.0);
  elevationFilter.SetHighPoint(0.0, 0.0, 2000.0);
  elevationFilter.SetRange(101325.0, 77325.0);

  //// LABEL SetInputField
  elevationFilter.SetUseCoordinateSystemAsField(true);

  //// LABEL SetStateEnd
  //// LABEL SetOutputField
  elevationFilter.SetOutputFieldName("pressure");

  //// LABEL Execute
  vtkm::cont::DataSet result = elevationFilter.Execute(dataSet);

  return result;
}
////
//// END-EXAMPLE PointElevation
////

void DoPointElevation()
{
  std::cout << "** Run elevation filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet inData = makeData.Make3DRegularDataSet0();

  vtkm::cont::DataSet pressureData = ComputeAirPressure(inData);

  pressureData.GetField("pressure").PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoVertexClustering()
{
  std::cout << "** Run vertex clustering filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet originalSurface = makeData.Make3DExplicitDataSetCowNose();

  ////
  //// BEGIN-EXAMPLE VertexClustering
  ////
  vtkm::filter::geometry_refinement::VertexClustering vertexClustering;

  vertexClustering.SetNumberOfDivisions(vtkm::Id3(128, 128, 128));

  vtkm::cont::DataSet simplifiedSurface = vertexClustering.Execute(originalSurface);
  ////
  //// END-EXAMPLE VertexClustering
  ////

  simplifiedSurface.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoClipWithImplicitFunction()
{
  std::cout << "** Run clip with implicit function filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet inData = makeData.Make3DUniformDataSet0();

  ////
  //// BEGIN-EXAMPLE ClipWithImplicitFunction
  ////
  ////
  //// BEGIN-EXAMPLE ImplicitFunctionGeneral
  ////
  // Parameters needed for implicit function
  vtkm::Sphere implicitFunction(vtkm::make_Vec(1, 0, 1), 0.5);

  // Create an instance of a clip filter with this implicit function.
  vtkm::filter::contour::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(implicitFunction);
  ////
  //// END-EXAMPLE ImplicitFunctionGeneral
  ////

  // By default, ClipWithImplicitFunction will remove everything inside the sphere.
  // Set the invert clip flag to keep the inside of the sphere and remove everything
  // else.
  clip.SetInvertClip(true);

  // Execute the clip filter
  vtkm::cont::DataSet outData = clip.Execute(inData);
  ////
  //// END-EXAMPLE ClipWithImplicitFunction
  ////

  outData.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoContour()
{
  std::cout << "** Run Contour filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet inData = makeData.Make3DRectilinearDataSet0();

  ////
  //// BEGIN-EXAMPLE Contour
  ////
  vtkm::filter::contour::Contour contour;

  contour.SetActiveField("pointvar");
  contour.SetIsoValue(10.0);

  vtkm::cont::DataSet isosurface = contour.Execute(inData);
  ////
  //// END-EXAMPLE Contour
  ////

  isosurface.PrintSummary(std::cout);
  std::cout << std::endl;

  vtkm::filter::contour::Contour filter = contour;
  ////
  //// BEGIN-EXAMPLE SetActiveFieldWithAssociation
  ////
  filter.SetActiveField("pointvar", vtkm::cont::Field::Association::Points);
  ////
  //// END-EXAMPLE SetActiveFieldWithAssociation
  ////
  vtkm::cont::DataSet other = filter.Execute(inData);
  VTKM_TEST_ASSERT(isosurface.GetNumberOfCells() == other.GetNumberOfCells());
  VTKM_TEST_ASSERT(isosurface.GetNumberOfPoints() == other.GetNumberOfPoints());
}

void DoClipWithField()
{
  std::cout << "** Run clip with field filter" << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet inData = makeData.Make3DUniformDataSet0();

  ////
  //// BEGIN-EXAMPLE ClipWithField
  ////
  // Create an instance of a clip filter that discards all regions with scalar
  // value less than 25.
  vtkm::filter::contour::ClipWithField clip;
  clip.SetClipValue(25.0);
  clip.SetActiveField("pointvar");

  // Execute the clip filter
  vtkm::cont::DataSet outData = clip.Execute(inData);
  ////
  //// END-EXAMPLE ClipWithField
  ////

  outData.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoStreamlines()
{
  std::cout << "** Run streamlines filter" << std::endl;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet inData = dataSetBuilder.Create(vtkm::Id3(5, 5, 5));
  vtkm::Id numPoints = inData.GetCellSet().GetNumberOfPoints();

  vtkm::cont::ArrayHandle<vtkm::Vec3f> vectorField;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::Vec3f(1, 0, 0), numPoints), vectorField);
  inData.AddPointField("vectorvar", vectorField);

  ////
  //// BEGIN-EXAMPLE Streamlines
  ////
  vtkm::filter::flow::Streamline streamlines;

  // Specify the seeds.
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  seedArray.Allocate(2);
  seedArray.WritePortal().Set(0, vtkm::Particle({ 0, 0, 0 }, 0));
  seedArray.WritePortal().Set(1, vtkm::Particle({ 1, 1, 1 }, 1));

  streamlines.SetActiveField("vectorvar");
  streamlines.SetStepSize(0.1f);
  streamlines.SetNumberOfSteps(100);
  streamlines.SetSeeds(seedArray);

  vtkm::cont::DataSet output = streamlines.Execute(inData);
  ////
  //// END-EXAMPLE Streamlines
  ////

  output.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoStreamsurface()
{
  std::cout << "** Run streamsurface filter" << std::endl;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet inData = dataSetBuilder.Create(vtkm::Id3(5, 5, 5));
  vtkm::Id numPoints = inData.GetCellSet().GetNumberOfPoints();

  vtkm::cont::ArrayHandle<vtkm::Vec3f> vectorField;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::Vec3f(1, 0, 0), numPoints), vectorField);
  inData.AddPointField("vectorvar", vectorField);

  ////
  //// BEGIN-EXAMPLE StreamSurface
  ////
  vtkm::filter::flow::StreamSurface streamSurface;

  // Specify the seeds.
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  seedArray.Allocate(2);
  seedArray.WritePortal().Set(0, vtkm::Particle({ 0, 0, 0 }, 0));
  seedArray.WritePortal().Set(1, vtkm::Particle({ 1, 1, 1 }, 1));

  streamSurface.SetActiveField("vectorvar");
  streamSurface.SetStepSize(0.1f);
  streamSurface.SetNumberOfSteps(100);
  streamSurface.SetSeeds(seedArray);

  vtkm::cont::DataSet output = streamSurface.Execute(inData);
  ////
  //// END-EXAMPLE StreamSurface
  ////

  output.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoTube()
{
  std::cout << "** Run tube filter" << std::endl;

  vtkm::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<vtkm::Id> ids;
  vtkm::Id pid;


  pid = dsb.AddPoint(vtkm::Vec3f(1, 0, 0));
  ids.push_back(pid);
  pid = dsb.AddPoint(vtkm::Vec3f(2, 1, 0));
  ids.push_back(pid);
  pid = dsb.AddPoint(vtkm::Vec3f(3, 0, 0));
  ids.push_back(pid);
  dsb.AddCell(vtkm::CELL_SHAPE_POLY_LINE, ids);

  vtkm::cont::DataSet inData = dsb.Create();

  ////
  //// BEGIN-EXAMPLE Tube
  ////
  vtkm::filter::geometry_refinement::Tube tubeFilter;

  tubeFilter.SetRadius(0.5f);
  tubeFilter.SetNumberOfSides(7);
  tubeFilter.SetCapping(true);

  vtkm::cont::DataSet output = tubeFilter.Execute(inData);
  ////
  //// END-EXAMPLE Tube
  ////

  output.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoPathlines()
{
  std::cout << "** Run pathlines filter" << std::endl;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  vtkm::cont::DataSet inData1 = dataSetBuilder.Create(vtkm::Id3(5, 5, 5));
  vtkm::cont::DataSet inData2 = dataSetBuilder.Create(vtkm::Id3(5, 5, 5));
  vtkm::Id numPoints = inData1.GetCellSet().GetNumberOfPoints();

  vtkm::cont::ArrayHandle<vtkm::Vec3f> vectorField1;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::Vec3f(1, 0, 0), numPoints), vectorField1);
  inData1.AddPointField("vectorvar", vectorField1);

  vtkm::cont::ArrayHandle<vtkm::Vec3f> vectorField2;
  vtkm::cont::ArrayCopy(
    vtkm::cont::make_ArrayHandleConstant(vtkm::Vec3f(0, 1, 0), numPoints), vectorField2);
  inData2.AddPointField("vectorvar", vectorField2);

  ////
  //// BEGIN-EXAMPLE Pathlines
  ////
  vtkm::filter::flow::Pathline pathlines;

  // Specify the seeds.
  vtkm::cont::ArrayHandle<vtkm::Particle> seedArray;
  seedArray.Allocate(2);
  seedArray.WritePortal().Set(0, vtkm::Particle({ 0, 0, 0 }, 0));
  seedArray.WritePortal().Set(1, vtkm::Particle({ 1, 1, 1 }, 1));

  pathlines.SetActiveField("vectorvar");
  pathlines.SetStepSize(0.1f);
  pathlines.SetNumberOfSteps(100);
  pathlines.SetSeeds(seedArray);
  pathlines.SetPreviousTime(0.0f);
  pathlines.SetNextTime(1.0f);
  pathlines.SetNextDataSet(inData2);

  vtkm::cont::DataSet pathlineCurves = pathlines.Execute(inData1);
  ////
  //// END-EXAMPLE Pathlines
  ////

  pathlineCurves.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoCheckFieldPassing()
{
  std::cout << "** Check field passing" << std::endl;

  vtkm::cont::testing::MakeTestDataSet makeData;
  vtkm::cont::DataSet inData = makeData.Make3DRectilinearDataSet0();

  vtkm::cont::ArrayHandle<vtkm::Float32> scalars;
  vtkm::cont::ArrayCopy(vtkm::cont::ArrayHandleConstant<vtkm::Float32>(
                          1, inData.GetCellSet().GetNumberOfPoints()),
                        scalars);
  inData.AddPointField("scalars", scalars);

  vtkm::filter::field_transform::PointElevation filter;
  filter.SetLowPoint(0.0, 0.0, 0.0);
  filter.SetHighPoint(0.0, 0.0, 1.0);
  filter.SetRange(0.0, 1.0);
  ////
  //// BEGIN-EXAMPLE SetCoordinateSystem
  ////
  filter.SetUseCoordinateSystemAsField(true);
  filter.SetActiveCoordinateSystem(1);
  ////
  //// END-EXAMPLE SetCoordinateSystem
  ////
  filter.SetActiveCoordinateSystem(0);
  filter.SetOutputFieldName("elevation");

  {
    vtkm::cont::DataSet outData = filter.Execute(inData);
    for (vtkm::IdComponent fieldId = 0; fieldId < inData.GetNumberOfFields(); ++fieldId)
    {
      vtkm::cont::Field inField = inData.GetField(fieldId);
      VTKM_TEST_ASSERT(outData.HasField(inField.GetName(), inField.GetAssociation()),
                       "Did not automatically pass all fields.");
    }
  }

  {
    ////
    //// BEGIN-EXAMPLE PassNoFields
    ////
    filter.SetFieldsToPass(vtkm::filter::FieldSelection::Mode::None);
    ////
    //// END-EXAMPLE PassNoFields
    ////

    ////
    //// BEGIN-EXAMPLE PassNoCoordinates
    ////
    filter.SetPassCoordinateSystems(false);
    ////
    //// END-EXAMPLE PassNoCoordinates
    ////

    vtkm::cont::DataSet outData = filter.Execute(inData);
    VTKM_TEST_ASSERT(outData.GetNumberOfFields() == 1,
                     "Could not turn off passing of fields");
  }

  {
    ////
    //// BEGIN-EXAMPLE PassOneField
    ////
    filter.SetFieldsToPass("pointvar");
    ////
    //// END-EXAMPLE PassOneField
    ////
    filter.SetPassCoordinateSystems(false);

    vtkm::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VTKM_TEST_ASSERT(outData.GetNumberOfFields() == 2,
                     "Could not set field passing correctly.");
    VTKM_TEST_ASSERT(outData.HasPointField("pointvar"));
  }

  {
    ////
    //// BEGIN-EXAMPLE PassListOfFields
    ////
    filter.SetFieldsToPass({ "pointvar", "cellvar" });
    ////
    //// END-EXAMPLE PassListOfFields
    ////
    filter.SetPassCoordinateSystems(false);

    vtkm::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VTKM_TEST_ASSERT(outData.GetNumberOfFields() == 3,
                     "Could not set field passing correctly.");
    VTKM_TEST_ASSERT(outData.HasPointField("pointvar"));
    VTKM_TEST_ASSERT(outData.HasCellField("cellvar"));
  }

  {
    ////
    //// BEGIN-EXAMPLE PassExcludeFields
    ////
    filter.SetFieldsToPass({ "pointvar", "cellvar" },
                           vtkm::filter::FieldSelection::Mode::Exclude);
    ////
    //// END-EXAMPLE PassExcludeFields
    ////

    vtkm::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VTKM_TEST_ASSERT(outData.GetNumberOfFields() == (inData.GetNumberOfFields() - 1),
                     "Could not set field passing correctly.");
    VTKM_TEST_ASSERT(outData.HasField("scalars"));
  }

  {
    ////
    //// BEGIN-EXAMPLE FieldSelection
    ////
    vtkm::filter::FieldSelection fieldSelection;
    fieldSelection.AddField("scalars");
    fieldSelection.AddField("cellvar", vtkm::cont::Field::Association::Cells);

    filter.SetFieldsToPass(fieldSelection);
    ////
    //// END-EXAMPLE FieldSelection
    ////
    filter.SetPassCoordinateSystems(false);

    vtkm::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VTKM_TEST_ASSERT(outData.GetNumberOfFields() == 3,
                     "Could not set field passing correctly.");
    VTKM_TEST_ASSERT(outData.HasField("scalars"));
    VTKM_TEST_ASSERT(outData.HasCellField("cellvar"));
  }

  {
    ////
    //// BEGIN-EXAMPLE PassFieldAndAssociation
    ////
    filter.SetFieldsToPass("pointvar", vtkm::cont::Field::Association::Points);
    ////
    //// END-EXAMPLE PassFieldAndAssociation
    ////
    filter.SetPassCoordinateSystems(false);

    vtkm::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VTKM_TEST_ASSERT(outData.GetNumberOfFields() == 2,
                     "Could not set field passing correctly.");
    VTKM_TEST_ASSERT(outData.HasPointField("pointvar"));
  }

  {
    ////
    //// BEGIN-EXAMPLE PassListOfFieldsAndAssociations
    ////
    filter.SetFieldsToPass(
      { vtkm::make_Pair("pointvar", vtkm::cont::Field::Association::Points),
        vtkm::make_Pair("cellvar", vtkm::cont::Field::Association::Cells),
        vtkm::make_Pair("scalars", vtkm::cont::Field::Association::Any) });
    ////
    //// END-EXAMPLE PassListOfFieldsAndAssociations
    ////
    filter.SetPassCoordinateSystems(false);

    vtkm::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VTKM_TEST_ASSERT(outData.GetNumberOfFields() == 4,
                     "Could not set field passing correctly.");
    VTKM_TEST_ASSERT(outData.HasPointField("pointvar"));
    VTKM_TEST_ASSERT(outData.HasCellField("cellvar"));
    VTKM_TEST_ASSERT(outData.HasField("scalars"));
  }
}

void Test()
{
  DoPointElevation();
  DoVertexClustering();
  DoClipWithImplicitFunction();
  DoContour();
  DoClipWithField();
  DoStreamlines();
  DoStreamsurface();
  DoTube();
  DoPathlines();
  DoCheckFieldPassing();
}

} // anonymous namespace

int GuideExampleProvidedFilters(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(Test, argc, argv);
}
