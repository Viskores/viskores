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
#include <viskores/filter/uncertainty/ContourUncertainGaussianIndependent.h>

namespace
{

template <typename T>
viskores::cont::DataSet MakeGaussianIndependentTestDataSet()
{
  const viskores::Id3 dims(20, 20, 20);
  const viskores::Id numPoints = dims[0] * dims[1] * dims[2];

  viskores::cont::DataSetBuilderUniform dataSetBuilder;
  viskores::cont::DataSet dataSet = dataSetBuilder.Create(dims);

  // Two random draws per point: one for the mean, one for the variance.
  viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault> randomArray(numPoints * 2,
                                                                                   { 0xceed });
  auto randomPortal = randomArray.ReadPortal();

  std::vector<T> meanField;
  std::vector<T> varianceField;
  meanField.reserve(numPoints);
  varianceField.reserve(numPoints);

  viskores::IdComponent randomIndex = 0;
  for (viskores::Id pointIndex = 0; pointIndex < numPoints; ++pointIndex)
  {
    // Mean spans [0, 100] so that an isovalue of 50 produces a non-empty surface.
    viskores::FloatDefault meanValue = 100.0f * randomPortal.Get(randomIndex);
    // Variance lives in [0, 4]; uniform-real output is non-negative, so this
    // gives valid Gaussian variances without further filtering.
    viskores::FloatDefault varianceValue = 4.0f * randomPortal.Get(randomIndex + 1);
    meanField.push_back(static_cast<T>(meanValue));
    varianceField.push_back(static_cast<T>(varianceValue));
    randomIndex += 2;
  }

  dataSet.AddPointField("mean", meanField);
  dataSet.AddPointField("variance", varianceField);
  return dataSet;
}

void TestContourUncertainGaussianIndependent()
{
  using Filter = viskores::filter::uncertainty::ContourUncertainGaussianIndependent;
  viskores::cont::DataSet input = MakeGaussianIndependentTestDataSet<viskores::FloatDefault>();

  Filter closedFormFilter;
  closedFormFilter.SetMeanField("mean");
  closedFormFilter.SetVarianceField("variance");
  closedFormFilter.SetIsoValue(50.0);
  closedFormFilter.SetMergeDuplicatePoints(true);
  // ClosedForm is the default approach; setting it explicitly documents intent.
  closedFormFilter.SetApproach(Filter::ApproachEnum::ClosedForm);
  viskores::cont::DataSet closedFormOutput = closedFormFilter.Execute(input);

  VISKORES_TEST_ASSERT(closedFormOutput.HasPointField(closedFormFilter.GetCrossingVarianceName()),
                       "Crossing variance output field is missing.");
  VISKORES_TEST_ASSERT(closedFormOutput.HasPointField(closedFormFilter.GetExpectedCrossingName()),
                       "Expected crossing output field is missing.");

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
  VISKORES_TEST_ASSERT(closedFormCrossingArray.GetNumberOfValues() == numOutputPoints,
                       "Output field length mismatch between variance and expected crossing.");

  for (viskores::Id pointIndex = 0; pointIndex < numOutputPoints; ++pointIndex)
  {
    viskores::FloatDefault crossingVariance = closedFormVariancePortal.Get(pointIndex);
    viskores::Float64 expectedCrossing = closedFormCrossingPortal.Get(pointIndex);

    VISKORES_TEST_ASSERT(
      crossingVariance >= 0.0f, "Crossing variance must be non-negative; got ", crossingVariance);
    VISKORES_TEST_ASSERT(expectedCrossing >= 0.0 && expectedCrossing <= 1.0,
                         "Expected crossing must lie in [0, 1]; got ",
                         expectedCrossing);
  }

  // Monte Carlo path: same inputs, compare against the closed-form output
  // within a tolerance comparable to MC's standard error.
  Filter monteCarloFilter;
  monteCarloFilter.SetMeanField("mean");
  monteCarloFilter.SetVarianceField("variance");
  monteCarloFilter.SetIsoValue(50.0);
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
  const viskores::Float64 expectedCrossingTolerance = 0.05f;
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

} // anonymous namespace

int UnitTestContourUncertainGaussianIndependent(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestContourUncertainGaussianIndependent, argc, argv);
}
