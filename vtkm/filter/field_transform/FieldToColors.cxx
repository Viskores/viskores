//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/VecTraits.h>
#include <vtkm/cont/ColorTableMap.h>
#include <vtkm/cont/ErrorFilterExecution.h>
#include <vtkm/filter/field_transform/FieldToColors.h>

namespace vtkm
{
namespace filter
{
namespace field_transform
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
inline bool execute(ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagSingleComponent)
{
  return vtkm::cont::ColorTableMap(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagMultipleComponents)
{
  return vtkm::cont::ColorTableMapMagnitude(input, samples, output);
}

template <typename T, typename S, typename U>
inline bool execute(ComponentInputMode,
                    int comp,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagMultipleComponents)
{
  return vtkm::cont::ColorTableMapComponent(input, comp, samples, output);
}

//error cases
template <typename T, typename S, typename U>
inline bool execute(ScalarInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagMultipleComponents)
{ //vector input in scalar mode so do magnitude
  return vtkm::cont::ColorTableMapMagnitude(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(MagnitudeInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore Magnitude mode
  return vtkm::cont::ColorTableMap(input, samples, output);
}
template <typename T, typename S, typename U>
inline bool execute(ComponentInputMode,
                    int,
                    const T& input,
                    const S& samples,
                    U& output,
                    vtkm::VecTraitsTagSingleComponent)
{ //is a scalar array so ignore InputMode::Component
  return vtkm::cont::ColorTableMap(input, samples, output);
}


//-----------------------------------------------------------------------------
VTKM_CONT FieldToColors::FieldToColors(const vtkm::cont::ColorTable& table)
  : Table(table)

{
}

//-----------------------------------------------------------------------------
VTKM_CONT void FieldToColors::SetNumberOfSamplingPoints(vtkm::Int32 count)
{
  if (this->SampleCount != count && count > 0)
  {
    this->ModifiedCount = -1;
    this->SampleCount = count;
  }
}

//-----------------------------------------------------------------------------
vtkm::cont::DataSet FieldToColors::DoExecute(const vtkm::cont::DataSet& input)
{
  const auto& field = this->GetFieldFromDataSet(input);

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
    outputName = field.GetName() + "_colors";
  }
  vtkm::cont::Field outField;

  //We need to verify if the array is a vtkm::Vec
  vtkm::cont::UnknownArrayHandle outArray;
  auto resolveType = [&](const auto& concrete) {
    // use std::decay to remove const ref from the decltype of concrete.
    using T = typename std::decay_t<decltype(concrete)>::ValueType;
    using IsVec = typename vtkm::VecTraits<T>::HasMultipleComponents;

    if (this->OutputModeType == OutputMode::RGBA)
    {
      vtkm::cont::ArrayHandle<vtkm::Vec4ui_8> result;

      bool ran = false;
      switch (this->InputModeType)
      {
        case InputMode::Scalar:
        {
          ran = execute(
            ScalarInputMode{}, this->Component, concrete, this->SamplesRGBA, result, IsVec{});
          break;
        }
        case InputMode::Magnitude:
        {
          ran = execute(
            MagnitudeInputMode{}, this->Component, concrete, this->SamplesRGBA, result, IsVec{});
          break;
        }
        case InputMode::Component:
        {
          ran = execute(
            ComponentInputMode{}, this->Component, concrete, this->SamplesRGBA, result, IsVec{});
          break;
        }
      }

      if (!ran)
      {
        throw vtkm::cont::ErrorFilterExecution("Unsupported input mode.");
      }
      outField = vtkm::cont::make_FieldPoint(outputName, result);
    }
    else
    {
      vtkm::cont::ArrayHandle<vtkm::Vec3ui_8> result;

      bool ran = false;
      switch (this->InputModeType)
      {
        case InputMode::Scalar:
        {
          ran = execute(
            ScalarInputMode{}, this->Component, concrete, this->SamplesRGB, result, IsVec{});
          break;
        }
        case InputMode::Magnitude:
        {
          ran = execute(
            MagnitudeInputMode{}, this->Component, concrete, this->SamplesRGB, result, IsVec{});
          break;
        }
        case InputMode::Component:
        {
          ran = execute(
            ComponentInputMode{}, this->Component, concrete, this->SamplesRGB, result, IsVec{});
          break;
        }
      }

      if (!ran)
      {
        throw vtkm::cont::ErrorFilterExecution("Unsupported input mode.");
      }
      outField = vtkm::cont::make_FieldPoint(outputName, result);
    }
  };
  field.GetData()
    .CastAndCallForTypesWithFloatFallback<vtkm::TypeListField, VTKM_DEFAULT_STORAGE_LIST>(
      resolveType);

  return this->CreateResultField(input, outField);
}
} // namespace field_transform
} // namespace filter
} // namespace vtkm
