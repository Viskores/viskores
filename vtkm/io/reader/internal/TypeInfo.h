//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_io_readers_internal_TypeInfo_h
#define vtk_m_io_readers_internal_TypeInfo_h

#include <vtkm/Types.h>

#include <cassert>
#include <string>

namespace vtkm {
namespace io {
namespace reader {
namespace internal {

enum DataSetType
{
  DATASET_UNKNOWN = 0,
  DATASET_STRUCTURED_POINTS,
  DATASET_STRUCTURED_GRID,
  DATASET_UNSTRUCTURED_GRID,
  DATASET_POLYDATA,
  DATASET_RECTILINEAR_GRID,
  DATASET_FIELD
};

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

inline const char* DataSetTypeString(int id)
{
  static const char *strings[] = {
    "",
    "STRUCTURED_POINTS",
    "STRUCTURED_GRID",
    "UNSTRUCTURED_GRID",
    "POLYDATA",
    "RECTILINEAR_GRID",
    "FIELD"
  };
  return strings[id];
}

inline DataSetType DataSetTypeId(const std::string &str)
{
  DataSetType type = DATASET_UNKNOWN;
  for (int id = 1; id < 7; ++id)
  {
    if (str == DataSetTypeString(id))
    {
      type = static_cast<DataSetType>(id);
    }
  }

  return type;
}

inline const char* DataTypeString(int id)
{
  static const char *strings[] = {
    "",
    "bit",
    "unsigned_char",
    "char",
    "unsigned_short",
    "short",
    "unsigned_int",
    "int",
    "unsigned_long",
    "long",
    "float",
    "double"
  };
  return strings[id];
}

inline DataType DataTypeId(const std::string &str)
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
  operator bool() const
  {
    return false;
  }
};

template <typename T, typename Functor>
inline void SelectVecTypeAndCall(T, vtkm::IdComponent numComponents, const Functor &functor)
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
inline void SelectTypeAndCall(DataType dtype, vtkm::IdComponent numComponents,
                        const Functor &functor)
{
  switch (dtype)
  {
  case DTYPE_BIT:
    SelectVecTypeAndCall(DummyBitType(), numComponents, functor);
    break;
  case DTYPE_UNSIGNED_CHAR:
    SelectVecTypeAndCall(vtkm::Int8(), numComponents, functor);
    break;
  case DTYPE_CHAR:
    SelectVecTypeAndCall(vtkm::UInt8(), numComponents, functor);
    break;
  case DTYPE_UNSIGNED_SHORT:
    SelectVecTypeAndCall(vtkm::Int16(), numComponents, functor);
    break;
  case DTYPE_SHORT:
    SelectVecTypeAndCall(vtkm::UInt16(), numComponents, functor);
    break;
  case DTYPE_UNSIGNED_INT:
    SelectVecTypeAndCall(vtkm::Int32(), numComponents, functor);
    break;
  case DTYPE_INT:
    SelectVecTypeAndCall(vtkm::UInt32(), numComponents, functor);
    break;
  case DTYPE_UNSIGNED_LONG:
    SelectVecTypeAndCall(vtkm::Int64(), numComponents, functor);
    break;
  case DTYPE_LONG:
    SelectVecTypeAndCall(vtkm::UInt64(), numComponents, functor);
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
}
} // namespace vtkm::io:reader::internal

#endif // vtk_m_io_readers_internal_TypeInfo_h
