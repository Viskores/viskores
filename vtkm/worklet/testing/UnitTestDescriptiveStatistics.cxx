//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/DescriptiveStatistics.h>

#include <random>

void TestSingle()
{
  std::vector<vtkm::Float32> single(1, 42);
  auto single_array = vtkm::cont::make_ArrayHandle(single);
  auto result = vtkm::worklet::DescriptiveStatistics::Run(single_array);

  VTKM_TEST_ASSERT(result.N() == 1);
  VTKM_TEST_ASSERT(result.Mean() == 42);
  VTKM_TEST_ASSERT(result.PopulationVariance() == 0);

  // A single number does not have skewness nor kurtosis
  VTKM_TEST_ASSERT(result.Skewness() == 0);
  VTKM_TEST_ASSERT(result.Kurtosis() == 0);
}

void TestConstant()
{
  auto constants = vtkm::cont::make_ArrayHandleConstant(1234.f, 10000);
  auto result = vtkm::worklet::DescriptiveStatistics::Run(constants);

  VTKM_TEST_ASSERT(result.N() == 10000);
  VTKM_TEST_ASSERT(result.Sum() == 12340000);
  VTKM_TEST_ASSERT(result.PopulationVariance() == 0);
  VTKM_TEST_ASSERT(result.Skewness() == 0);
  VTKM_TEST_ASSERT(result.Kurtosis() == 0);
}

void TestIntegerSequence()
{
  // We only have 23 bits for FloatInt in Float32. This limits N to 11 bits.
  constexpr vtkm::Float32 N = 1000;

  auto integers = vtkm::cont::ArrayHandleCounting<vtkm::Float32>(0.0f, 1.0f, vtkm::Id(N));
  auto result = vtkm::worklet::DescriptiveStatistics::Run(integers);

  VTKM_TEST_ASSERT(result.N() == N);
  VTKM_TEST_ASSERT(result.Sum() == N * (N - 1) / 2);
  VTKM_TEST_ASSERT(test_equal(result.Mean(), (N - 1) / 2));

  // Expected values are from Numpy/SciPy
  VTKM_TEST_ASSERT(test_equal(result.PopulationVariance(), 83333.25));
  VTKM_TEST_ASSERT(test_equal(result.Skewness(), 0));
  // We are using the Pearson's definition, with fisher = False when calling
  // numpy.
  VTKM_TEST_ASSERT(test_equal(result.Kurtosis(), 1.8));
}

void TestStandardNormal()
{
  // Draw random numbers from the Standard Normal distribution, with mean = 0, stddev = 1
  std::mt19937 gen(0xceed);
  std::normal_distribution<vtkm::Float32> dis(0.0f, 1.0f);

  std::vector<vtkm::Float32> x(1000000);
  std::generate(x.begin(), x.end(), [&gen, &dis]() { return dis(gen); });

  auto array = vtkm::cont::make_ArrayHandle(x);
  auto result = vtkm::worklet::DescriptiveStatistics::Run(array);

  // Variance should be positive
  VTKM_TEST_ASSERT(result.SampleVariance() >= 0);
  // SampleStddev should be very close to 1.0, Skewness ~= 0 and Kurtosis ~= 3.0
  VTKM_TEST_ASSERT(test_equal(result.SampleStddev(), 1.0f, 1.0f / 100));
  VTKM_TEST_ASSERT(test_equal(result.Skewness(), 0.0f, 1.0f / 100));
  VTKM_TEST_ASSERT(test_equal(result.Kurtosis(), 3.0f, 1.0f / 100));
}

void TestCatastrophicCancellation()
{
  // Good examples of the effect of catastrophic cancellation from Wikipedia.
  std::vector<vtkm::Float64> okay{ 1e8 + 4, 1e8 + 7, 1e8 + 13, 1.0e8 + 16 };
  auto arrayOK = vtkm::cont::make_ArrayHandle(okay);
  auto resultOK = vtkm::worklet::DescriptiveStatistics::Run(arrayOK);

  VTKM_TEST_ASSERT(resultOK.N() == 4);
  VTKM_TEST_ASSERT(resultOK.Sum() == 4.0e8 + 40);
  VTKM_TEST_ASSERT(resultOK.Min() == 1.0e8 + 4);
  VTKM_TEST_ASSERT(resultOK.Max() == 1.0e8 + 16);
  VTKM_TEST_ASSERT(test_equal(resultOK.SampleVariance(), 30));
  VTKM_TEST_ASSERT(test_equal(resultOK.PopulationVariance(), 22.5));

  // Bad examples of the effect of catastrophic cancellation from Wikipedia.
  // A naive algorithm will fail in calculating the correct variance
  std::vector<vtkm::Float64> evil{ 1e9 + 4, 1e9 + 7, 1e9 + 13, 1.0e9 + 16 };
  auto arrayEvil = vtkm::cont::make_ArrayHandle(evil);
  auto resultEvil = vtkm::worklet::DescriptiveStatistics::Run(arrayEvil);

  VTKM_TEST_ASSERT(resultEvil.N() == 4);
  VTKM_TEST_ASSERT(resultEvil.Sum() == 4.0e9 + 40);
  VTKM_TEST_ASSERT(resultEvil.Min() == 1.0e9 + 4);
  VTKM_TEST_ASSERT(resultEvil.Max() == 1.0e9 + 16);
  VTKM_TEST_ASSERT(test_equal(resultEvil.SampleVariance(), 30));
  VTKM_TEST_ASSERT(test_equal(resultEvil.PopulationVariance(), 22.5));
}

void TestGeneGolub()
{
  // Bad case example proposed by Gene Golub, the variance may come out
  // as negative due to numerical precision. Thanks to Nick Thompson for
  // providing this unit test.

  // Draw random numbers from the Normal distribution, with mean = 500, stddev = 0.01
  std::mt19937 gen(0xceed);
  std::normal_distribution<vtkm::Float32> dis(500.0f, 0.01f);

  std::vector<vtkm::Float32> v(50000);
  for (float& i : v)
  {
    i = dis(gen);
  }

  auto array = vtkm::cont::make_ArrayHandle(v);
  auto result = vtkm::worklet::DescriptiveStatistics::Run(array);

  // Variance should be positive
  VTKM_TEST_ASSERT(result.SampleVariance() >= 0);
}

void TestMeanProperties()
{
  // Draw random numbers from the Normal distribution, with mean = 500, stddev = 0.01
  std::mt19937 gen(0xceed);
  std::normal_distribution<vtkm::Float32> dis(500.0f, 0.01f);

  std::vector<vtkm::Float32> x(50000);
  std::generate(x.begin(), x.end(), [&gen, &dis]() { return dis(gen); });

  // 1. Linearity, Mean(a * x + b) = a * Mean(x) + b
  std::vector<vtkm::Float32> axpb(x.size());
  std::transform(
    x.begin(), x.end(), axpb.begin(), [](vtkm::Float32 value) { return 4.0f * value + 1000.f; });

  auto x_array = vtkm::cont::make_ArrayHandle(x);
  auto axpb_array = vtkm::cont::make_ArrayHandle(axpb);

  auto mean_x = vtkm::worklet::DescriptiveStatistics::Run(x_array).Mean();
  auto mean_axpb = vtkm::worklet::DescriptiveStatistics::Run(axpb_array).Mean();

  VTKM_TEST_ASSERT(test_equal(4.0f * mean_x + 1000.f, mean_axpb, 0.01f));

  // 2. Random shuffle
  std::vector<vtkm::Float32> px = x;
  std::shuffle(px.begin(), px.end(), gen);

  auto px_array = vtkm::cont::make_ArrayHandle(px);
  auto mean_px = vtkm::worklet::DescriptiveStatistics::Run(px_array).Mean();

  VTKM_TEST_ASSERT(test_equal(mean_x, mean_px, 0.01f));
}

void TestVarianceProperty()
{
  // Draw random numbers from the Normal distribution, with mean = 500, stddev = 0.01
  std::mt19937 gen(0xceed);
  std::normal_distribution<vtkm::Float32> dis(500.0f, 0.01f);

  std::vector<vtkm::Float32> v(50000);
  std::generate(v.begin(), v.end(), [&gen, &dis]() { return dis(gen); });

  // 1. Linearity, Var(a * x + b) = a^2 * Var(x)
  std::vector<vtkm::Float32> kv(v.size());
  std::transform(
    v.begin(), v.end(), kv.begin(), [](vtkm::Float32 value) { return 4.0f * value + 5.0f; });

  auto array_v = vtkm::cont::make_ArrayHandle(v);
  auto array_kv = vtkm::cont::make_ArrayHandle(kv);
  auto result_v = vtkm::worklet::DescriptiveStatistics::Run(array_v);
  auto result_kv = vtkm::worklet::DescriptiveStatistics::Run(array_kv);
  auto mean_v = result_v.Mean();
  auto mean_kv = result_kv.Mean();
  auto var_v = result_v.SampleVariance();
  auto var_kv = result_kv.SampleVariance();

  vtkm::Float32 condition_number_kv = 0;
  auto rp = array_kv.ReadPortal();
  for (vtkm::Id i = 0; i < rp.GetNumberOfValues(); ++i)
  {
    condition_number_kv += vtkm::Abs(rp.Get(i) - mean_kv) * vtkm::Abs(rp.Get(i));
  }
  condition_number_kv *= (2.0f / ((rp.GetNumberOfValues() - 1) * var_kv));
  VTKM_TEST_ASSERT(test_equal(var_kv,
                              4.0 * 4.0 * var_v,
                              condition_number_kv * std::numeric_limits<vtkm::Float32>::epsilon()));

  // Random shuffle
  std::vector<vtkm::Float32> px = v;
  std::shuffle(px.begin(), px.end(), gen);

  auto px_array = vtkm::cont::make_ArrayHandle(px);
  auto var_px = vtkm::worklet::DescriptiveStatistics::Run(px_array).SampleVariance();

  vtkm::Float32 condition_number_v = 0;
  rp = px_array.ReadPortal();
  for (vtkm::Id i = 0; i < rp.GetNumberOfValues(); ++i)
  {
    condition_number_v += vtkm::Abs(rp.Get(i) - mean_v) * vtkm::Abs(rp.Get(i));
  }
  condition_number_v *= (2.0f / ((rp.GetNumberOfValues() - 1) * var_v));

  VTKM_TEST_ASSERT(
    test_equal(var_v, var_px, condition_number_v * std::numeric_limits<vtkm::Float32>::epsilon()));
}

void TestMomentsByKey()
{
  std::vector<vtkm::UInt32> keys{ 0, 1, 2, 2, 3, 3, 3, 4, 4, 4, 4 };

  auto values_array = vtkm::cont::make_ArrayHandleConstant(1.0f, vtkm::Id(keys.size()));
  auto keys_array = vtkm::cont::make_ArrayHandle(keys);

  auto results = vtkm::worklet::DescriptiveStatistics::Run(keys_array, values_array);
  VTKM_TEST_ASSERT(results.GetNumberOfValues() == 5);

  std::vector<vtkm::UInt32> expected_ns{ 1, 1, 2, 3, 4 };
  std::vector<vtkm::Float32> expected_sums{ 1, 1, 2, 3, 4 };
  std::vector<vtkm::Float32> expected_means{ 1, 1, 1, 1, 1 };
  using size_type = typename std::vector<vtkm::UInt32>::size_type;

  auto resultsPortal = results.ReadPortal();
  for (vtkm::Id i = 0; i < results.GetNumberOfValues(); ++i)
  {
    auto result = resultsPortal.Get(i);
    VTKM_TEST_ASSERT(result.first == vtkm::UInt32(i));
    VTKM_TEST_ASSERT(result.second.N() == expected_ns[size_type(i)]);
    VTKM_TEST_ASSERT(result.second.PopulationVariance() == 0);
  }
}

void TestDescriptiveStatistics()
{
  TestSingle();
  TestConstant();
  TestIntegerSequence();
  TestStandardNormal();
  TestCatastrophicCancellation();
  TestGeneGolub();
  TestMeanProperties();
  TestVarianceProperty();
  TestMomentsByKey();
}

int UnitTestDescriptiveStatistics(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestDescriptiveStatistics, argc, argv);
}
