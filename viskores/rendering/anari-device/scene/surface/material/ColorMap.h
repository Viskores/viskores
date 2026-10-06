//=============================================================================
//
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//
//=============================================================================

#pragma once

#include "viskores_device_math.h"

#include <viskores/cont/ArrayHandle.h>

namespace viskores_device
{

struct ColorMap
{
  viskores::cont::ArrayHandle<viskores::Vec4f_32> colors;
  viskores::IdComponent2 size;
  Mat4f_32 inFieldTransform;
  viskores::Vec4f_32 inFieldOffset;
};

}
