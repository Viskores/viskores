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

#include <random>
#include <string>

#include <viskores/Math.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/SplineEvaluateStructuredGrid.h>

#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/MakeTestDataSet.h>
#include <viskores/cont/testing/Testing.h>

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/io/VTKDataSetWriter.h>

namespace
{

VISKORES_EXEC
viskores::FloatDefault EvaluateNormalizedGyroid(const viskores::Vec3f& point)
{
  //f(x,y,z)=sin(2πx)cos(2πy)+sin(2πy)cos(2πz)+sin(2πz)cos(2πx)
  return viskores::Sin(viskores::TwoPi() * point[0]) * viskores::Cos(viskores::TwoPi() * point[1]) +
    viskores::Sin(viskores::TwoPi() * point[1]) * viskores::Cos(viskores::TwoPi() * point[2]) +
    viskores::Sin(viskores::TwoPi() * point[2]) * viskores::Cos(viskores::TwoPi() * point[0]);
}

class EvalWorklet : public viskores::worklet::WorkletMapField
{
public:
  EvalWorklet() {}

  using ControlSignature = void(FieldIn pointsIn, ExecObject splineEval, FieldOut results);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename PointType, typename SplineEvalType, typename ResultType>
  VISKORES_EXEC void operator()(const PointType& pointIn,
                                const SplineEvalType& splineEval,
                                ResultType& result) const
  {
    splineEval.Evaluate(pointIn, result);
  }
};

class GenerateData : public viskores::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn pointsIn, FieldOut results);
  //using ControlSignature = void(FieldIn coords, FieldOut fieldOut);
  using ExecutionSignature = void(_1, _2);

  VISKORES_EXEC
  void operator()(const viskores::Vec3f& point, viskores::FloatDefault& value) const
  {
    // Example function to generate a field value based on the point coordinates.
    // This is a placeholder for the actual field generation logic.
    value = EvaluateNormalizedGyroid(point);
  }
};


viskores::cont::DataSet MakeDataSet3D(viskores::Id3 dims)
{
  viskores::cont::DataSetBuilderUniform builder;
  viskores::Vec3f origin(0.0f, 0.0f, 0.0f);
  viskores::Vec3f spacing(1.0 / (dims[0] - 1), 1.0 / (dims[1] - 1), 1.0 / (dims[2] - 1));
  auto ds = builder.Create(dims, origin, spacing);

  viskores::cont::Invoker invoker;
  viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
  invoker(GenerateData(), ds.GetCoordinateSystem(), fieldArray);
  ds.AddPointField("field", fieldArray);

  viskores::io::VTKDataSetWriter writer("testDataSet.vtk");
  writer.WriteDataSet(ds);

  return ds;
}

viskores::cont::DataSet MakeRectDataSet3D(viskores::Id3 dims)
{
  viskores::cont::DataSetBuilderRectilinear builder;
  std::vector<viskores::FloatDefault> xcoords, ycoords, zcoords;
  viskores::FloatDefault dx = 1.0f / (dims[0] - 1);
  viskores::FloatDefault dy = 1.0f / (dims[1] - 1);
  viskores::FloatDefault dz = 1.0f / (dims[2] - 1);

  viskores::FloatDefault x = 0, y = 0, z = 0;
  for (viskores::Id i = 0; i < dims[0]; i++, x += dx)
    xcoords.push_back(x);
  for (viskores::Id i = 0; i < dims[1]; i++, y += dy)
    ycoords.push_back(y);
  for (viskores::Id i = 0; i < dims[2]; i++, z += dz)
    zcoords.push_back(z);

  auto ds = builder.Create(xcoords, ycoords, zcoords);
  viskores::cont::Invoker invoker;
  viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
  invoker(GenerateData(), ds.GetCoordinateSystem(), fieldArray);
  ds.AddPointField("field", fieldArray);

  viskores::io::VTKDataSetWriter writer("testRectDataSet.vtk");
  writer.WriteDataSet(ds);

  return ds;
}

template <typename SplineEvalType>
void RunTest(SplineEvalType& splineEval,
             const std::vector<viskores::Vec3f>& points,
             const std::vector<viskores::FloatDefault>& expectedValues)
{
  viskores::cont::Invoker invoke;
  viskores::cont::ArrayHandle<viskores::Vec3f> pointsHandle =
    viskores::cont::make_ArrayHandle(points, viskores::CopyFlag::On);
  viskores::cont::ArrayHandle<viskores::FloatDefault> results;

  EvalWorklet evalWorklet;
  invoke(evalWorklet, pointsHandle, splineEval, results);

  auto portal = results.ReadPortal();
  for (viskores::Id i = 0; i < portal.GetNumberOfValues(); i++)
  {
    auto value = portal.Get(i);
    auto diff = value - expectedValues[i];
    std::cout << "Point: " << points[i] << " Value: " << value << " Expected: " << expectedValues[i]
              << " Diff= " << diff << std::endl;
  }
}

void DoSplineEvalTest()
{
  viskores::Id3 dims(20, 20, 20);

  auto dsUniform = MakeDataSet3D(dims);
  auto dsRect = MakeRectDataSet3D(dims);

  viskores::cont::ArrayHandle<viskores::Vec3f> points;
  viskores::cont::ArrayHandle<viskores::FloatDefault> results;
  EvalWorklet evalWorklet;
  std::vector<viskores::Vec3f> pointData = { { 0.15235f, 0.1525f, 0.15342f },
                                             { 0.252351f, 0.3986f, 0.1589f },
                                             { 0.85219f, 0.15764f, 0.7469f } };
  std::vector<viskores::FloatDefault> expectedValues;
  for (const auto& pt : pointData)
    expectedValues.push_back(EvaluateNormalizedGyroid(pt));

  viskores::cont::SplineEvaluateUniformGrid evalUniform(dsUniform, "field");
  RunTest(evalUniform, pointData, expectedValues);

  viskores::cont::SplineEvaluateRectilinearGrid evalRect(dsRect, "field");
  RunTest(evalUniform, pointData, expectedValues);
}
} // anonymous namespace

int UnitTestSplineEvaluate(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoSplineEvalTest, argc, argv);
}
