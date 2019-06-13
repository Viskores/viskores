//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_io_internal_VTKDataSetTypes_h
#define vtk_m_io_internal_VTKDataSetTypes_h

#include <vtkm/Types.h>

#include <algorithm>
#include <cassert>
#include <string>

namespace vtkm
{
namespace io
{
namespace internal
{

enum DataType
{
  DTYPE_UNKNOWN = 0,
  DTYPE_BIT,
  DTYPE_UNSIGNED_CHAR,
  DTYPE_CHAR,
  DTYPE_UNSIGNED_SHORT,
  DTYPE_SHORT,
  DTYPE_UNSIGNED_INT,
  DTYPE_INT,
  DTYPE_UNSIGNED_LONG,
  DTYPE_LONG,
  DTYPE_FLOAT,
  DTYPE_DOUBLE
};

inline const char* DataTypeString(int id)
{
  static const char* strings[] = {
    "",    "bit",           "unsigned_char", "char",  "unsigned_short", "short", "unsigned_int",
    "int", "unsigned_long", "long",          "float", "double"
  };
  return strings[id];
}

inline DataType DataTypeId(const std::string& str)
{
  DataType type = DTYPE_UNKNOWN;
  for (int id = 1; id < 12; ++id)
  {
    if (str == DataTypeString(id))
    {
      type = static_cast<DataType>(id);
    }
  }

  return type;
}

struct DummyBitType
{
  // Needs to work with streams' << operator
  operator bool() const { return false; }
};

class ColorChannel8
{
public:
  ColorChannel8()
    : Data()
  {
  }
  ColorChannel8(vtkm::UInt8 val)
    : Data(val)
  {
  }
  ColorChannel8(vtkm::Float32 val)
    : Data(static_cast<vtkm::UInt8>(std::min(std::max(val, 1.0f), 0.0f) * 255))
  {
  }
  operator vtkm::Float32() const { return static_cast<vtkm::Float32>(this->Data) / 255.0f; }
  operator vtkm::UInt8() const { return this->Data; }

private:
  vtkm::UInt8 Data;
};

inline std::ostream& operator<<(std::ostream& out, const ColorChannel8& val)
{
  return out << static_cast<vtkm::Float32>(val);
}

inline std::istream& operator>>(std::istream& in, ColorChannel8& val)
{
  vtkm::Float32 fval;
  in >> fval;
  val = ColorChannel8(fval);
  return in;
}

template <typename T>
struct DataTypeName
{
  static const char* Name() { return "unknown"; }
};
template <>
struct DataTypeName<DummyBitType>
{
  static const char* Name() { return "bit"; }
};
template <>
struct DataTypeName<vtkm::Int8>
{
  static const char* Name() { return "char"; }
};
template <>
struct DataTypeName<vtkm::UInt8>
{
  static const char* Name() { return "unsigned_char"; }
};
template <>
struct DataTypeName<vtkm::Int16>
{
  static const char* Name() { return "short"; }
};
template <>
struct DataTypeName<vtkm::UInt16>
{
  static const char* Name() { return "unsigned_short"; }
};
template <>
struct DataTypeName<vtkm::Int32>
{
  static const char* Name() { return "int"; }
};
template <>
struct DataTypeName<vtkm::UInt32>
{
  static const char* Name() { return "unsigned_int"; }
};
template <>
struct DataTypeName<vtkm::Int64>
{
  static const char* Name() { return "long"; }
};
template <>
struct DataTypeName<vtkm::UInt64>
{
  static const char* Name() { return "unsigned_long"; }
};
template <>
struct DataTypeName<vtkm::Float32>
{
  static const char* Name() { return "float"; }
};
template <>
struct DataTypeName<vtkm::Float64>
{
  static const char* Name() { return "double"; }
};

template <typename T, typename Functor>
inline void SelectVecTypeAndCall(T, vtkm::IdComponent numComponents, const Functor& functor)
{
  switch (numComponents)
  {
    case 1:
      functor(T());
      break;
    case 2:
      functor(vtkm::Vec<T, 2>());
      break;
    case 3:
      functor(vtkm::Vec<T, 3>());
      break;
    case 4:
      functor(vtkm::Vec<T, 4>());
      break;
    case 9:
      functor(vtkm::Vec<T, 9>());
      break;
    default:
      functor(numComponents, T());
      break;
  }
}

template <typename Functor>
inline void SelectTypeAndCall(DataType dtype,
                              vtkm::IdComponent numComponents,
                              const Functor& functor)
{
  switch (dtype)
  {
    case DTYPE_BIT:
      SelectVecTypeAndCall(DummyBitType(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_CHAR:
      SelectVecTypeAndCall(vtkm::UInt8(), numComponents, functor);
      break;
    case DTYPE_CHAR:
      SelectVecTypeAndCall(vtkm::Int8(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_SHORT:
      SelectVecTypeAndCall(vtkm::UInt16(), numComponents, functor);
      break;
    case DTYPE_SHORT:
      SelectVecTypeAndCall(vtkm::Int16(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_INT:
      SelectVecTypeAndCall(vtkm::UInt32(), numComponents, functor);
      break;
    case DTYPE_INT:
      SelectVecTypeAndCall(vtkm::Int32(), numComponents, functor);
      break;
    case DTYPE_UNSIGNED_LONG:
      SelectVecTypeAndCall(vtkm::UInt64(), numComponents, functor);
      break;
    case DTYPE_LONG:
      SelectVecTypeAndCall(vtkm::Int64(), numComponents, functor);
      break;
    case DTYPE_FLOAT:
      SelectVecTypeAndCall(vtkm::Float32(), numComponents, functor);
      break;
    case DTYPE_DOUBLE:
      SelectVecTypeAndCall(vtkm::Float64(), numComponents, functor);
      break;
    default:
      assert(false);
  }
}
}
}
} // namespace vtkm::io::internal

#endif // vtk_m_io_internal_VTKDataSetTypes_h
