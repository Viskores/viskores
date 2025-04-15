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

#include <viskores/cont/ColorTable.h>
#include <viskores/rendering/CanvasRayTracer.h>

#include <viskores/cont/testing/Testing.h>

#include <sys/stat.h>

#include <fstream>

namespace
{

static const viskores::Id TABLE_IMAGE_WIDTH = 150;
static const viskores::Id TABLE_IMAGE_HEIGHT = 20;

std::string FilenameFriendly(const std::string& name)
{
  std::string filename;
  for (auto&& ch : name)
  {
    if (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) ||
        ((ch >= '0') && (ch <= '9')))
    {
      filename.push_back(ch);
    }
    else
    {
      filename.push_back('-');
    }
  }
  return filename;
}

void CreateColorTableImage(const std::string& name)
{
  std::cout << "Creating color table " << name << std::endl;

  viskores::cont::ColorTable colorTable(name);

  // Create a CanvasRayTracer simply for the color buffer and the ability to
  // write out images.
  viskores::rendering::CanvasRayTracer canvas(TABLE_IMAGE_WIDTH, TABLE_IMAGE_HEIGHT);
  using ColorBufferType = viskores::rendering::CanvasRayTracer::ColorBufferType;
  ColorBufferType colorBuffer = canvas.GetColorBuffer();
  ColorBufferType::WritePortalType colorPortal = colorBuffer.WritePortal();
  VISKORES_TEST_ASSERT(colorPortal.GetNumberOfValues() ==
                         TABLE_IMAGE_WIDTH * TABLE_IMAGE_HEIGHT,
                       "Wrong size of color buffer.");

  viskores::cont::ArrayHandle<viskores::Vec4ui_8> temp;
  colorTable.Sample(TABLE_IMAGE_WIDTH, temp);

  constexpr viskores::Float32 conversionToFloatSpace = (1.0f / 255.0f);

  for (viskores::Id j = 0; j < TABLE_IMAGE_HEIGHT; ++j)
  {
    auto tempPortal = temp.ReadPortal();
    for (viskores::Id i = 0; i < TABLE_IMAGE_WIDTH; ++i)
    {
      auto color = tempPortal.Get(i);
      viskores::Vec4f_32 t(color[0] * conversionToFloatSpace,
                           color[1] * conversionToFloatSpace,
                           color[2] * conversionToFloatSpace,
                           color[3] * conversionToFloatSpace);
      colorPortal.Set(j * TABLE_IMAGE_WIDTH + i, t);
    }
  }

  canvas.SaveAs("color-tables/" + FilenameFriendly(name) + ".png");
}

void DoColorTables()
{
#ifndef VISKORES_MSVC
  // Disabled for MSVC because POSIX mkdir is not supported.
  // We can use std::filestyem::create_directories later when we support C++17.
  mkdir("color-tables", S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

  std::ofstream rstTable("color-tables/color-table-presets.rst",
                         std::ios::out | std::ios::trunc);
  rstTable << ".. DO NOT EDIT!\n";
  rstTable << ".. Created by GuideExampleColorTables test.\n";
  rstTable << "\n";

  viskores::cont::ColorTable table;
  std::set<std::string> names = table.GetPresets();
  for (auto& n : names)
  {
    CreateColorTableImage(n);
    rstTable << ".. |" << FilenameFriendly(n) << "| image:: images/color-tables/"
             << FilenameFriendly(n) << ".png\n";
  }
#endif
}

} // anonymous namespace

int GuideExampleColorTables(int argc, char* argv[])
{
  return viskores::cont::testing::Testing::Run(DoColorTables, argc, argv);
}
