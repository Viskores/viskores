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

#include <viskores/source/Amr.h>
#include <viskores/source/Oscillator.h>
#include <viskores/source/PerlinNoise.h>
#include <viskores/source/Tangle.h>
#include <viskores/source/Wavelet.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/testing/Testing.h>

namespace
{

void UseTangle()
{
  ////
  //// BEGIN-EXAMPLE SourceTangle
  ////
  viskores::source::Tangle tangle;
  tangle.SetCellDimensions({ 64, 64, 64 });

  viskores::cont::DataSet dataSet = tangle.Execute();
  ////
  //// END-EXAMPLE SourceTangle
  ////

  VISKORES_TEST_ASSERT(dataSet.HasPointField("tangle"));
}

void UsePerlinNoise()
{
  ////
  //// BEGIN-EXAMPLE SourcePerlinNoise
  ////
  viskores::source::PerlinNoise noise;
  noise.SetCellDimensions({ 64, 64, 64 });
  noise.SetOrigin({ 0.0f, 0.0f, 0.0f });
  noise.SetSeed(12345);

  viskores::cont::DataSet dataSet = noise.Execute();
  ////
  //// END-EXAMPLE SourcePerlinNoise
  ////

  VISKORES_TEST_ASSERT(dataSet.HasPointField("perlinnoise"));
}

void UseOscillator()
{
  ////
  //// BEGIN-EXAMPLE SourceOscillator
  ////
  viskores::source::Oscillator oscillator;
  oscillator.SetPointDimensions({ 65, 65, 65 });
  oscillator.SetTime(0.25f);
  oscillator.AddPeriodic(0.50f, 0.50f, 0.50f, 0.25f, 0.10f, 0.20f);
  oscillator.AddDamped(0.25f, 0.25f, 0.25f, 0.30f, 0.15f, 0.10f);

  viskores::cont::DataSet dataSet = oscillator.Execute();
  ////
  //// END-EXAMPLE SourceOscillator
  ////

  VISKORES_TEST_ASSERT(dataSet.HasPointField("oscillating"));
}

void UseWavelet()
{
  ////
  //// BEGIN-EXAMPLE SourceWavelet
  ////
  viskores::source::Wavelet wavelet;
  wavelet.SetExtent({ -32, -32, -32 }, { 32, 32, 32 });
  wavelet.SetSpacing({ 0.5f, 0.5f, 0.5f });
  wavelet.SetCenter({ 0.0f, 0.0f, 0.0f });

  viskores::cont::DataSet dataSet = wavelet.Execute();
  ////
  //// END-EXAMPLE SourceWavelet
  ////

  VISKORES_TEST_ASSERT(dataSet.HasPointField("RTData"));
}

void UseWavelet2D()
{
  ////
  //// BEGIN-EXAMPLE SourceWavelet2D
  ////
  viskores::source::Wavelet wavelet2D;
  wavelet2D.SetExtent({ -32, -32, 0 }, { 32, 32, 0 });

  viskores::cont::DataSet imageData = wavelet2D.Execute();
  ////
  //// END-EXAMPLE SourceWavelet2D
  ////

  VISKORES_TEST_ASSERT(imageData.HasPointField("RTData"));
}

void UseAmr()
{
  ////
  //// BEGIN-EXAMPLE SourceAmr
  ////
  viskores::source::Amr amr;
  amr.SetDimension(3);
  amr.SetCellsPerDimension(8);
  amr.SetNumberOfLevels(3);

  viskores::cont::PartitionedDataSet dataSet = amr.Execute();
  ////
  //// END-EXAMPLE SourceAmr
  ////

  VISKORES_TEST_ASSERT(dataSet.GetNumberOfPartitions() == 7,
                       "Unexpected number of partitions.");
  VISKORES_TEST_ASSERT(dataSet.GetPartition(0).HasPointField("RTData"));
  VISKORES_TEST_ASSERT(dataSet.GetPartition(0).HasCellField("RTDataCells"));
}

void Test()
{
  UseTangle();
  UsePerlinNoise();
  UseOscillator();
  UseWavelet();
  UseWavelet2D();
  UseAmr();
}

} // namespace

int GuideExampleSource(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(Test, argc, argv);
}
