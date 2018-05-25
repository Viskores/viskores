//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#ifndef vtk_m_worklet_colorconversion_LookupTable_h
#define vtk_m_worklet_colorconversion_LookupTable_h

#include <vtkm/cont/ColorTableSamples.h>

#include <vtkm/exec/ColorTable.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/colorconversion/Conversions.h>

#include <float.h>

namespace vtkm
{
namespace worklet
{
namespace colorconversion
{

struct LookupTableTypes : vtkm::ListTagBase<vtkm::Vec<vtkm::UInt8, 3>,
                                            vtkm::Vec<vtkm::UInt8, 4>,
                                            vtkm::Vec<vtkm::Float32, 3>,
                                            vtkm::Vec<vtkm::Float64, 4>>
{
};

struct LookupTable : public vtkm::worklet::WorkletMapField
{
  vtkm::Float32 Shift;
  vtkm::Float32 Scale;
  vtkm::Range TableRange;
  vtkm::Int32 NumberOfSamples;

  //needs to support Nan, Above, Below Range colors
  VTKM_CONT
  template <typename T>
  LookupTable(const T& colorTableSamples)
  {
    this->Shift = static_cast<vtkm::Float32>(-colorTableSamples.SampleRange.Min);
    double rangeDelta = colorTableSamples.SampleRange.Length();
    if (rangeDelta < DBL_MIN * colorTableSamples.NumberOfSamples)
    {
      // if the range is tiny, anything within the range will map to the bottom
      // of the color scale.
      this->Scale = 0.0;
    }
    else
    {
      this->Scale = static_cast<vtkm::Float32>(colorTableSamples.NumberOfSamples / rangeDelta);
    }
    this->TableRange = colorTableSamples.SampleRange;
    this->NumberOfSamples = colorTableSamples.NumberOfSamples;
  }

  using ControlSignature = void(FieldIn<> in,
                                WholeArrayIn<LookupTableTypes> lookup,
                                FieldOut<> color);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename T, typename WholeFieldIn, typename U, int N>
  VTKM_EXEC void operator()(const T& in,
                            const WholeFieldIn lookupTable,
                            vtkm::Vec<U, N>& output) const
  {
    vtkm::Float64 v = (static_cast<vtkm::Float64>(in));
    vtkm::Int32 idx = 1;

    //This logic uses how ColorTableSamples is constructed. See
    //vtkm/cont/ColorTableSamples to see why we use these magic offset values
    if (vtkm::IsNan(v))
    {
      idx = this->NumberOfSamples + 3;
    }
    else if (v < this->TableRange.Min)
    { //If we are below the color range
      idx = 0;
    }
    else if (v == this->TableRange.Min)
    { //If we are at the ranges min value
      idx = 1;
    }
    else if (v > this->TableRange.Max)
    { //If we are above the ranges max value
      idx = this->NumberOfSamples + 2;
    }
    else if (v == this->TableRange.Max)
    { //If we are at the ranges min value
      idx = this->NumberOfSamples;
    }
    else
    {
      v = (v + this->Shift) * this->Scale;
      // When v is very close to p.Range[1], the floating point calculation giving
      // idx may map above the highest value in the lookup table. That is why it
      // is padded
      idx = static_cast<vtkm::Int32>(v);
    }
    output = lookupTable.Get(idx);
  }
};
}
}
}
#endif
