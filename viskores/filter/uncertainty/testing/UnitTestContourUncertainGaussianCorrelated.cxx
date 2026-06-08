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

#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/uncertainty/ContourUncertainGaussianCorrelated.h>
#include <viskores/filter/uncertainty/ContourUncertainGaussianIndependent.h>

namespace
{

template <typename T>
viskores::cont::DataSet MakeGaussianCorrelatedTestDataSet(T rhoValue = static_cast<T>(0))
{
  const viskores::Id3 dims(25, 25, 25);
  const viskores::Id numPoints = dims[0] * dims[1] * dims[2];

  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet dataSet = dataSetBuilder.Create(dims);

  // Two random draws per point: one for the mean, one for the variance.
  viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault> randomArray(numPoints * 2,
                                                                                   { 0xceed });
  auto randomPortal = randomArray.ReadPortal();

  std::vector<T> meanField;
  std::vector<T> varianceField;
  std::vector<T> rhoField(numPoints, rhoValue);
  meanField.reserve(numPoints);
  varianceField.reserve(numPoints);

  viskores::IdComponent randomIndex = 0;
  for (viskores::Id pointIndex = 0; pointIndex < numPoints; ++pointIndex)
  {
    viskores::FloatDefault meanValue = 100.0f * randomPortal.Get(randomIndex);
    viskores::FloatDefault varianceValue = 4.0f * randomPortal.Get(randomIndex + 1);
    meanField.push_back(static_cast<T>(meanValue));
    varianceField.push_back(static_cast<T>(varianceValue));
    randomIndex += 2;
  }

  dataSet.AddPointField("mean", meanField);
  dataSet.AddPointField("variance", varianceField);
  dataSet.AddPointField("rhoX", rhoField);
  dataSet.AddPointField("rhoY", rhoField);
  dataSet.AddPointField("rhoZ", rhoField);
  return dataSet;
}

void TestContourUncertainGaussianCorrelated()
{
  // All-zero covariance fields reduce the correlated filter to the independent
  // case; the cross-check below exploits this.
  viskores::cont::DataSet input = MakeGaussianCorrelatedTestDataSet<viskores::FloatDefault>(0.0f);
  const viskores::FloatDefault isovalue = 50.0;

  viskores::filter::uncertainty::ContourUncertainGaussianCorrelated correlatedFilter;
  correlatedFilter.SetMeanField("mean");
  correlatedFilter.SetVarianceField("variance");
  correlatedFilter.SetRhoXField("rhoX");
  correlatedFilter.SetRhoYField("rhoY");
  correlatedFilter.SetRhoZField("rhoZ");
  correlatedFilter.SetIsoValue(isovalue);
  correlatedFilter.SetMergeDuplicatePoints(true);
  viskores::cont::DataSet correlatedOutput = correlatedFilter.Execute(input);

  viskores::filter::uncertainty::ContourUncertainGaussianIndependent independentFilter;
  independentFilter.SetMeanField("mean");
  independentFilter.SetVarianceField("variance");
  independentFilter.SetIsoValue(isovalue);
  independentFilter.SetMergeDuplicatePoints(true);
  viskores::cont::DataSet independentOutput = independentFilter.Execute(input);

  VISKORES_TEST_ASSERT(correlatedOutput.HasPointField(correlatedFilter.GetCrossingVarianceName()),
                       "Crossing variance output field is missing.");
  VISKORES_TEST_ASSERT(correlatedOutput.HasPointField(correlatedFilter.GetExpectedCrossingName()),
                       "Expected crossing output field is missing.");

  // Cross-check: correlated with rhoX = rhoY = rhoZ = 0 must reduce to
  // independent. This exercises the full correlated path (including the
  // axis-detection logic) and only matches if the math is consistent.
  viskores::cont::ArrayHandle<viskores::FloatDefault> correlatedVarianceArray;
  correlatedOutput.GetField(correlatedFilter.GetCrossingVarianceName())
    .GetData()
    .AsArrayHandle(correlatedVarianceArray);
  viskores::cont::ArrayHandle<viskores::FloatDefault> independentVarianceArray;
  independentOutput.GetField(independentFilter.GetCrossingVarianceName())
    .GetData()
    .AsArrayHandle(independentVarianceArray);

  viskores::cont::ArrayHandle<viskores::Float64> correlatedCrossingArray;
  correlatedOutput.GetField(correlatedFilter.GetExpectedCrossingName())
    .GetData()
    .AsArrayHandle(correlatedCrossingArray);
  viskores::cont::ArrayHandle<viskores::Float64> independentCrossingArray;
  independentOutput.GetField(independentFilter.GetExpectedCrossingName())
    .GetData()
    .AsArrayHandle(independentCrossingArray);

  const viskores::Id numOutputPoints = correlatedVarianceArray.GetNumberOfValues();
  VISKORES_TEST_ASSERT(numOutputPoints > 0,
                       "Filter produced an empty isosurface; check the isovalue range.");
  VISKORES_TEST_ASSERT(independentVarianceArray.GetNumberOfValues() == numOutputPoints,
                       "Correlated and independent outputs have different point counts.");

  auto correlatedVariancePortal = correlatedVarianceArray.ReadPortal();
  auto independentVariancePortal = independentVarianceArray.ReadPortal();
  auto correlatedCrossingPortal = correlatedCrossingArray.ReadPortal();
  auto independentCrossingPortal = independentCrossingArray.ReadPortal();

  for (viskores::Id pointIndex = 0; pointIndex < numOutputPoints; ++pointIndex)
  {
    viskores::FloatDefault correlatedVariance = correlatedVariancePortal.Get(pointIndex);
    viskores::FloatDefault independentVariance = independentVariancePortal.Get(pointIndex);
    viskores::Float64 correlatedCrossing = correlatedCrossingPortal.Get(pointIndex);
    viskores::Float64 independentCrossing = independentCrossingPortal.Get(pointIndex);

    VISKORES_TEST_ASSERT(correlatedVariance >= 0.0f,
                         "Crossing variance must be non-negative; got ",
                         correlatedVariance);
    VISKORES_TEST_ASSERT(correlatedCrossing >= 0.0 && correlatedCrossing <= 1.0,
                         "Expected crossing must lie in [0, 1]; got ",
                         correlatedCrossing);
    VISKORES_TEST_ASSERT(
      test_equal(correlatedVariance, independentVariance, 1e-10),
      "Correlated-with-zero-covariance variance disagrees with independent at index ",
      pointIndex);
    VISKORES_TEST_ASSERT(
      test_equal(correlatedCrossing, independentCrossing, 1e-10),
      "Correlated-with-zero-covariance expected crossing disagrees with independent at index ",
      pointIndex);
  }
}

void TestClosedFormVsMonteCarlo()
{
  using Filter = viskores::filter::uncertainty::ContourUncertainGaussianCorrelated;

  // Use non-zero rho (0.3) so that the correlated code path is exercised beyond
  // the trivial rho=0 case above.
  viskores::cont::DataSet input = MakeGaussianCorrelatedTestDataSet<viskores::FloatDefault>(0.3f);
  const viskores::FloatDefault isovalue = 50.0;

  Filter closedFormFilter;
  closedFormFilter.SetMeanField("mean");
  closedFormFilter.SetVarianceField("variance");
  closedFormFilter.SetRhoXField("rhoX");
  closedFormFilter.SetRhoYField("rhoY");
  closedFormFilter.SetRhoZField("rhoZ");
  closedFormFilter.SetIsoValue(isovalue);
  closedFormFilter.SetMergeDuplicatePoints(true);
  closedFormFilter.SetApproach(Filter::ApproachEnum::ClosedForm);
  viskores::cont::DataSet closedFormOutput = closedFormFilter.Execute(input);

  VISKORES_TEST_ASSERT(closedFormOutput.HasPointField(closedFormFilter.GetCrossingVarianceName()),
                       "Closed-form crossing variance output field is missing.");
  VISKORES_TEST_ASSERT(closedFormOutput.HasPointField(closedFormFilter.GetExpectedCrossingName()),
                       "Closed-form expected crossing output field is missing.");

  viskores::cont::ArrayHandle<viskores::FloatDefault> closedFormVarianceArray;
  closedFormOutput.GetField(closedFormFilter.GetCrossingVarianceName())
    .GetData()
    .AsArrayHandle(closedFormVarianceArray);
  auto closedFormVariancePortal = closedFormVarianceArray.ReadPortal();

  viskores::cont::ArrayHandle<viskores::Float64> closedFormCrossingArray;
  closedFormOutput.GetField(closedFormFilter.GetExpectedCrossingName())
    .GetData()
    .AsArrayHandle(closedFormCrossingArray);
  auto closedFormCrossingPortal = closedFormCrossingArray.ReadPortal();

  const viskores::Id numOutputPoints = closedFormVarianceArray.GetNumberOfValues();
  VISKORES_TEST_ASSERT(numOutputPoints > 0,
                       "Filter produced an empty isosurface; check the isovalue range.");

  Filter monteCarloFilter;
  monteCarloFilter.SetMeanField("mean");
  monteCarloFilter.SetVarianceField("variance");
  monteCarloFilter.SetRhoXField("rhoX");
  monteCarloFilter.SetRhoYField("rhoY");
  monteCarloFilter.SetRhoZField("rhoZ");
  monteCarloFilter.SetIsoValue(isovalue);
  monteCarloFilter.SetMergeDuplicatePoints(true);
  monteCarloFilter.SetApproach(Filter::ApproachEnum::MonteCarlo);
  monteCarloFilter.SetNumberOfSamples(1000);
  viskores::cont::DataSet monteCarloOutput = monteCarloFilter.Execute(input);

  viskores::cont::ArrayHandle<viskores::FloatDefault> monteCarloVarianceArray;
  monteCarloOutput.GetField(monteCarloFilter.GetCrossingVarianceName())
    .GetData()
    .AsArrayHandle(monteCarloVarianceArray);
  auto monteCarloVariancePortal = monteCarloVarianceArray.ReadPortal();

  viskores::cont::ArrayHandle<viskores::Float64> monteCarloCrossingArray;
  monteCarloOutput.GetField(monteCarloFilter.GetExpectedCrossingName())
    .GetData()
    .AsArrayHandle(monteCarloCrossingArray);
  auto monteCarloCrossingPortal = monteCarloCrossingArray.ReadPortal();

  VISKORES_TEST_ASSERT(monteCarloVarianceArray.GetNumberOfValues() == numOutputPoints,
                       "Monte Carlo and closed-form outputs have different point counts.");

  // Tolerance reflects MC's standard error at NumberOfSamples = 5000.
  // 1/sqrt(5000) ~ 0.014 for the mean; allow generous slack for the variance.
  const viskores::Float64 expectedCrossingTolerance = 0.05;
  const viskores::FloatDefault varianceTolerance = 0.05f;

  for (viskores::Id pointIndex = 0; pointIndex < numOutputPoints; ++pointIndex)
  {
    viskores::FloatDefault monteCarloVariance = monteCarloVariancePortal.Get(pointIndex);
    viskores::Float64 monteCarloCrossing = monteCarloCrossingPortal.Get(pointIndex);

    VISKORES_TEST_ASSERT(monteCarloVariance >= 0.0f,
                         "Monte Carlo crossing variance must be non-negative; got ",
                         monteCarloVariance);
    VISKORES_TEST_ASSERT(monteCarloCrossing >= 0.0 && monteCarloCrossing <= 1.0,
                         "Monte Carlo expected crossing must lie in [0, 1]; got ",
                         monteCarloCrossing);

    viskores::Float64 closedFormCrossing = closedFormCrossingPortal.Get(pointIndex);
    viskores::FloatDefault closedFormVariance = closedFormVariancePortal.Get(pointIndex);

    VISKORES_TEST_ASSERT(
      test_equal(monteCarloCrossing, closedFormCrossing, expectedCrossingTolerance),
      "Monte Carlo expected crossing disagrees with closed-form at index ",
      pointIndex,
      ": MC=",
      monteCarloCrossing,
      " CF=",
      closedFormCrossing);
    VISKORES_TEST_ASSERT(test_equal(monteCarloVariance, closedFormVariance, varianceTolerance),
                         "Monte Carlo crossing variance disagrees with closed-form at index ",
                         pointIndex,
                         ": MC=",
                         monteCarloVariance,
                         " CF=",
                         closedFormVariance);
  }
}

void RunAllTests()
{
  TestContourUncertainGaussianCorrelated();
  TestClosedFormVsMonteCarlo();
}

} // anonymous namespace

int UnitTestContourUncertainGaussianCorrelated(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(RunAllTests, argc, argv);
}
