//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.md for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_c_ErrorCode_h
#define vtk_c_ErrorCode_h

#include <vtkc/internal/Config.h>

#include <cstdint>

namespace vtkc
{

enum class ErrorCode : std::int32_t
{
  SUCCESS = 0,
  INVALID_SHAPE_ID,
  INVALID_NUMBER_OF_POINTS,
  WRONG_SHAPE_ID_FOR_TAG_TYPE,
  INVALID_POINT_ID,
  SOLUTION_DID_NOT_CONVERGE,
  MATRIX_LUP_FACTORIZATION_FAILED,
  DEGENERATE_CELL_DETECTED
};

VTKC_EXEC
inline const char* errorString(ErrorCode code) noexcept
{
  switch (code)
  {
    case ErrorCode::SUCCESS:
      return "Success";
    case ErrorCode::INVALID_SHAPE_ID:
      return "Invalid shape id";
    case ErrorCode::INVALID_NUMBER_OF_POINTS:
      return "Invalid number of points";
    case ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE:
      return "Wrong shape id for tag type";
    case ErrorCode::INVALID_POINT_ID:
      return "Invalid point id";
    case ErrorCode::SOLUTION_DID_NOT_CONVERGE:
      return "Solution did not converge";
    case ErrorCode::MATRIX_LUP_FACTORIZATION_FAILED:
      return "LUP factorization failed";
    case ErrorCode::DEGENERATE_CELL_DETECTED:
      return "Degenerate cell detected";
  }

  return "Invalid error";
}

} // vtkc

#define VTKC_RETURN_ON_ERROR(call)                                                                 \
  {                                                                                                \
    auto status = call;                                                                            \
    if (status != vtkc::ErrorCode::SUCCESS)                                                        \
    {                                                                                              \
      return status;                                                                               \
    }                                                                                              \
  }

#endif // vtk_c_ErrorCode_h
