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
#ifndef vtk_m_cont_ArrayHandleVirtual_h
#define vtk_m_cont_ArrayHandleVirtual_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <vtkm/cont/StorageVirtual.h>

#include <memory>

namespace vtkm
{
namespace cont
{


template <typename T>
class ArrayHandleVirtual : public vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual>
{
  using StorageType = vtkm::cont::internal::Storage<T, vtkm::cont::StorageTagVirtual>;

public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleVirtual,
                             (ArrayHandleVirtual<T>),
                             (vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual>));

  ///Construct a valid ArrayHandleVirtual from an existing ArrayHandle
  ///that doesn't derive from ArrayHandleVirtual.
  ///Note left non-explicit to allow:
  ///
  /// std::vector<vtkm::cont::ArrayHandleVirtual<vtkm::Float64>> vectorOfArrays;
  /// //Make basic array.
  /// vtkm::cont::ArrayHandle<vtkm::Float64> basicArray;
  ///  //Fill basicArray...
  /// vectorOfArrays.push_back(basicArray);
  ///
  /// // Make fancy array.
  /// vtkm::cont::ArrayHandleCounting<vtkm::Float64> fancyArray(-1.0, 0.1, ARRAY_SIZE);
  /// vectorOfArrays.push_back(fancyArray);
  template <typename S>
  ArrayHandleVirtual(const vtkm::cont::ArrayHandle<T, S>& ah)
    : Superclass(StorageType(ah))
  {
    using is_base = std::is_base_of<StorageType, S>;
    static_assert(!is_base::value, "Wrong specialization for ArrayHandleVirtual selected");
  }

  /// Returns true if this array matches the type passed in.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT bool IsType() const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

    //We need to determine if we are checking that `ArrayHandleType`
    //is a virtual array handle since that is an easy check.
    //Or if we have to go ask the storage if they are holding
    //
    using ST = typename ArrayHandleType::StorageTag;
    using is_base = std::is_same<vtkm::cont::StorageTagVirtual, ST>;

    //Determine if the Value type of the virtual and ArrayHandleType
    //are the same. This an easy compile time check, and doesn't need
    // to be done at runtime.
    using VT = typename ArrayHandleType::ValueType;
    using same_value_type = std::is_same<T, VT>;

    return this->IsSameType<ArrayHandleType>(same_value_type{}, is_base{});
  }

  /// Returns this array cast to the given \c ArrayHandle type. Throws \c
  /// ErrorBadType if the cast does not work. Use \c IsType
  /// to check if the cast can happen.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT ArrayHandleType Cast() const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    //We need to determine if we are checking that `ArrayHandleType`
    //is a virtual array handle since that is an easy check.
    //Or if we have to go ask the storage if they are holding
    //
    using ST = typename ArrayHandleType::StorageTag;
    using is_base = std::is_same<vtkm::cont::StorageTagVirtual, ST>;

    //Determine if the Value type of the virtual and ArrayHandleType
    //are the same. This an easy compile time check, and doesn't need
    // to be done at runtime.
    using VT = typename ArrayHandleType::ValueType;
    using same_value_type = std::is_same<T, VT>;

    return this->CastToType<ArrayHandleType>(same_value_type{}, is_base{});
  }

  /// Returns a new instance of an ArrayHandleVirtual with the same storage
  ///
  VTKM_CONT ArrayHandleVirtual<T> NewInstance() const
  {
    return ArrayHandleVirtual<T>(this->GetStorage().NewInstance());
  }

private:
  template <typename ArrayHandleType>
  inline bool IsSameType(std::false_type, std::true_type) const
  {
    return false;
  }
  template <typename ArrayHandleType>
  inline bool IsSameType(std::false_type, std::false_type) const
  {
    return false;
  }

  template <typename ArrayHandleType>
  inline bool IsSameType(std::true_type vtkmNotUsed(valueTypesMatch),
                         std::true_type vtkmNotUsed(inheritsFromArrayHandleVirtual)) const
  {
    //The type being past has no requirements in being the most derived type
    //so the typeid info won't match but dynamic_cast will work
    auto casted = dynamic_cast<const ArrayHandleType*>(this);
    return casted != nullptr;
  }

  template <typename ArrayHandleType>
  inline bool IsSameType(std::true_type vtkmNotUsed(valueTypesMatch),
                         std::false_type vtkmNotUsed(notFromArrayHandleVirtual)) const
  {
    auto* storage = this->GetStorage().GetStorageVirtual();
    if (!storage)
    {
      return false;
    }
    using S = typename ArrayHandleType::StorageTag;
    return storage->template IsType<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
  }

  template <typename ArrayHandleType>
  inline ArrayHandleType CastToType(std::false_type vtkmNotUsed(valueTypesMatch),
                                    std::true_type vtkmNotUsed(notFromArrayHandleVirtual)) const
  {
    VTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
    throwFailedDynamicCast("ArrayHandleVirtual", vtkm::cont::TypeToString<ArrayHandleType>());
    return ArrayHandleType{};
  }

  template <typename ArrayHandleType>
  inline ArrayHandleType CastToType(std::false_type vtkmNotUsed(valueTypesMatch),
                                    std::false_type vtkmNotUsed(notFromArrayHandleVirtual)) const
  {
    VTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
    throwFailedDynamicCast("ArrayHandleVirtual", vtkm::cont::TypeToString<ArrayHandleType>());
    return ArrayHandleType{};
  }

  template <typename ArrayHandleType>
  inline ArrayHandleType CastToType(
    std::true_type vtkmNotUsed(valueTypesMatch),
    std::true_type vtkmNotUsed(inheritsFromArrayHandleVirtual)) const
  {
    //The type being passed has no requirements in being the most derived type
    //so the typeid info won't match but dynamic_cast will work
    const ArrayHandleType* derived = dynamic_cast<const ArrayHandleType*>(this);
    if (!derived)
    {
      VTKM_LOG_CAST_FAIL(*this, ArrayHandleType);
      throwFailedDynamicCast("ArrayHandleVirtual", vtkm::cont::TypeToString<ArrayHandleType>());
    }
    VTKM_LOG_CAST_SUCC(*this, derived);
    return *derived;
  }

  template <typename ArrayHandleType>
  ArrayHandleType CastToType(std::true_type vtkmNotUsed(valueTypesMatch),
                             std::false_type vtkmNotUsed(notFromArrayHandleVirtual)) const;
};


//=============================================================================
/// A convenience function for creating an ArrayHandleVirtual.
template <typename T, typename S>
VTKM_CONT vtkm::cont::ArrayHandleVirtual<T> make_ArrayHandleVirtual(
  const vtkm::cont::ArrayHandle<T, S>& ah)
{
  return vtkm::cont::ArrayHandleVirtual<T>(ah);
}

//=============================================================================
// Free function casting helpers

/// Returns true if \c virtHandle matches the type of ArrayHandleType.
///
template <typename ArrayHandleType, typename T>
VTKM_CONT inline bool IsType(const vtkm::cont::ArrayHandleVirtual<T>& virtHandle)
{
  return virtHandle.template IsType<ArrayHandleType>();
}

/// Returns \c virtHandle cast to the given \c ArrayHandle type. Throws \c
/// ErrorBadType if the cast does not work. Use \c IsType
/// to check if the cast can happen.
///
template <typename ArrayHandleType, typename T>
VTKM_CONT inline ArrayHandleType Cast(const vtkm::cont::ArrayHandleVirtual<T>& virtHandle)
{
  return virtHandle.template Cast<ArrayHandleType>();
}

//=============================================================================
// Specializations of CastAndCall to help make sure ArrayHandleVirtual
// holding a ArrayHandleUniformPointCoordinates works properly
template <typename Functor, typename... Args>
void CastAndCall(vtkm::cont::ArrayHandleVirtual<vtkm::Vec<vtkm::FloatDefault, 3>> coords,
                 Functor&& f,
                 Args&&... args)
{
  using HandleType = ArrayHandleUniformPointCoordinates;
  using T = typename HandleType::ValueType;
  using S = typename HandleType::StorageTag;
  if (coords.IsType<HandleType>())
  {
    const vtkm::cont::internal::detail::StorageVirtual* storage =
      coords.GetStorage().GetStorageVirtual();
    auto* virtualImpl = storage->Cast<vtkm::cont::internal::detail::StorageVirtualImpl<T, S>>();
    f(virtualImpl->GetHandle(), std::forward<Args>(args)...);
  }
  else
  {
    f(coords, std::forward<Args>(args)...);
  }
}



//=============================================================================
// Specializations of serialization related classes
template <typename T>
struct SerializableTypeString<vtkm::cont::ArrayHandleVirtual<T>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "AH_Virtual<" + SerializableTypeString<T>::Get() + ">";
    return name;
  }
};
}
} //namespace vtkm::cont


#endif //vtk_m_cont_ArrayHandleVirtual_h
