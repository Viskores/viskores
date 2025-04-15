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

#include <viskores/filter/contour/ClipWithField.h>
#include <viskores/filter/contour/ClipWithImplicitFunction.h>
#include <viskores/filter/contour/Contour.h>
#include <viskores/filter/field_transform/PointElevation.h>
#include <viskores/filter/flow/Pathline.h>
#include <viskores/filter/flow/StreamSurface.h>
#include <viskores/filter/flow/Streamline.h>
#include <viskores/filter/geometry_refinement/Tube.h>
#include <viskores/filter/geometry_refinement/VertexClustering.h>

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderUniform.h>

#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

////
//// BEGIN-EXAMPLE PointElevation
////
VISKORES_CONT
viskores::cont::DataSet ComputeAirPressure(viskores::cont::DataSet dataSet)
{
  //// LABEL Construct
  viskores::filter::field_transform::PointElevation elevationFilter;

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
  viskores::cont::DataSet result = elevationFilter.Execute(dataSet);

  return result;
}
////
//// END-EXAMPLE PointElevation
////

void DoPointElevation()
{
  std::cout << "** Run elevation filter" << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;
  viskores::cont::DataSet inData = makeData.Make3DRegularDataSet0();

  viskores::cont::DataSet pressureData = ComputeAirPressure(inData);

  pressureData.GetField("pressure").PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoVertexClustering()
{
  std::cout << "** Run vertex clustering filter" << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;
  viskores::cont::DataSet originalSurface = makeData.Make3DExplicitDataSetCowNose();

  ////
  //// BEGIN-EXAMPLE VertexClustering
  ////
  viskores::filter::geometry_refinement::VertexClustering vertexClustering;

  vertexClustering.SetNumberOfDivisions(viskores::Id3(128, 128, 128));

  viskores::cont::DataSet simplifiedSurface = vertexClustering.Execute(originalSurface);
  ////
  //// END-EXAMPLE VertexClustering
  ////

  simplifiedSurface.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoClipWithImplicitFunction()
{
  std::cout << "** Run clip with implicit function filter" << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;
  viskores::cont::DataSet inData = makeData.Make3DUniformDataSet0();

  ////
  //// BEGIN-EXAMPLE ClipWithImplicitFunction
  ////
  ////
  //// BEGIN-EXAMPLE ImplicitFunctionGeneral
  ////
  // Parameters needed for implicit function
  viskores::Sphere implicitFunction(viskores::make_Vec(1, 0, 1), 0.5);

  // Create an instance of a clip filter with this implicit function.
  viskores::filter::contour::ClipWithImplicitFunction clip;
  clip.SetImplicitFunction(implicitFunction);
  ////
  //// END-EXAMPLE ImplicitFunctionGeneral
  ////

  // By default, ClipWithImplicitFunction will remove everything inside the sphere.
  // Set the invert clip flag to keep the inside of the sphere and remove everything
  // else.
  clip.SetInvertClip(true);

  // Execute the clip filter
  viskores::cont::DataSet outData = clip.Execute(inData);
  ////
  //// END-EXAMPLE ClipWithImplicitFunction
  ////

  outData.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoContour()
{
  std::cout << "** Run Contour filter" << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;
  viskores::cont::DataSet inData = makeData.Make3DRectilinearDataSet0();

  ////
  //// BEGIN-EXAMPLE Contour
  ////
  viskores::filter::contour::Contour contour;

  contour.SetActiveField("pointvar");
  contour.SetIsoValue(10.0);

  viskores::cont::DataSet isosurface = contour.Execute(inData);
  ////
  //// END-EXAMPLE Contour
  ////

  isosurface.PrintSummary(std::cout);
  std::cout << std::endl;

  viskores::filter::contour::Contour filter = contour;
  ////
  //// BEGIN-EXAMPLE SetActiveFieldWithAssociation
  ////
  filter.SetActiveField("pointvar", viskores::cont::Field::Association::Points);
  ////
  //// END-EXAMPLE SetActiveFieldWithAssociation
  ////
  viskores::cont::DataSet other = filter.Execute(inData);
  VISKORES_TEST_ASSERT(isosurface.GetNumberOfCells() == other.GetNumberOfCells());
  VISKORES_TEST_ASSERT(isosurface.GetNumberOfPoints() == other.GetNumberOfPoints());
}

void DoClipWithField()
{
  std::cout << "** Run clip with field filter" << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;
  viskores::cont::DataSet inData = makeData.Make3DUniformDataSet0();

  ////
  //// BEGIN-EXAMPLE ClipWithField
  ////
  // Create an instance of a clip filter that discards all regions with scalar
  // value less than 25.
  viskores::filter::contour::ClipWithField clip;
  clip.SetClipValue(25.0);
  clip.SetActiveField("pointvar");

  // Execute the clip filter
  viskores::cont::DataSet outData = clip.Execute(inData);
  ////
  //// END-EXAMPLE ClipWithField
  ////

  outData.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoStreamlines()
{
  std::cout << "** Run streamlines filter" << std::endl;

  viskores::cont::DataSetBuilderUniform dataSetBuilder;

  viskores::cont::DataSet inData = dataSetBuilder.Create(viskores::Id3(5, 5, 5));
  viskores::Id numPoints = inData.GetCellSet().GetNumberOfPoints();

  viskores::cont::ArrayHandle<viskores::Vec3f> vectorField;
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandleConstant(viskores::Vec3f(1, 0, 0), numPoints),
    vectorField);
  inData.AddPointField("vectorvar", vectorField);

  ////
  //// BEGIN-EXAMPLE Streamlines
  ////
  viskores::filter::flow::Streamline streamlines;

  // Specify the seeds.
  viskores::cont::ArrayHandle<viskores::Particle> seedArray;
  seedArray.Allocate(2);
  seedArray.WritePortal().Set(0, viskores::Particle({ 0, 0, 0 }, 0));
  seedArray.WritePortal().Set(1, viskores::Particle({ 1, 1, 1 }, 1));

  streamlines.SetActiveField("vectorvar");
  streamlines.SetStepSize(0.1f);
  streamlines.SetNumberOfSteps(100);
  streamlines.SetSeeds(seedArray);

  viskores::cont::DataSet output = streamlines.Execute(inData);
  ////
  //// END-EXAMPLE Streamlines
  ////

  output.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoStreamsurface()
{
  std::cout << "** Run streamsurface filter" << std::endl;

  viskores::cont::DataSetBuilderUniform dataSetBuilder;

  viskores::cont::DataSet inData = dataSetBuilder.Create(viskores::Id3(5, 5, 5));
  viskores::Id numPoints = inData.GetCellSet().GetNumberOfPoints();

  viskores::cont::ArrayHandle<viskores::Vec3f> vectorField;
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandleConstant(viskores::Vec3f(1, 0, 0), numPoints),
    vectorField);
  inData.AddPointField("vectorvar", vectorField);

  ////
  //// BEGIN-EXAMPLE StreamSurface
  ////
  viskores::filter::flow::StreamSurface streamSurface;

  // Specify the seeds.
  viskores::cont::ArrayHandle<viskores::Particle> seedArray;
  seedArray.Allocate(2);
  seedArray.WritePortal().Set(0, viskores::Particle({ 0, 0, 0 }, 0));
  seedArray.WritePortal().Set(1, viskores::Particle({ 1, 1, 1 }, 1));

  streamSurface.SetActiveField("vectorvar");
  streamSurface.SetStepSize(0.1f);
  streamSurface.SetNumberOfSteps(100);
  streamSurface.SetSeeds(seedArray);

  viskores::cont::DataSet output = streamSurface.Execute(inData);
  ////
  //// END-EXAMPLE StreamSurface
  ////

  output.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoTube()
{
  std::cout << "** Run tube filter" << std::endl;

  viskores::cont::DataSetBuilderExplicitIterative dsb;
  std::vector<viskores::Id> ids;
  viskores::Id pid;


  pid = dsb.AddPoint(viskores::Vec3f(1, 0, 0));
  ids.push_back(pid);
  pid = dsb.AddPoint(viskores::Vec3f(2, 1, 0));
  ids.push_back(pid);
  pid = dsb.AddPoint(viskores::Vec3f(3, 0, 0));
  ids.push_back(pid);
  dsb.AddCell(viskores::CELL_SHAPE_POLY_LINE, ids);

  viskores::cont::DataSet inData = dsb.Create();

  ////
  //// BEGIN-EXAMPLE Tube
  ////
  viskores::filter::geometry_refinement::Tube tubeFilter;

  tubeFilter.SetRadius(0.5f);
  tubeFilter.SetNumberOfSides(7);
  tubeFilter.SetCapping(true);

  viskores::cont::DataSet output = tubeFilter.Execute(inData);
  ////
  //// END-EXAMPLE Tube
  ////

  output.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoPathlines()
{
  std::cout << "** Run pathlines filter" << std::endl;

  viskores::cont::DataSetBuilderUniform dataSetBuilder;

  viskores::cont::DataSet inData1 = dataSetBuilder.Create(viskores::Id3(5, 5, 5));
  viskores::cont::DataSet inData2 = dataSetBuilder.Create(viskores::Id3(5, 5, 5));
  viskores::Id numPoints = inData1.GetCellSet().GetNumberOfPoints();

  viskores::cont::ArrayHandle<viskores::Vec3f> vectorField1;
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandleConstant(viskores::Vec3f(1, 0, 0), numPoints),
    vectorField1);
  inData1.AddPointField("vectorvar", vectorField1);

  viskores::cont::ArrayHandle<viskores::Vec3f> vectorField2;
  viskores::cont::ArrayCopy(
    viskores::cont::make_ArrayHandleConstant(viskores::Vec3f(0, 1, 0), numPoints),
    vectorField2);
  inData2.AddPointField("vectorvar", vectorField2);

  ////
  //// BEGIN-EXAMPLE Pathlines
  ////
  viskores::filter::flow::Pathline pathlines;

  // Specify the seeds.
  viskores::cont::ArrayHandle<viskores::Particle> seedArray;
  seedArray.Allocate(2);
  seedArray.WritePortal().Set(0, viskores::Particle({ 0, 0, 0 }, 0));
  seedArray.WritePortal().Set(1, viskores::Particle({ 1, 1, 1 }, 1));

  pathlines.SetActiveField("vectorvar");
  pathlines.SetStepSize(0.1f);
  pathlines.SetNumberOfSteps(100);
  pathlines.SetSeeds(seedArray);
  pathlines.SetPreviousTime(0.0f);
  pathlines.SetNextTime(1.0f);
  pathlines.SetNextDataSet(inData2);

  viskores::cont::DataSet pathlineCurves = pathlines.Execute(inData1);
  ////
  //// END-EXAMPLE Pathlines
  ////

  pathlineCurves.PrintSummary(std::cout);
  std::cout << std::endl;
}

void DoCheckFieldPassing()
{
  std::cout << "** Check field passing" << std::endl;

  viskores::cont::testing::MakeTestDataSet makeData;
  viskores::cont::DataSet inData = makeData.Make3DRectilinearDataSet0();

  viskores::cont::ArrayHandle<viskores::Float32> scalars;
  viskores::cont::ArrayCopy(viskores::cont::ArrayHandleConstant<viskores::Float32>(
                              1, inData.GetCellSet().GetNumberOfPoints()),
                            scalars);
  inData.AddPointField("scalars", scalars);

  viskores::filter::field_transform::PointElevation filter;
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
    viskores::cont::DataSet outData = filter.Execute(inData);
    for (viskores::IdComponent fieldId = 0; fieldId < inData.GetNumberOfFields();
         ++fieldId)
    {
      viskores::cont::Field inField = inData.GetField(fieldId);
      VISKORES_TEST_ASSERT(outData.HasField(inField.GetName(), inField.GetAssociation()),
                           "Did not automatically pass all fields.");
    }
  }

  {
    ////
    //// BEGIN-EXAMPLE PassNoFields
    ////
    filter.SetFieldsToPass(viskores::filter::FieldSelection::Mode::None);
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

    viskores::cont::DataSet outData = filter.Execute(inData);
    VISKORES_TEST_ASSERT(outData.GetNumberOfFields() == 1,
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

    viskores::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VISKORES_TEST_ASSERT(outData.GetNumberOfFields() == 2,
                         "Could not set field passing correctly.");
    VISKORES_TEST_ASSERT(outData.HasPointField("pointvar"));
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

    viskores::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VISKORES_TEST_ASSERT(outData.GetNumberOfFields() == 3,
                         "Could not set field passing correctly.");
    VISKORES_TEST_ASSERT(outData.HasPointField("pointvar"));
    VISKORES_TEST_ASSERT(outData.HasCellField("cellvar"));
  }

  {
    ////
    //// BEGIN-EXAMPLE PassExcludeFields
    ////
    filter.SetFieldsToPass({ "pointvar", "cellvar" },
                           viskores::filter::FieldSelection::Mode::Exclude);
    ////
    //// END-EXAMPLE PassExcludeFields
    ////

    viskores::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VISKORES_TEST_ASSERT(outData.GetNumberOfFields() == (inData.GetNumberOfFields() - 1),
                         "Could not set field passing correctly.");
    VISKORES_TEST_ASSERT(outData.HasField("scalars"));
  }

  {
    ////
    //// BEGIN-EXAMPLE FieldSelection
    ////
    viskores::filter::FieldSelection fieldSelection;
    fieldSelection.AddField("scalars");
    fieldSelection.AddField("cellvar", viskores::cont::Field::Association::Cells);

    filter.SetFieldsToPass(fieldSelection);
    ////
    //// END-EXAMPLE FieldSelection
    ////
    filter.SetPassCoordinateSystems(false);

    viskores::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VISKORES_TEST_ASSERT(outData.GetNumberOfFields() == 3,
                         "Could not set field passing correctly.");
    VISKORES_TEST_ASSERT(outData.HasField("scalars"));
    VISKORES_TEST_ASSERT(outData.HasCellField("cellvar"));
  }

  {
    ////
    //// BEGIN-EXAMPLE PassFieldAndAssociation
    ////
    filter.SetFieldsToPass("pointvar", viskores::cont::Field::Association::Points);
    ////
    //// END-EXAMPLE PassFieldAndAssociation
    ////
    filter.SetPassCoordinateSystems(false);

    viskores::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VISKORES_TEST_ASSERT(outData.GetNumberOfFields() == 2,
                         "Could not set field passing correctly.");
    VISKORES_TEST_ASSERT(outData.HasPointField("pointvar"));
  }

  {
    ////
    //// BEGIN-EXAMPLE PassListOfFieldsAndAssociations
    ////
    filter.SetFieldsToPass(
      { viskores::make_Pair("pointvar", viskores::cont::Field::Association::Points),
        viskores::make_Pair("cellvar", viskores::cont::Field::Association::Cells),
        viskores::make_Pair("scalars", viskores::cont::Field::Association::Any) });
    ////
    //// END-EXAMPLE PassListOfFieldsAndAssociations
    ////
    filter.SetPassCoordinateSystems(false);

    viskores::cont::DataSet outData = filter.Execute(inData);
    outData.PrintSummary(std::cout);
    VISKORES_TEST_ASSERT(outData.GetNumberOfFields() == 4,
                         "Could not set field passing correctly.");
    VISKORES_TEST_ASSERT(outData.HasPointField("pointvar"));
    VISKORES_TEST_ASSERT(outData.HasCellField("cellvar"));
    VISKORES_TEST_ASSERT(outData.HasField("scalars"));
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
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
