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
#ifndef vtk_m_cont_TypeString_h
#define vtk_m_cont_TypeString_h

#include <vtkm/Types.h>

#include <string>

namespace vtkm
{
namespace cont
{

/// \brief A traits class that gives a unique name for a type. This class
/// should be specialized for every type that has to be serialized by diy.
template <typename T>
struct TypeString
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
std::string GetVariadicTypeString(const T&, const Ts&... ts)
{
  return TypeString<T>::Get() + "," + GetVariadicTypeString(ts...);
}

template <typename T>
std::string GetVariadicTypeString(const T&)
{
  return TypeString<T>::Get();
}

} // internal

template <>
struct TypeString<vtkm::Int8>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "I8";
    return name;
  }
};

template <>
struct TypeString<vtkm::UInt8>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "U8";
    return name;
  }
};

template <>
struct TypeString<vtkm::Int16>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "I16";
    return name;
  }
};

template <>
struct TypeString<vtkm::UInt16>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "U16";
    return name;
  }
};

template <>
struct TypeString<vtkm::Int32>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "I32";
    return name;
  }
};

template <>
struct TypeString<vtkm::UInt32>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "U32";
    return name;
  }
};

template <>
struct TypeString<vtkm::Int64>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "I64";
    return name;
  }
};

template <>
struct TypeString<vtkm::UInt64>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "U64";
    return name;
  }
};

template <>
struct TypeString<vtkm::Float32>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "F32";
    return name;
  }
};

template <>
struct TypeString<vtkm::Float64>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "F64";
    return name;
  }
};

template <typename T, vtkm::IdComponent NumComponents>
struct TypeString<vtkm::Vec<T, NumComponents>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "V<" + TypeString<T>::Get() + "," + std::to_string(NumComponents) + ">";
    return name;
  }
};

template <typename T1, typename T2>
struct TypeString<vtkm::Pair<T1, T2>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name =
      "vtkm::Pair<" + TypeString<T1>::Get() + "," + TypeString<T2>::Get() + ">";
    return name;
  }
};
}
} // vtkm::cont

#endif // vtk_m_cont_TypeString_h
