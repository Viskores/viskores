//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_Matrix_h
#define vtk_m_Matrix_h

#include <vtkm/Assert.h>
#include <vtkm/Math.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/Types.h>
#include <vtkm/VecTraits.h>

namespace vtkm
{

/// \brief Basic Matrix type.
///
/// The Matrix class holds a small two dimensional array for simple linear
/// algebra and vector operations. VTK-m provides several Matrix-based
/// operations to assist in visualization computations.
///
/// A Matrix is not intended to hold very large arrays. Rather, they are a
/// per-thread data structure to hold information like geometric transforms and
/// tensors.
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
class Matrix
{
public:
  using ComponentType = T;
  static constexpr vtkm::IdComponent NUM_ROWS = NumRow;
  static constexpr vtkm::IdComponent NUM_COLUMNS = NumCol;

  VTKM_EXEC_CONT
  Matrix() {}

  VTKM_EXEC_CONT
  explicit Matrix(const ComponentType& value)
    : Components(vtkm::Vec<ComponentType, NUM_COLUMNS>(value))
  {
  }

  /// Brackets are used to reference a matrix like a 2D array (i.e.
  /// matrix[row][column]).
  ///
  VTKM_EXEC_CONT
  const vtkm::Vec<ComponentType, NUM_COLUMNS>& operator[](vtkm::IdComponent rowIndex) const
  {
    VTKM_ASSERT(rowIndex >= 0);
    VTKM_ASSERT(rowIndex < NUM_ROWS);
    return this->Components[rowIndex];
  }

  /// Brackets are used to referens a matrix like a 2D array i.e.
  /// matrix[row][column].
  ///
  VTKM_EXEC_CONT
  vtkm::Vec<ComponentType, NUM_COLUMNS>& operator[](vtkm::IdComponent rowIndex)
  {
    VTKM_ASSERT(rowIndex >= 0);
    VTKM_ASSERT(rowIndex < NUM_ROWS);
    return this->Components[rowIndex];
  }

  /// Parentheses are used to reference a matrix using mathematical tuple
  /// notation i.e. matrix(row,column).
  ///
  VTKM_EXEC_CONT
  const ComponentType& operator()(vtkm::IdComponent rowIndex, vtkm::IdComponent colIndex) const
  {
    VTKM_ASSERT(rowIndex >= 0);
    VTKM_ASSERT(rowIndex < NUM_ROWS);
    VTKM_ASSERT(colIndex >= 0);
    VTKM_ASSERT(colIndex < NUM_COLUMNS);
    return (*this)[rowIndex][colIndex];
  }

  /// Parentheses are used to reference a matrix using mathematical tuple
  /// notation i.e. matrix(row,column).
  ///
  VTKM_EXEC_CONT
  ComponentType& operator()(vtkm::IdComponent rowIndex, vtkm::IdComponent colIndex)
  {
    VTKM_ASSERT(rowIndex >= 0);
    VTKM_ASSERT(rowIndex < NUM_ROWS);
    VTKM_ASSERT(colIndex >= 0);
    VTKM_ASSERT(colIndex < NUM_COLUMNS);
    return (*this)[rowIndex][colIndex];
  }

private:
  vtkm::Vec<vtkm::Vec<ComponentType, NUM_COLUMNS>, NUM_ROWS> Components;
};

/// Returns a tuple containing the given row (indexed from 0) of the given
/// matrix.
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT const vtkm::Vec<T, NumCol>& MatrixGetRow(
  const vtkm::Matrix<T, NumRow, NumCol>& matrix,
  vtkm::IdComponent rowIndex)
{
  return matrix[rowIndex];
}

/// Returns a tuple containing the given column (indexed from 0) of the given
/// matrix.  Might not be as efficient as the \c MatrixGetRow function.
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT vtkm::Vec<T, NumRow> MatrixGetColumn(const vtkm::Matrix<T, NumRow, NumCol>& matrix,
                                                    vtkm::IdComponent columnIndex)
{
  vtkm::Vec<T, NumRow> columnValues;
  for (vtkm::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    columnValues[rowIndex] = matrix(rowIndex, columnIndex);
  }
  return columnValues;
}

/// Convenience function for setting a row of a matrix.
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT void MatrixSetRow(vtkm::Matrix<T, NumRow, NumCol>& matrix,
                                 vtkm::IdComponent rowIndex,
                                 const vtkm::Vec<T, NumCol>& rowValues)
{
  matrix[rowIndex] = rowValues;
}

/// Convenience function for setting a column of a matrix.
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT void MatrixSetColumn(vtkm::Matrix<T, NumRow, NumCol>& matrix,
                                    vtkm::IdComponent columnIndex,
                                    const vtkm::Vec<T, NumRow>& columnValues)
{
  for (vtkm::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    matrix(rowIndex, columnIndex) = columnValues[rowIndex];
  }
}

/// Standard matrix multiplication.
///
template <typename T,
          vtkm::IdComponent NumRow,
          vtkm::IdComponent NumCol,
          vtkm::IdComponent NumInternal>
VTKM_EXEC_CONT vtkm::Matrix<T, NumRow, NumCol> MatrixMultiply(
  const vtkm::Matrix<T, NumRow, NumInternal>& leftFactor,
  const vtkm::Matrix<T, NumInternal, NumCol>& rightFactor)
{
  vtkm::Matrix<T, NumRow, NumCol> result;
  for (vtkm::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    for (vtkm::IdComponent colIndex = 0; colIndex < NumCol; colIndex++)
    {
      T sum = T(leftFactor(rowIndex, 0) * rightFactor(0, colIndex));
      for (vtkm::IdComponent internalIndex = 1; internalIndex < NumInternal; internalIndex++)
      {
        sum = T(sum + (leftFactor(rowIndex, internalIndex) * rightFactor(internalIndex, colIndex)));
      }
      result(rowIndex, colIndex) = sum;
    }
  }
  return result;
}

/// Standard matrix-vector multiplication.
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT vtkm::Vec<T, NumRow> MatrixMultiply(
  const vtkm::Matrix<T, NumRow, NumCol>& leftFactor,
  const vtkm::Vec<T, NumCol>& rightFactor)
{
  vtkm::Vec<T, NumRow> product;
  for (vtkm::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    product[rowIndex] = vtkm::Dot(vtkm::MatrixGetRow(leftFactor, rowIndex), rightFactor);
  }
  return product;
}

/// Standard vector-matrix multiplication
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT vtkm::Vec<T, NumCol> MatrixMultiply(
  const vtkm::Vec<T, NumRow>& leftFactor,
  const vtkm::Matrix<T, NumRow, NumCol>& rightFactor)
{
  vtkm::Vec<T, NumCol> product;
  for (vtkm::IdComponent colIndex = 0; colIndex < NumCol; colIndex++)
  {
    product[colIndex] = vtkm::Dot(leftFactor, vtkm::MatrixGetColumn(rightFactor, colIndex));
  }
  return product;
}

/// Returns the identity matrix.
///
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Matrix<T, Size, Size> MatrixIdentity()
{
  vtkm::Matrix<T, Size, Size> result(T(0));
  for (vtkm::IdComponent index = 0; index < Size; index++)
  {
    result(index, index) = T(1.0);
  }
  return result;
}

/// Fills the given matrix with the identity matrix.
///
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT void MatrixIdentity(vtkm::Matrix<T, Size, Size>& matrix)
{
  matrix = vtkm::MatrixIdentity<T, Size>();
}

/// Returns the transpose of the given matrix.
///
template <typename T, vtkm::IdComponent NumRows, vtkm::IdComponent NumCols>
VTKM_EXEC_CONT vtkm::Matrix<T, NumCols, NumRows> MatrixTranspose(
  const vtkm::Matrix<T, NumRows, NumCols>& matrix)
{
  vtkm::Matrix<T, NumCols, NumRows> result;
  for (vtkm::IdComponent index = 0; index < NumRows; index++)
  {
    vtkm::MatrixSetColumn(result, index, vtkm::MatrixGetRow(matrix, index));
#ifdef VTKM_ICC
    // For reasons I do not really understand, the Intel compiler with with
    // optimization on is sometimes removing this for loop. It appears that the
    // optimizer sometimes does not recognize that the MatrixSetColumn function
    // has side effects. I cannot fathom any reason for this other than a bug in
    // the compiler, but unfortunately I do not know a reliable way to
    // demonstrate the problem.
    __asm__("");
#endif
  }
  return result;
}

namespace detail
{

// Used with MatrixLUPFactor.
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT void MatrixLUPFactorFindPivot(vtkm::Matrix<T, Size, Size>& A,
                                             vtkm::Vec<vtkm::IdComponent, Size>& permutation,
                                             vtkm::IdComponent topCornerIndex,
                                             T& inversionParity,
                                             bool& valid)
{
  vtkm::IdComponent maxRowIndex = topCornerIndex;
  T maxValue = vtkm::Abs(A(maxRowIndex, topCornerIndex));
  for (vtkm::IdComponent rowIndex = topCornerIndex + 1; rowIndex < Size; rowIndex++)
  {
    T compareValue = vtkm::Abs(A(rowIndex, topCornerIndex));
    if (maxValue < compareValue)
    {
      maxValue = compareValue;
      maxRowIndex = rowIndex;
    }
  }

  if (maxValue < vtkm::Epsilon<T>())
  {
    valid = false;
  }

  if (maxRowIndex != topCornerIndex)
  {
    // Swap rows in matrix.
    vtkm::Vec<T, Size> maxRow = vtkm::MatrixGetRow(A, maxRowIndex);
    vtkm::MatrixSetRow(A, maxRowIndex, vtkm::MatrixGetRow(A, topCornerIndex));
    vtkm::MatrixSetRow(A, topCornerIndex, maxRow);

    // Record change in permutation matrix.
    vtkm::IdComponent maxOriginalRowIndex = permutation[maxRowIndex];
    permutation[maxRowIndex] = permutation[topCornerIndex];
    permutation[topCornerIndex] = maxOriginalRowIndex;

    // Keep track of inversion parity.
    inversionParity = -inversionParity;
  }
}

// Used with MatrixLUPFactor
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT void MatrixLUPFactorFindUpperTriangleElements(vtkm::Matrix<T, Size, Size>& A,
                                                             vtkm::IdComponent topCornerIndex)
{
  // Compute values for upper triangle on row topCornerIndex
  for (vtkm::IdComponent colIndex = topCornerIndex + 1; colIndex < Size; colIndex++)
  {
    A(topCornerIndex, colIndex) /= A(topCornerIndex, topCornerIndex);
  }

  // Update the rest of the matrix for calculations on subsequent rows
  for (vtkm::IdComponent rowIndex = topCornerIndex + 1; rowIndex < Size; rowIndex++)
  {
    for (vtkm::IdComponent colIndex = topCornerIndex + 1; colIndex < Size; colIndex++)
    {
      A(rowIndex, colIndex) -= A(rowIndex, topCornerIndex) * A(topCornerIndex, colIndex);
    }
  }
}

/// Performs an LUP-factorization on the given matrix using Crout's method. The
/// LU-factorization takes a matrix A and decomposes it into a lower triangular
/// matrix L and upper triangular matrix U such that A = LU. The
/// LUP-factorization also allows permutation of A, which makes the
/// decomposition always possible so long as A is not singular. In addition to
/// matrices L and U, LUP also finds permutation matrix P containing all zeros
/// except one 1 per row and column such that PA = LU.
///
/// The result is done in place such that the lower triangular matrix, L, is
/// stored in the lower-left triangle of A including the diagonal. The upper
/// triangular matrix, U, is stored in the upper-right triangle of L not
/// including the diagonal. The diagonal of U in Crout's method is all 1's (and
/// therefore not explicitly stored).
///
/// The permutation matrix P is represented by the permutation vector. If
/// permutation[i] = j then row j in the original matrix A has been moved to
/// row i in the resulting matrices. The permutation matrix P can be
/// represented by a matrix with p_i,j = 1 if permutation[i] = j and 0
/// otherwise. If using LUP-factorization to compute a determinant, you also
/// need to know the parity (whether there is an odd or even amount) of
/// inversions. An inversion is an instance of a smaller number appearing after
/// a larger number in permutation. Although you can compute the inversion
/// parity after the fact, this function keeps track of it with much less
/// compute resources. The parameter inversionParity is set to 1.0 for even
/// parity and -1.0 for odd parity.
///
/// Not all matrices (specifically singular matrices) have an
/// LUP-factorization. If the LUP-factorization succeeds, valid is set to true.
/// Otherwise, valid is set to false and the result is indeterminant.
///
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT void MatrixLUPFactor(vtkm::Matrix<T, Size, Size>& A,
                                    vtkm::Vec<vtkm::IdComponent, Size>& permutation,
                                    T& inversionParity,
                                    bool& valid)
{
  // Initialize permutation.
  for (vtkm::IdComponent index = 0; index < Size; index++)
  {
    permutation[index] = index;
  }
  inversionParity = T(1);
  valid = true;

  for (vtkm::IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    MatrixLUPFactorFindPivot(A, permutation, rowIndex, inversionParity, valid);
    MatrixLUPFactorFindUpperTriangleElements(A, rowIndex);
  }
}

/// Use a previous factorization done with MatrixLUPFactor to solve the
/// system Ax = b.  Instead of A, this method takes in the LU and P
/// matrices calculated by MatrixLUPFactor from A. The x matrix is returned.
///
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> MatrixLUPSolve(
  const vtkm::Matrix<T, Size, Size>& LU,
  const vtkm::Vec<vtkm::IdComponent, Size>& permutation,
  const vtkm::Vec<T, Size>& b)
{
  // The LUP-factorization gives us PA = LU or equivalently A = inv(P)LU.
  // Substituting into Ax = b gives us inv(P)LUx = b or LUx = Pb.
  // Now consider the intermediate vector y = Ux.
  // Substituting in the previous two equations yields Ly = Pb.
  // Solving Ly = Pb is easy because L is triangular and P is just a
  // permutation.
  vtkm::Vec<T, Size> y;
  for (vtkm::IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    y[rowIndex] = b[permutation[rowIndex]];
    // Recall that L is stored in the lower triangle of LU including diagonal.
    for (vtkm::IdComponent colIndex = 0; colIndex < rowIndex; colIndex++)
    {
      y[rowIndex] -= LU(rowIndex, colIndex) * y[colIndex];
    }
    y[rowIndex] /= LU(rowIndex, rowIndex);
  }

  // Now that we have y, we can easily solve Ux = y for x.
  vtkm::Vec<T, Size> x;
  for (vtkm::IdComponent rowIndex = Size - 1; rowIndex >= 0; rowIndex--)
  {
    // Recall that U is stored in the upper triangle of LU with the diagonal
    // implicitly all 1's.
    x[rowIndex] = y[rowIndex];
    for (vtkm::IdComponent colIndex = rowIndex + 1; colIndex < Size; colIndex++)
    {
      x[rowIndex] -= LU(rowIndex, colIndex) * x[colIndex];
    }
  }

  return x;
}

} // namespace detail

/// Solve the linear system Ax = b for x. If a single solution is found, valid
/// is set to true, false otherwise.
///
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Vec<T, Size> SolveLinearSystem(const vtkm::Matrix<T, Size, Size>& A,
                                                    const vtkm::Vec<T, Size>& b,
                                                    bool& valid)
{
  // First, we will make an LUP-factorization to help us.
  vtkm::Matrix<T, Size, Size> LU = A;
  vtkm::Vec<vtkm::IdComponent, Size> permutation;
  T inversionParity; // Unused.
  detail::MatrixLUPFactor(LU, permutation, inversionParity, valid);

  // Next, use the decomposition to solve the system.
  return detail::MatrixLUPSolve(LU, permutation, b);
}

/// Find and return the inverse of the given matrix. If the matrix is singular,
/// the inverse will not be correct and valid will be set to false.
///
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT vtkm::Matrix<T, Size, Size> MatrixInverse(const vtkm::Matrix<T, Size, Size>& A,
                                                         bool& valid)
{
  // First, we will make an LUP-factorization to help us.
  vtkm::Matrix<T, Size, Size> LU = A;
  vtkm::Vec<vtkm::IdComponent, Size> permutation;
  T inversionParity; // Unused
  detail::MatrixLUPFactor(LU, permutation, inversionParity, valid);

  // We will use the decomposition to solve AX = I for X where X is
  // clearly the inverse of A.  Our solve method only works for vectors,
  // so we solve for one column of invA at a time.
  vtkm::Matrix<T, Size, Size> invA;
  vtkm::Vec<T, Size> ICol(T(0));
  for (vtkm::IdComponent colIndex = 0; colIndex < Size; colIndex++)
  {
    ICol[colIndex] = 1;
    vtkm::Vec<T, Size> invACol = detail::MatrixLUPSolve(LU, permutation, ICol);
    ICol[colIndex] = 0;
    vtkm::MatrixSetColumn(invA, colIndex, invACol);
  }
  return invA;
}

/// Compute the determinant of a matrix.
///
template <typename T, vtkm::IdComponent Size>
VTKM_EXEC_CONT T MatrixDeterminant(const vtkm::Matrix<T, Size, Size>& A)
{
  // First, we will make an LUP-factorization to help us.
  vtkm::Matrix<T, Size, Size> LU = A;
  vtkm::Vec<vtkm::IdComponent, Size> permutation;
  T inversionParity;
  bool valid;
  detail::MatrixLUPFactor(LU, permutation, inversionParity, valid);

  // If the matrix is singular, the factorization is invalid, but in that
  // case we know that the determinant is 0.
  if (!valid)
  {
    return 0;
  }

  // The determinant is equal to the product of the diagonal of the L matrix,
  // possibly negated depending on the parity of the inversion. The
  // inversionParity variable is set to 1.0 and -1.0 for even and odd parity,
  // respectively. This sign determines whether the product should be negated.
  T product = inversionParity;
  for (vtkm::IdComponent index = 0; index < Size; index++)
  {
    product *= LU(index, index);
  }
  return product;
}

// Specializations for common small determinants.

template <typename T>
VTKM_EXEC_CONT T MatrixDeterminant(const vtkm::Matrix<T, 1, 1>& A)
{
  return A(0, 0);
}

template <typename T>
VTKM_EXEC_CONT T MatrixDeterminant(const vtkm::Matrix<T, 2, 2>& A)
{
  return A(0, 0) * A(1, 1) - A(1, 0) * A(0, 1);
}

template <typename T>
VTKM_EXEC_CONT T MatrixDeterminant(const vtkm::Matrix<T, 3, 3>& A)
{
  return A(0, 0) * A(1, 1) * A(2, 2) + A(1, 0) * A(2, 1) * A(0, 2) + A(2, 0) * A(0, 1) * A(1, 2) -
    A(0, 0) * A(2, 1) * A(1, 2) - A(1, 0) * A(0, 1) * A(2, 2) - A(2, 0) * A(1, 1) * A(0, 2);
}

//---------------------------------------------------------------------------
// Implementations of traits for matrices.

/// Tag used to identify 2 dimensional types (matrices). A TypeTraits class
/// will typedef this class to DimensionalityTag.
///
struct TypeTraitsMatrixTag
{
};

template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
struct TypeTraits<vtkm::Matrix<T, NumRow, NumCol>>
{
  using NumericTag = typename TypeTraits<T>::NumericTag;
  using DimensionalityTag = vtkm::TypeTraitsMatrixTag;

  VTKM_EXEC_CONT
  static vtkm::Matrix<T, NumRow, NumCol> ZeroInitialization()
  {
    return vtkm::Matrix<T, NumRow, NumCol>(vtkm::TypeTraits<T>::ZeroInitialization());
  }
};

/// A matrix has vector traits to implement component-wise operations.
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
struct VecTraits<vtkm::Matrix<T, NumRow, NumCol>>
{
private:
  using MatrixType = vtkm::Matrix<T, NumRow, NumCol>;

public:
  using ComponentType = T;
  using BaseComponentType = typename vtkm::VecTraits<T>::BaseComponentType;
  static constexpr vtkm::IdComponent NUM_COMPONENTS = NumRow * NumCol;
  using HasMultipleComponents = vtkm::VecTraitsTagMultipleComponents;
  using IsSizeStatic = vtkm::VecTraitsTagSizeStatic;

  VTKM_EXEC_CONT
  static vtkm::IdComponent GetNumberOfComponents(const MatrixType&) { return NUM_COMPONENTS; }

  VTKM_EXEC_CONT
  static const ComponentType& GetComponent(const MatrixType& matrix, vtkm::IdComponent component)
  {
    vtkm::IdComponent colIndex = component % NumCol;
    vtkm::IdComponent rowIndex = component / NumCol;
    return matrix(rowIndex, colIndex);
  }
  VTKM_EXEC_CONT
  static ComponentType& GetComponent(MatrixType& matrix, vtkm::IdComponent component)
  {
    vtkm::IdComponent colIndex = component % NumCol;
    vtkm::IdComponent rowIndex = component / NumCol;
    return matrix(rowIndex, colIndex);
  }
  VTKM_EXEC_CONT
  static void SetComponent(MatrixType& matrix, vtkm::IdComponent component, T value)
  {
    GetComponent(matrix, component) = value;
  }

  template <typename NewComponentType>
  using ReplaceComponentType = vtkm::Matrix<NewComponentType, NumRow, NumCol>;

  template <typename NewComponentType>
  using ReplaceBaseComponentType =
    vtkm::Matrix<typename vtkm::VecTraits<T>::template ReplaceBaseComponentType<NewComponentType>,
                 NumRow,
                 NumCol>;
};

//---------------------------------------------------------------------------
// Basic comparison operators.

template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT bool operator==(const vtkm::Matrix<T, NumRow, NumCol>& a,
                               const vtkm::Matrix<T, NumRow, NumCol>& b)
{
  for (vtkm::IdComponent colIndex = 0; colIndex < NumCol; colIndex++)
  {
    for (vtkm::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
    {
      if (a(rowIndex, colIndex) != b(rowIndex, colIndex))
        return false;
    }
  }
  return true;
}
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_EXEC_CONT bool operator!=(const vtkm::Matrix<T, NumRow, NumCol>& a,
                               const vtkm::Matrix<T, NumRow, NumCol>& b)
{
  return !(a == b);
}

/// Helper function for printing out matricies during testing
///
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
VTKM_CONT std::ostream& operator<<(std::ostream& stream, const vtkm::Matrix<T, NumRow, NumCol>& mat)
{
  stream << std::endl;
  for (vtkm::IdComponent row = 0; row < NumRow; ++row)
  {
    stream << "| ";
    for (vtkm::IdComponent col = 0; col < NumCol; ++col)
    {
      stream << mat(row, col) << "\t";
    }
    stream << "|" << std::endl;
  }
  return stream;
}
} // namespace vtkm

#endif //vtk_m_Matrix_h
