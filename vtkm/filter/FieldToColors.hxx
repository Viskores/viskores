//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_filter_FieldToColors_hxx
#define vtk_m_filter_FieldToColors_hxx

#include <vtkm/VecTraits.h>
#include <vtkm/cont/ColorTable.hxx>
#include <vtkm/cont/ErrorFilterExecution.h>

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
template <typename T, typename StorageType, typename DerivedPolicy>
inline VTKM_CONT vtkm::cont::DataSet FieldToColors::DoExecute(
  const vtkm::cont::DataSet& input,
  const vtkm::cont::ArrayHandle<T, StorageType>& inField,
  const vtkm::filter::FieldMetadata& fieldMetadata,
  vtkm::filter::PolicyBase<DerivedPolicy>)
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
  if (outputName.empty())
  {
    // Default name is name of input_colors.
    outputName = fieldMetadata.GetName() + "_colors";
  }
  vtkm::cont::Field outField;

  //We need to verify if the array is a vtkm::Vec

  using IsVec = typename vtkm::VecTraits<T>::HasMultipleComponents;
  if (this->OutputMode == RGBA)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> output;

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
    outField = vtkm::cont::make_FieldPoint(outputName, output);
  }
  else
  {
    vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> output;

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
    outField = vtkm::cont::make_FieldPoint(outputName, output);
  }


  return CreateResult(input, outField);
}
}
} // namespace vtkm::filter

#endif
