//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_SerializableTypeString_h
#define vtk_m_cont_SerializableTypeString_h

#include <vtkm/Types.h>

#include <string>

namespace vtkm
{
namespace cont
{

/// \brief A traits class that gives a unique name for a type. This class
/// should be specialized for every type that has to be serialized by diy.
template <typename T>
struct SerializableTypeString
#ifdef VTKM_DOXYGEN_ONLY
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "TypeName";
    return name;
  }
}
#endif
;

namespace internal
{

template <typename T, typename... Ts>
std::string GetVariadicSerializableTypeString(const T&, const Ts&... ts)
{
  return SerializableTypeString<T>::Get() + "," + GetVariadicSerializableTypeString(ts...);
}

template <typename T>
std::string GetVariadicSerializableTypeString(const T&)
{
  return SerializableTypeString<T>::Get();
}

template <typename T>
std::string IntTypeString()
{
  return (std::is_signed<T>::value ? "I" : "U") + std::to_string(sizeof(T) * 8);
}

} // internal

/// @cond SERIALIZATION
#define VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(int_type)                              \
  template <>                                                                          \
  struct SerializableTypeString<int_type>                                              \
  {                                                                                    \
    static VTKM_CONT std::string Get() { return internal::IntTypeString<int_type>(); } \
  }

VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(char);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(signed char);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(unsigned char);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(signed short);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(unsigned short);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(signed int);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(unsigned int);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(signed long);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(unsigned long);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(signed long long);
VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL(unsigned long long);

#undef VTK_M_SERIALIZABLE_INT_TYPE_STRING_IMPL

template <>
struct SerializableTypeString<vtkm::Float32>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "F32";
    return name;
  }
};

template <>
struct SerializableTypeString<vtkm::Float64>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "F64";
    return name;
  }
};

template <typename T, vtkm::IdComponent NumComponents>
struct SerializableTypeString<vtkm::Vec<T, NumComponents>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "V<" + SerializableTypeString<T>::Get() + "," + std::to_string(NumComponents) + ">";
    return name;
  }
};

template <typename T1, typename T2>
struct SerializableTypeString<vtkm::Pair<T1, T2>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "vtkm::Pair<" + SerializableTypeString<T1>::Get() + "," +
      SerializableTypeString<T2>::Get() + ">";
    return name;
  }
};
}
} // vtkm::cont
/// @endcond SERIALIZATION

#endif // vtk_m_cont_SerializableTypeString_h
