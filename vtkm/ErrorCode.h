//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_ErrorCode_h
#define vtk_m_exec_ErrorCode_h

#include <lcl/ErrorCode.h>

#include <vtkm/internal/ExportMacros.h>
namespace vtkm
{

enum class ErrorCode
{
  Success,
  InvalidShapeId,
  InvalidNumberOfPoints,
  InvalidCellMetric,
  WrongShapeIdForTagType,
  InvalidPointId,
  InvalidEdgeId,
  InvalidFaceId,
  SolutionDidNotConverge,
  MatrixFactorizationFailed,
  DegenerateCellDetected,
  MalformedCellDetected,
  OperationOnEmptyCell,
  CellNotFound,

  UnknownError
};

VTKM_EXEC_CONT
inline const char* ErrorString(vtkm::ErrorCode code) noexcept
{
  switch (code)
  {
    case vtkm::ErrorCode::Success:
      return "Success";
    case vtkm::ErrorCode::InvalidShapeId:
      return "Invalid shape id";
    case vtkm::ErrorCode::InvalidNumberOfPoints:
      return "Invalid number of points";
    case vtkm::ErrorCode::InvalidCellMetric:
      return "Invalid cell metric";
    case vtkm::ErrorCode::WrongShapeIdForTagType:
      return "Wrong shape id for tag type";
    case vtkm::ErrorCode::InvalidPointId:
      return "Invalid point id";
    case vtkm::ErrorCode::InvalidEdgeId:
      return "Invalid edge id";
    case vtkm::ErrorCode::InvalidFaceId:
      return "Invalid face id";
    case vtkm::ErrorCode::SolutionDidNotConverge:
      return "Solution did not converge";
    case vtkm::ErrorCode::MatrixFactorizationFailed:
      return "Matrix factorization failed";
    case vtkm::ErrorCode::DegenerateCellDetected:
      return "Degenerate cell detected";
    case vtkm::ErrorCode::MalformedCellDetected:
      return "Malformed cell detected";
    case vtkm::ErrorCode::OperationOnEmptyCell:
      return "Operation on empty cell";
    case vtkm::ErrorCode::CellNotFound:
      return "Cell not found";
    case vtkm::ErrorCode::UnknownError:
      return "Unknown error";
  }

  return "Invalid error";
}

namespace internal
{

VTKM_EXEC_CONT inline vtkm::ErrorCode LclErrorToVtkmError(lcl::ErrorCode code) noexcept
{
  switch (code)
  {
    case lcl::ErrorCode::SUCCESS:
      return vtkm::ErrorCode::Success;
    case lcl::ErrorCode::INVALID_SHAPE_ID:
      return vtkm::ErrorCode::InvalidShapeId;
    case lcl::ErrorCode::INVALID_NUMBER_OF_POINTS:
      return vtkm::ErrorCode::InvalidNumberOfPoints;
    case lcl::ErrorCode::WRONG_SHAPE_ID_FOR_TAG_TYPE:
      return vtkm::ErrorCode::WrongShapeIdForTagType;
    case lcl::ErrorCode::INVALID_POINT_ID:
      return vtkm::ErrorCode::InvalidPointId;
    case lcl::ErrorCode::SOLUTION_DID_NOT_CONVERGE:
      return vtkm::ErrorCode::SolutionDidNotConverge;
    case lcl::ErrorCode::MATRIX_LUP_FACTORIZATION_FAILED:
      return vtkm::ErrorCode::MatrixFactorizationFailed;
    case lcl::ErrorCode::DEGENERATE_CELL_DETECTED:
      return vtkm::ErrorCode::DegenerateCellDetected;
  }

  return vtkm::ErrorCode::UnknownError;
}

} // namespace internal

} // namespace vtkm

#define VTKM_RETURN_ON_ERROR(call)            \
  do                                          \
  {                                           \
    auto status = (call);                     \
    if (status != ::vtkm::ErrorCode::Success) \
    {                                         \
      return status;                          \
    }                                         \
  } while (false)

#endif //vtk_m_exec_ErrorCode_h
