//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_raytracing_Worklets_h
#define vtk_m_rendering_raytracing_Worklets_h
#include <vtkm/worklet/WorkletMapField.h>
namespace vtkm
{
namespace rendering
{
namespace raytracing
{
//
// Utility memory set functor
//
template <class T>
class MemSet : public vtkm::worklet::WorkletMapField
{
  T Value;

public:
  VTKM_CONT
  MemSet(T value)
    : Value(value)
  {
  }
  using ControlSignature = void(FieldOut);
  using ExecutionSignature = void(_1);
  VTKM_EXEC
  void operator()(T& outValue) const { outValue = Value; }
}; //class MemSet

template <typename FloatType>
class CopyAndOffset : public vtkm::worklet::WorkletMapField
{
  FloatType Offset;

public:
  VTKM_CONT
  CopyAndOffset(const FloatType offset = 0.00001)
    : Offset(offset)
  {
  }
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  VTKM_EXEC inline void operator()(const FloatType& inValue, FloatType& outValue) const
  {
    outValue = inValue + Offset;
  }
}; //class Copy and iffset

template <typename FloatType>
class CopyAndOffsetMask : public vtkm::worklet::WorkletMapField
{
  FloatType Offset;
  vtkm::UInt8 MaskValue;

public:
  VTKM_CONT
  CopyAndOffsetMask(const FloatType offset = 0.00001, const vtkm::UInt8 mask = 1)
    : Offset(offset)
    , MaskValue(mask)
  {
  }
  using ControlSignature = void(FieldIn, FieldInOut, FieldIn);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename MaskType>
  VTKM_EXEC inline void operator()(const FloatType& inValue,
                                   FloatType& outValue,
                                   const MaskType& mask) const
  {
    if (mask == MaskValue)
      outValue = inValue + Offset;
  }
}; //class Copy and iffset

template <class T>
class Mask : public vtkm::worklet::WorkletMapField
{
  T Value;

public:
  VTKM_CONT
  Mask(T value)
    : Value(value)
  {
  }
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename O>
  VTKM_EXEC void operator()(const T& inValue, O& outValue) const
  {
    if (inValue == Value)
      outValue = static_cast<O>(1);
    else
      outValue = static_cast<O>(0);
  }
}; //class mask

template <class T, int N>
class ManyMask : public vtkm::worklet::WorkletMapField
{
  vtkm::Vec<T, N> Values;

public:
  VTKM_CONT
  ManyMask(vtkm::Vec<T, N> values)
    : Values(values)
  {
  }
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2);

  template <typename O>
  VTKM_EXEC void operator()(const T& inValue, O& outValue) const
  {
    bool doMask = false;
    for (vtkm::Int32 i = 0; i < N; ++i)
    {
      if (inValue == Values[i])
        doMask = true;
    }
    if (doMask)
      outValue = static_cast<O>(1);
    else
      outValue = static_cast<O>(0);
  }
}; //class double mask

struct MaxValue
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return (a > b) ? a : b;
  }

}; //struct MaxValue

struct MinValue
{
  template <typename T>
  VTKM_EXEC_CONT T operator()(const T& a, const T& b) const
  {
    return (a < b) ? a : b;
  }

}; //struct MinValue
}
}
} //namespace vtkm::rendering::raytracing
#endif //vtk_m_rendering_raytracing_Worklets_h
