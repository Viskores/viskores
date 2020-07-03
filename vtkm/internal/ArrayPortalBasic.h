//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_ArrayPortalBasic_h
#define vtk_m_internal_ArrayPortalBasic_h

#include <vtkm/Assert.h>
#include <vtkm/Types.h>

#ifdef VTKM_CUDA
// CUDA devices have special instructions for faster data loading
#include <vtkm/exec/cuda/internal/ArrayPortalBasicCuda.h>
#endif // VTKM_CUDA

namespace vtkm
{
namespace internal
{

namespace detail
{

// These templated methods can be overloaded for special access to data.

template <typename T>
VTKM_EXEC_CONT static inline T ArrayPortalBasicReadGet(const T* const data)
{
  return *data;
}

template <typename T>
VTKM_EXEC_CONT static inline T ArrayPortalBasicWriteGet(const T* const data)
{
  return *data;
}

template <typename T>
VTKM_EXEC_CONT static inline void ArrayPortalBasicWriteSet(T* data, const T& value)
{
  *data = value;
}

} // namespace detail

template <typename T>
class ArrayPortalBasicRead
{
  const T* Array = nullptr;
  vtkm::Id NumberOfValues = 0;

public:
  using ValueType = T;

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->NumberOfValues);

    return detail::ArrayPortalBasicReadGet(this->Array + index);
  }

  VTKM_EXEC_CONT const ValueType* GetIteratorBegin() const { return this->Array; }
  VTKM_EXEC_CONT const ValueType* GetIteratorEnd() const
  {
    return this->Array + this->NumberOfValues;
  }

  VTKM_EXEC_CONT const ValueType* GetArray() const { return this->Array; }

  ArrayPortalBasicRead() = default;
  ArrayPortalBasicRead(ArrayPortalBasicRead&&) = default;
  ArrayPortalBasicRead(const ArrayPortalBasicRead&) = default;
  ArrayPortalBasicRead& operator=(ArrayPortalBasicRead&&) = default;
  ArrayPortalBasicRead& operator=(const ArrayPortalBasicRead&) = default;

  VTKM_EXEC_CONT ArrayPortalBasicRead(const T* array, vtkm::Id numberOfValues)
    : Array(array)
    , NumberOfValues(numberOfValues)
  {
  }
};

template <typename T>
class ArrayPortalBasicWrite
{
  T* Array = nullptr;
  vtkm::Id NumberOfValues = 0;

public:
  using ValueType = T;

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id index) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->NumberOfValues);

    return detail::ArrayPortalBasicWriteGet(this->Array + index);
  }

  VTKM_EXEC_CONT void Set(vtkm::Id index, const ValueType& value) const
  {
    VTKM_ASSERT(index >= 0);
    VTKM_ASSERT(index < this->NumberOfValues);

    detail::ArrayPortalBasicWriteSet(this->Array + index, value);
  }

  VTKM_EXEC_CONT ValueType* GetIteratorBegin() const { return this->Array; }
  VTKM_EXEC_CONT ValueType* GetIteratorEnd() const { return this->Array + this->NumberOfValues; }

  VTKM_EXEC_CONT ValueType* GetArray() const { return this->Array; }

  ArrayPortalBasicWrite() = default;
  ArrayPortalBasicWrite(ArrayPortalBasicWrite&&) = default;
  ArrayPortalBasicWrite(const ArrayPortalBasicWrite&) = default;
  ArrayPortalBasicWrite& operator=(ArrayPortalBasicWrite&&) = default;
  ArrayPortalBasicWrite& operator=(const ArrayPortalBasicWrite&) = default;

  VTKM_EXEC_CONT ArrayPortalBasicWrite(T* array, vtkm::Id numberOfValues)
    : Array(array)
    , NumberOfValues(numberOfValues)
  {
  }
};
}
} // namespace vtkm::internal

#endif //vtk_m_internal_ArrayPortalBasic_h
