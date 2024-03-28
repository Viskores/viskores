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

/// @brief Identifies whether an operation was successful or what type of error it had.
///
/// Most errors in VTK-m are reported by throwing an exception. However, there are
/// some places, most notably the execution environment, where it is not possible
/// to throw an exception. For those cases, it is typical for a function to return
/// an `ErrorCode` identifier. The calling code can check to see if the operation was
/// a success or what kind of error was encountered otherwise.
///
/// Use the `vtkm::ErrorString()` function to get a descriptive string of the error type.
enum class ErrorCode
{
  // Documentation is below (for better layout in generated documents).
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

/// @var ErrorCode Success
/// @brief A successful operation.
///
/// This code is returned when the operation was successful. Calling code
/// should check the error code against this identifier when checking the
/// status.

/// @var ErrorCode InvalidShapeId
/// @brief A unknown shape identifier was encountered.
///
/// All cell shapes must be listed in `vtkm::CellShapeIdEnum`.

/// @var ErrorCode InvalidNumberOfPoints
/// @brief The wrong number of points was provided for a given cell type.
///
/// For example, if a triangle has 4 points associated with it, you are
/// likely to get this error.

/// @var ErrorCode InvalidCellMetric
/// @brief A cell metric was requested for a cell that does not support that metric.

/// @var ErrorCode WrongShapeIdForTagType
/// This is an internal error from the lightweight cell library.

/// @var ErrorCode InvalidPointId
/// @brief A bad point identifier was detected while operating on a cell.

/// @var ErrorCode InvalidEdgeId
/// @brief A bad edge identifier was detected while operating on a cell.

/// @var ErrorCode InvalidFaceId
/// @brief A bad face identifier was detected while operating on a cell.

/// @var ErrorCode SolutionDidNotConverge
/// @brief An iterative operation did not find an appropriate solution.
///
/// This error code might be returned with some results of an iterative
/// solution. However, solution did not appear to resolve, so the results
/// might not be accurate.

/// @var ErrorCode MatrixFactorizationFailed
/// @brief A solution was not found for a linear system.
///
/// Some VTK-m computations use linear algebra to solve a system of equations.
/// If the equations does not give a valid result, this error can be returned.

/// @var ErrorCode DegenerateCellDetected
/// @brief An operation detected a degenerate cell.
///
/// A degenerate cell has two or more vertices combined into one, which
/// changes the structure of the cell. For example, if 2 vertices of
/// a tetrahedron are at the same point, the cell degenerates to a
/// triangle. Degenerate cells have the potential to interfere with some
/// computations on cells.

/// @var ErrorCode MalformedCellDetected
/// @brief An operation detected on a malformed cell.
///
/// Most cell shapes have some assumptions about their geometry (e.g. not
/// self intersecting). If an operation detects an expected behavior is
/// violated, this error is returned. (Note that `vtkm::DegenerateCellDetected`
/// has its own error coe.)

/// @var ErrorCode OperationOnEmptyCell
/// @brief An operation was attempted on a cell with an empty shape.
///
/// There is a special "empty" cell shape type (`vtkm::CellShapeTagEmpty`) that
/// can be used as a placeholder for a cell with no information. Math operations
/// such as interpolation cannot be performed on empty cells, and attempting to
/// do so will result in this error.

/// @var ErrorCode CellNotFound
/// @brief A cell matching some given criteria could not be found.
///
/// This error code is most often used in a cell locator where no cell in the
/// given region could be found.

/// @brief Convert a `vtkm::ErrorCode` into a human-readable string.
///
/// This method is useful when reporting the results of a function that
/// failed.
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
