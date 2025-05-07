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

#include <viskores/cont/CubicHermiteSpline.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/Invoker.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/worklet/WorkletMapField.h>

namespace
{

class SplineEvalWorklet : public viskores::worklet::WorkletMapField
{
public:
  SplineEvalWorklet() {}

  using ControlSignature = void(FieldIn param, ExecObject cubicSpline, FieldOut pos);
  using ExecutionSignature = void(_1, _2, _3);
  using InputDomain = _1;

  template <typename ParamType, typename CubicSplineType, typename ResultType>
  VISKORES_EXEC void operator()(const ParamType& param,
                                const CubicSplineType& spline,
                                ResultType& pos) const
  {
    auto res = spline.Evaluate(param, pos);
    if (res != viskores::ErrorCode::Success)
      this->RaiseError("Spline evaluation failed.");
  }
};

void CheckEvaluation(const viskores::cont::CubicHermiteSpline& spline,
                     const viskores::cont::ArrayHandle<viskores::FloatDefault>& params,
                     const std::vector<viskores::Vec3f>& posAnswer)
{
  viskores::cont::Invoker invoke;
  viskores::cont::ArrayHandle<viskores::Vec3f> posResult;

  invoke(SplineEvalWorklet{}, params, spline, posResult);
  VISKORES_TEST_ASSERT(test_equal_ArrayHandles(
    posResult, viskores::cont::make_ArrayHandle(posAnswer, viskores::CopyFlag::Off)));
}

void CheckEvaluation(const viskores::cont::CubicHermiteSpline& spline,
                     const std::vector<viskores::FloatDefault>& params,
                     const std::vector<viskores::Vec3f>& posAnswer)
{
  return CheckEvaluation(
    spline, viskores::cont::make_ArrayHandle(params, viskores::CopyFlag::Off), posAnswer);
}

//convenience function to save samples for debugging.
void SaveSamples(viskores::cont::CubicHermiteSpline& spline)
{
  std::ofstream fout("pts.txt");
  fout << "I, X, Y, Z" << std::endl;
  const auto pts = spline.GetData().ReadPortal();
  for (viskores::Id i = 0; i < pts.GetNumberOfValues(); ++i)
    fout << i << ", " << pts.Get(i)[0] << ", " << pts.Get(i)[1] << ", " << pts.Get(i)[2]
         << std::endl;
  fout.close();

  fout = std::ofstream("samples.txt");
  fout << "T, X, Y, Z" << std::endl;
  viskores::Id n = 1000;

  viskores::FloatDefault t0 = spline.GetParametricRange().Min;
  viskores::FloatDefault t1 = spline.GetParametricRange().Max;
  auto dt = (t1 - t0) / static_cast<viskores::FloatDefault>(n - 1);
  std::vector<viskores::FloatDefault> params;
  for (viskores::FloatDefault t = t0; t < t1; t += dt)
    params.push_back(t);

  viskores::cont::Invoker invoke;
  viskores::cont::ArrayHandle<viskores::Vec3f> posResults;
  invoke(SplineEvalWorklet{},
         viskores::cont::make_ArrayHandle(params, viskores::CopyFlag::Off),
         spline,
         posResults);

  auto pos = posResults.ReadPortal();
  for (viskores::Id i = 0; i < pos.GetNumberOfValues(); i++)
    fout << params[i] << ", " << pos.Get(i)[0] << ", " << pos.Get(i)[1] << ", " << pos.Get(i)[2]
         << std::endl;
  fout.close();
}

void CubicHermiteSplineTest()
{
  std::vector<viskores::Vec3f> pts = { { 0, 0, 0 },  { 1, 1, 1 },  { 2, 1, 0 }, { 3, -.5, -1 },
                                       { 4, -1, 0 }, { 5, -1, 1 }, { 6, 0, 0 } };

  viskores::cont::CubicHermiteSpline spline;
  spline.SetData(pts);
  //Evaluation at knots gives the sample pts.
  CheckEvaluation(spline, spline.GetKnots(), pts);
  CheckEvaluation(spline, spline.GetKnots(), pts);

  //Evaluation at non-knot values.
  std::vector<viskores::FloatDefault> params = { 0.21, 0.465, 0.501, 0.99832 };
  std::vector<viskores::Vec3f> result = { { 1.23261, 1.08861, 0.891725 },
                                          { 2.68524, -0.0560059, -0.855685 },
                                          { 2.85574, -0.32766, -0.970523 },
                                          { 5.99045, -0.00959875, 0.00964856 } };
  CheckEvaluation(spline, params, result);

  //Explicitly set knots and check.
  std::vector<viskores::FloatDefault> knots = { 0, 1, 2, 3, 4, 5, 6 };
  spline = viskores::cont::CubicHermiteSpline();
  spline.SetData(pts);
  spline.SetKnots(knots);
  CheckEvaluation(spline, knots, pts);

  //Evaluation at non-knot values.
  params = { 0.84, 1.399, 2.838, 4.930, 5.001, 5.993 };
  result = { { 0.84, 0.896448, 0.952896 },    { 1.399, 1.14382, 0.745119 },
             { 2.838, -0.297388, -0.951764 }, { 4.93, -1.03141, 0.990543 },
             { 5.001, -0.999499, 0.999998 },  { 5.993, -0.00702441, 0.00704873 } };
  CheckEvaluation(spline, params, result);

  //Non-uniform knots.
  knots = { 0, 1, 2, 2.1, 2.2, 2.3, 3 };
  spline = viskores::cont::CubicHermiteSpline();
  spline.SetData(pts);
  spline.SetKnots(knots);

  CheckEvaluation(spline, knots, pts);

  params = { 1.5, 2.05, 2.11, 2.299, 2.8 };
  result = { { 1.39773, 1.23295, 0.727273 },
             { 2.39773, 0.357954, -0.522727 },
             { 3.1, -0.59275, -0.981 },
             { 4.99735, -1.00125, 0.999801 },
             { 5.75802, -0.293003, 0.344023 } };
  CheckEvaluation(spline, params, result);

  //Create a more complex spline from analytical functions.
  viskores::Id n = 500;
  viskores::FloatDefault t = 0.0, dt = viskores::TwoPi() / static_cast<viskores::FloatDefault>(n);

  pts.clear();
  knots.clear();
  while (t <= viskores::TwoPi())
  {
    viskores::FloatDefault x = viskores::Cos(t);
    viskores::FloatDefault y = viskores::Sin(t);
    viskores::FloatDefault z = x * y;
    pts.push_back({ x, y, z });
    knots.push_back(t);
    t += dt;
  }
  spline = viskores::cont::CubicHermiteSpline();
  spline.SetData(pts);
  spline.SetKnots(knots);
  CheckEvaluation(spline, knots, pts);

  //Evaluate at a few points and check against analytical results.
  params = { 0.15, 1.83, 2.38, 3.0291, 3.8829, 4.92, 6.2 };
  result.clear();
  for (const auto& p : params)
    result.push_back({ viskores::Cos(p), viskores::Sin(p), viskores::Cos(p) * viskores::Sin(p) });
  CheckEvaluation(spline, params, result);
}

} // anonymous namespace

int UnitTestCubicHermiteSpline(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(CubicHermiteSplineTest, argc, argv);
}
