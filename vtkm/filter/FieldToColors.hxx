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

#ifndef vtk_m_filter_Field_to_Colors_hxx
#define vtk_m_filter_Field_to_Colors_hxx

#include <vtkm/filter/FieldToColors.h>

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ColorTable.hxx>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/internal/CreateResult.h>


namespace vtkm
{
namespace filter
{

namespace
{
struct ScalarInputMode
{
};
struct MagnitudeInputMode
{
};
struct ComponentInputMode
{
};
}

template <typename T, typename S, typename U>
inline bool execute(const vtkm::cont::ColorTable& table,
                    ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagSingleComponent)
{
  return table.Map(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(const vtkm::cont::ColorTable& table,
                    MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagMultipleComponents)
{
  return table.MapMagnitude(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(const vtkm::cont::ColorTable& table,
                    ComponentInputMode,
                    int comp,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagMultipleComponents)
{
  return table.MapComponent(input, comp, samples, output);
}

//error cases
template <typename T, typename S, typename U>
inline bool execute(const vtkm::cont::ColorTable& table,
                    ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagMultipleComponents)
{ //vector input in scalar mode so do magnitude
  return table.MapMagnitude(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(const vtkm::cont::ColorTable& table,
                    MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore Magnitude mode
  return table.Map(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(const vtkm::cont::ColorTable& table,
                    ComponentInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore COMPONENT mode
  return table.Map(input, samples, output);
}


//-----------------------------------------------------------------------------
inline VTKM_CONT FieldToColors::FieldToColors(const vtkm::cont::ColorTable& table)
  : vtkm::filter::FilterField<FieldToColors>()
  , Table(table)
  , InputMode(SCALAR)
  , OutputMode(RGBA)
  , SamplesRGB()
  , SamplesRGBA()
  , Component(0)
  , SampleCount(256)
  , ModifiedCount(-1)
{
}

//-----------------------------------------------------------------------------
inline VTKM_CONT void FieldToColors::SetNumberOfSamplingPoints(vtkm::Int32 count)
{
  if (this->SampleCount != count && count > 0)
  {
    this->ModifiedCount = -1;
    this->SampleCount = count;
  }
}

//-----------------------------------------------------------------------------
template <typename T, typename StorageType, typename DerivedPolicy, typename DeviceAdapter>
inline VTKM_CONT vtkm::cont::DataSet FieldToColors::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& inField,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  const vtkm::filter::PolicyBase<DerivedPolicy>&,
  const DeviceAdapter&)
{
  //If the table has been modified we need to rebuild our
  //sample tables
  if (this->Table.GetModifiedCount() > this->ModifiedCount)
  {
    this->Table.Sample(this->SampleCount, this->SamplesRGB);
    this->Table.Sample(this->SampleCount, this->SamplesRGBA);
    this->ModifiedCount = this->Table.GetModifiedCount();
  }


  std::string outputName = this->GetOutputFieldName();
  if (outputName == "")
  {
    // Default name is name of input_colors.
    outputName = fieldMetadata.GetName() + "_colors";
  }
  vtkm::cont::Field outField;

  //We need to verify if the array is a vtkm::Vec

  using IsVec = typename vtkm::VecTraits<T>::HasMultipleComponents;
  if (this->OutputMode == RGBA)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>> output;

    bool ran = false;
    switch (this->InputMode)
    {
      case SCALAR:
      {
        ran = execute(this->Table,
                      ScalarInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGBA,
                      output,
                      IsVec{});
        break;
      }
      case MAGNITUDE:
      {
        ran = execute(this->Table,
                      MagnitudeInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGBA,
                      output,
                      IsVec{});
        break;
      }
      case COMPONENT:
      {
        ran = execute(this->Table,
                      ComponentInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGBA,
                      output,
                      IsVec{});
        break;
      }
    }

    if (!ran)
    {
      throw vtkm::cont::ErrorFilterExecution("Unsupported input mode.");
    }
    outField = vtkm::cont::Field(outputName, vtkm::cont::Field::Association::POINTS, output);
  }
  else
  {
    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 3>> output;

    bool ran = false;
    switch (this->InputMode)
    {
      case SCALAR:
      {
        ran = execute(this->Table,
                      ScalarInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGB,
                      output,
                      IsVec{});
        break;
      }
      case MAGNITUDE:
      {
        ran = execute(this->Table,
                      MagnitudeInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGB,
                      output,
                      IsVec{});
        break;
      }
      case COMPONENT:
      {
        ran = execute(this->Table,
                      ComponentInputMode{},
                      this->Component,
                      inField,
                      this->SamplesRGB,
                      output,
                      IsVec{});
        break;
      }
    }

    if (!ran)
    {
      throw vtkm::cont::ErrorFilterExecution("Unsupported input mode.");
    }
    outField = vtkm::cont::Field(outputName, vtkm::cont::Field::Association::POINTS, output);
  }


  return internal::CreateResult(input, outField);
}
}
} // namespace vtkm::filter

#endif //vtk_m_filter_Field_to_Colors_hxx
