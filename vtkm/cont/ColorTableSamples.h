//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ColorTableSamples_h
#define vtk_m_cont_ColorTableSamples_h

#include <vtkm/Range.h>
#include <vtkm/cont/ArrayHandle.h>

namespace vtkm
{
namespace cont
{

/// \brief Color Sample Table used with vtkm::cont::ColorTable for fast coloring
///
/// Holds a special layout of sampled values with the pattern of
/// [Below Color, samples, last sample value again, Above Color, Nan Color ]
///
/// This layout has been chosen as it allows for efficient access for values
/// inside the range, and values outside the range. The last value being duplicated
/// a second time is an optimization for fast interpolation of values that are
/// very near to the Max value of the range.
///
///
class ColorTableSamplesRGBA
{
public:
  vtkm::Range SampleRange = { 1.0, 0.0 };
  vtkm::Int32 NumberOfSamples = 0; // this will not include end padding, NaN, Below or Above Range
  vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> Samples;
};

/// \brief Color Sample Table used with vtkm::cont::ColorTable for fast coloring
///
/// Holds a special layout of sampled values with the pattern of
/// [Below Color, samples, last sample value again, Above Color ]
///
/// This layout has been chosen as it allows for efficient access for values
/// inside the range, and values outside the range. The last value being duplicated
/// a second time is an optimization for fast interpolation of values that are
/// very near to the Max value of the range.
///
///
class ColorTableSamplesRGB
{
public:
  vtkm::Range SampleRange = { 1.0, 0.0 };
  vtkm::Int32 NumberOfSamples = 0; // this will not include end padding, NaN, Below or Above Range
  vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> Samples;
};
}
}

#endif
