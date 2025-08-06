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

#include <viskores/cont/DataSetBuilderExplicit.h>
#include <viskores/cont/DataSetBuilderRectilinear.h>
#include <viskores/cont/DataSetBuilderUniform.h>

#include <viskores/cont/testing/Testing.h>

#include <viskores/worklet/WorkletMapField.h>
#include <viskores/worklet/WorkletMapTopology.h>

#include <viskores/io/VTKDataSetWriter.h>

#include <viskores/filter/resampling/Probe.h>


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

std::vector<viskores::cont::DataSet> MakeDataSet3D(const std::vector<viskores::Id3>& dims)
{
  viskores::cont::DataSetBuilderUniform builder;
  viskores::Vec3f origin(0.0f, 0.0f, 0.0f);

  std::vector<viskores::cont::DataSet> dataSets;
  for (const auto& d : dims)
  {
    viskores::Vec3f spacing(1.0 / (d[0] - 1), 1.0 / (d[1] - 1), 1.0 / (d[2] - 1));
    auto ds = builder.Create(d, origin, spacing);

    viskores::cont::Invoker invoker;
    viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
    invoker(GenerateData{}, ds.GetCoordinateSystem(), fieldArray);
    ds.AddPointField("field", fieldArray);
    dataSets.push_back(ds);
  }

  return dataSets;
}

template <typename F>
static void fillCoords(std::vector<viskores::FloatDefault>& coords, viskores::Id N, F g)
{
  coords.resize(N);
  for (viskores::Id i = 0; i < N; ++i)
  {
    viskores::FloatDefault t = static_cast<viskores::FloatDefault>(i) / (N - 1);
    coords[i] = g(t);
  }
  coords[N - 1] = 1.0f;
}

std::vector<viskores::cont::DataSet> MakeRectDataSet3D(const std::vector<viskores::Id3>& dims)
{
  std::vector<viskores::cont::DataSet> dataSets;

  for (const auto& d : dims)
  {
    for (int i = 0; i < 4; i++)
    {
      viskores::cont::DataSetBuilderRectilinear builder;
      std::vector<viskores::FloatDefault> xcoords, ycoords, zcoords;

      if (i == 0)
      {
        //uniform spacing.
        fillCoords(xcoords, d[0], [](viskores::FloatDefault t) { return t; });
        fillCoords(ycoords, d[1], [](viskores::FloatDefault t) { return t; });
        fillCoords(zcoords, d[2], [](viskores::FloatDefault t) { return t; });
      }
      else if (i == 1)
      {
        //quadratic clustering near 0.
        fillCoords(xcoords, d[0], [](viskores::FloatDefault t) { return t * t; });
        fillCoords(ycoords, d[1], [](viskores::FloatDefault t) { return t * t; });
        fillCoords(zcoords, d[2], [](viskores::FloatDefault t) { return t * t; });
      }
      else if (i == 2)
      {
        //quadratic clustering near 1.
        fillCoords(xcoords, d[0], [](viskores::FloatDefault t) { return 1 - (1 - t) * (1 - t); });
        fillCoords(ycoords, d[1], [](viskores::FloatDefault t) { return 1 - (1 - t) * (1 - t); });
        fillCoords(zcoords, d[2], [](viskores::FloatDefault t) { return 1 - (1 - t) * (1 - t); });
      }
      else if (i == 3)
      {
        //exponential
        fillCoords(xcoords,
                   d[0],
                   [](viskores::FloatDefault t)
                   {
                     const viskores::FloatDefault k = 3;
                     return (std::exp(k * t) - 1) / (std::exp(k) - 1);
                   });
        fillCoords(ycoords,
                   d[1],
                   [](viskores::FloatDefault t)
                   {
                     const viskores::FloatDefault k = 3;
                     return (std::exp(k * t) - 1) / (std::exp(k) - 1);
                   });
        fillCoords(zcoords,
                   d[2],
                   [](viskores::FloatDefault t)
                   {
                     const viskores::FloatDefault k = 3;
                     return (std::exp(k * t) - 1) / (std::exp(k) - 1);
                   });
      }

      auto ds = builder.Create(xcoords, ycoords, zcoords);
      viskores::cont::Invoker invoker;
      viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
      invoker(GenerateData{}, ds.GetCoordinateSystem(), fieldArray);
      ds.AddPointField("field", fieldArray);
      dataSets.push_back(ds);
    }
  }

  return dataSets;
}

template <typename SplineEvalType>
void RunTest(SplineEvalType& splineEval,
             const std::vector<viskores::Vec3f>& points,
             const std::vector<viskores::FloatDefault>& expectedValues,
             const viskores::cont::DataSet& ds)
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
    auto diff = viskores::Abs(value - expectedValues[i]);
    if (diff > 1e-2)
    {
      viskores::io::VTKDataSetWriter writer("testDataSet.vtk");
      writer.WriteDataSet(ds);
      std::cout << "Crash coming: " << diff << ": " << points[i] << " " << value
                << " exp: " << expectedValues[i] << std::endl;
      ds.PrintSummary(std::cout);
    }
    VISKORES_TEST_ASSERT(diff < 1e-2, "Interpolated value differ.");
    std::cout << "Point: " << points[i] << " Value: " << value << " Expected: " << expectedValues[i]
              << " Diff= " << diff << std::endl;
  }
}

void DoSplineEvalTest()
{
  viskores::cont::ArrayHandle<viskores::Vec3f> points;
  viskores::cont::ArrayHandle<viskores::FloatDefault> results;
  EvalWorklet evalWorklet;
  std::vector<viskores::Vec3f> pointData = { { 0.15235f, 0.1525f, 0.15342f },
                                             { 0.252351f, 0.3986f, 0.1589f },
                                             { 0.85219f, 0.15764f, 0.7469f } };
  std::vector<viskores::FloatDefault> expectedValues;
  for (const auto& pt : pointData)
    expectedValues.push_back(EvaluateNormalizedGyroid(pt));

  //std::vector<viskores::Id3> dims = { { 30, 30, 30 }, { 20, 20, 20 } };
  std::vector<viskores::Id3> dims = { { 50, 50, 50 } };
  auto dsUniform = MakeDataSet3D(dims);
  auto dsRect = MakeRectDataSet3D(dims);

  for (const auto& ds : dsUniform)
  {
    viskores::cont::SplineEvaluateUniformGrid evalUniform(ds, "field");
    //RunTest(evalUniform, pointData, expectedValues, ds);
  }

  for (const auto& ds : dsRect)
  {
    viskores::cont::SplineEvaluateRectilinearGrid evalRect(ds, "field");
    RunTest(evalRect, pointData, expectedValues, ds);
  }

  viskores::filter::resampling::Probe probe;
  viskores::cont::DataSetBuilderExplicit builder;

  std::vector<viskores::Id> ids = { 0, 1, 2 };
  auto ptDataSet = builder.Create(pointData, viskores::CellShapeTagVertex, 1, ids);
  probe.SetGeometry(dsUniform[0]);
  auto output = probe.Execute(input);
  std::cout << "********* PROBE **********" << std::endl;
  output.printSummary(std::cout);
}
} // anonymous namespace

int UnitTestSplineEvaluate(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoSplineEvalTest, argc, argv);
}
