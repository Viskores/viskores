//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/filter/FieldToColors.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
namespace
{
void TestFieldToColors()
{
  //faux input field
  constexpr vtkm::Id nvals = 8;
  constexpr int data[nvals] = { -1, 0, 10, 20, 30, 40, 50, 60 };

  //build a color table with clamping off and verify that sampling works
  vtkm::Range range{ 0.0, 50.0 };
  vtkm::cont::ColorTable table(vtkm::cont::ColorTable::Preset::COOL_TO_WARM);
  table.RescaleToRange(range);
  table.SetClampingOff();
  table.SetAboveRangeColor(vtkm::Vec<float, 3>{ 1.0f, 0.0f, 0.0f }); //red
  table.SetBelowRangeColor(vtkm::Vec<float, 3>{ 0.0f, 0.0f, 1.0f }); //green

  vtkm::cont::DataSet ds = vtkm::cont::testing::MakeTestDataSet().Make3DExplicitDataSetPolygonal();
  vtkm::cont::DataSetFieldAdd dsf;
  dsf.AddPointField(ds, "faux", data, nvals);

  vtkm::filter::FieldToColors ftc(table);
  ftc.SetOutputToRGBA();
  ftc.SetActiveField("faux");
  ftc.SetOutputFieldName("colors");

  auto rgbaResult = ftc.Execute(ds);
  VTKM_TEST_ASSERT(rgbaResult.HasField("colors", vtkm::cont::Field::Association::POINTS),
                   "Field missing.");
  vtkm::cont::Field Result = rgbaResult.GetField("colors", vtkm::cont::Field::Association::POINTS);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>> resultRGBAHandle;
  Result.GetData().CopyTo(resultRGBAHandle);

  //values confirmed with ParaView 5.4
  const vtkm::Vec<vtkm::UInt8, 4> correct_diverging_rgba_values[nvals] = {
    { 0, 0, 255, 255 },     { 59, 76, 192, 255 },   { 122, 157, 248, 255 }, { 191, 211, 246, 255 },
    { 241, 204, 184, 255 }, { 238, 134, 105, 255 }, { 180, 4, 38, 255 },    { 255, 0, 0, 255 }
  };
  auto portalRGBA = resultRGBAHandle.GetPortalConstControl();
  for (std::size_t i = 0; i < nvals; ++i)
  {
    auto result = portalRGBA.Get(static_cast<vtkm::Id>(i));
    VTKM_TEST_ASSERT(result == correct_diverging_rgba_values[i],
                     "incorrect value when interpolating between values");
  }

  //Now verify that we can switching our output mode
  ftc.SetOutputToRGB();
  auto rgbResult = ftc.Execute(ds);
  VTKM_TEST_ASSERT(rgbResult.HasField("colors", vtkm::cont::Field::Association::POINTS),
                   "Field missing.");
  Result = rgbResult.GetField("colors", vtkm::cont::Field::Association::POINTS);
  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>> resultRGBHandle;
  Result.GetData().CopyTo(resultRGBHandle);

  //values confirmed with ParaView 5.4
  const vtkm::Vec<vtkm::UInt8, 3> correct_diverging_rgb_values[nvals] = {
    { 0, 0, 255 },     { 59, 76, 192 },   { 122, 157, 248 }, { 191, 211, 246 },
    { 241, 204, 184 }, { 238, 134, 105 }, { 180, 4, 38 },    { 255, 0, 0 }
  };
  auto portalRGB = resultRGBHandle.GetPortalConstControl();
  for (std::size_t i = 0; i < nvals; ++i)
  {
    auto result = portalRGB.Get(static_cast<vtkm::Id>(i));
    VTKM_TEST_ASSERT(result == correct_diverging_rgb_values[i],
                     "incorrect value when interpolating between values");
  }
}
}

int UnitTestFieldToColors(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestFieldToColors);
}
