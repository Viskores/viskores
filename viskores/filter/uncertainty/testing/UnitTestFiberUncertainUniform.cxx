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

#include <iostream>
#include <viskores/cont/ArrayHandleRandomUniformReal.h>
#include <viskores/cont/DataSet.h>
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/cont/testing/Testing.h>
#include <viskores/filter/uncertainty/FiberUncertainUniform.h>

namespace
{
template <typename T>
viskores::cont::DataSet MakeFiberUncertainUniformDataSet()
{
  const viskores::Id3 dims(20, 20, 20);
  viskores::Id numPoints = dims[0] * dims[1] * dims[2];
  viskores::cont::DataSetBuilderUniform dsBuilder;
  viskores::cont::DataSet ds = dsBuilder.Create(dims);

  std::vector<T> ensembleMinX;
  std::vector<T> ensembleMaxX;
  std::vector<T> ensembleMinY;
  std::vector<T> ensembleMaxY;

  viskores::IdComponent k = 0;
  viskores::cont::ArrayHandleRandomUniformReal<viskores::FloatDefault> randomArray(numPoints * 4,
                                                                           { 0xceed });
  auto portal = randomArray.ReadPortal();

  for (viskores::Id i = 0; i < numPoints; ++i)
  {
    viskores::FloatDefault value1 = 10 + (20 * portal.Get(k));
    viskores::FloatDefault value2 = 10 + (20 * portal.Get(k + 1));
    viskores::FloatDefault value3 = 10 + (20 * portal.Get(k + 2));
    viskores::FloatDefault value4 = 10 + (20 * portal.Get(k + 3));
    ensembleMinX.push_back(static_cast<T>(viskores::Min(value1, value2)));
    ensembleMaxX.push_back(static_cast<T>(viskores::Max(value1, value2)));
    ensembleMinY.push_back(static_cast<T>(viskores::Min(value3, value4)));
    ensembleMaxY.push_back(static_cast<T>(viskores::Max(value3, value4)));
    k += 4;
  }

  ds.AddPointField("ensemble_min_x", ensembleMinX);
  ds.AddPointField("ensemble_max_x", ensembleMaxX);
  ds.AddPointField("ensemble_min_y", ensembleMinY);
  ds.AddPointField("ensemble_max_y", ensembleMaxY);

  return ds;
}

void TestFiberUncertainUniform()
{
  viskores::cont::DataSet ds = MakeFiberUncertainUniformDataSet<viskores::FloatDefault>();

  viskores::Pair<viskores::FloatDefault, viskores::FloatDefault> minAxis(15.0, 15.0);
  viskores::Pair<viskores::FloatDefault, viskores::FloatDefault> maxAxis(25.0, 25.0);

  const viskores::FloatDefault delta = 0.05f;

  viskores::filter::uncertainty::FiberUncertainUniform closedFormFilter;
  closedFormFilter.SetMinAxis(minAxis);
  closedFormFilter.SetMaxAxis(maxAxis);
  closedFormFilter.SetMinX("ensemble_min_x");
  closedFormFilter.SetMaxX("ensemble_max_x");
  closedFormFilter.SetMinY("ensemble_min_y");
  closedFormFilter.SetMaxY("ensemble_max_y");
  closedFormFilter.SetApproach("ClosedForm");

  viskores::cont::DataSet outputClosed = closedFormFilter.Execute(ds);
  viskores::cont::Field closedField = outputClosed.GetField("ClosedForm");
  viskores::cont::UnknownArrayHandle unknownClosed = closedField.GetData();

  viskores::cont::ArrayHandle<viskores::FloatDefault> closedArray;
  unknownClosed.AsArrayHandle(closedArray);
  auto closedPortal = closedArray.ReadPortal();

  // Run MonteCarlo approach
  viskores::filter::uncertainty::FiberUncertainUniform monteCarloFilter;
  monteCarloFilter.SetMinAxis(minAxis);
  monteCarloFilter.SetMaxAxis(maxAxis);
  monteCarloFilter.SetMinX("ensemble_min_x");
  monteCarloFilter.SetMaxX("ensemble_max_x");
  monteCarloFilter.SetMinY("ensemble_min_y");
  monteCarloFilter.SetMaxY("ensemble_max_y");
  monteCarloFilter.SetApproach("MonteCarlo");

  monteCarloFilter.SetNumSamples(5000);
  viskores::cont::DataSet outputMC = monteCarloFilter.Execute(ds);
  viskores::cont::Field monteField = outputMC.GetField("MonteCarlo");
  viskores::cont::UnknownArrayHandle unknownMC = monteField.GetData();
  viskores::cont::ArrayHandle<viskores::FloatDefault> monteArray;
  unknownMC.AsArrayHandle(monteArray);
  auto montePortal = monteArray.ReadPortal();
  viskores::Id numValues = closedArray.GetNumberOfValues();
  std::cout << "Comparing outputs for " << numValues << " values." << std::endl;
  for (viskores::Id i = 0; i < numValues; ++i)
  {
    viskores::FloatDefault diff = std::fabs(closedPortal.Get(i) - montePortal.Get(i));
    VISKORES_TEST_ASSERT(diff <= delta, "Difference between ClosedForm and MonteCarlo value too large.");
  }
}
}

int UnitTestFiberUncertainUniform(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(TestFiberUncertainUniform, argc, argv);
}
