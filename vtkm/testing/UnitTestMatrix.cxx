//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Matrix.h>

#include <vtkm/VecTraits.h>

#include <vtkm/testing/Testing.h>

// If more tests need a value for Matrix, we can move this to Testing.h
template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
vtkm::Matrix<T, NumRow, NumCol> TestValue(vtkm::Id index, const vtkm::Matrix<T, NumRow, NumCol>&)
{
  vtkm::Matrix<T, NumRow, NumCol> value;
  for (vtkm::IdComponent rowIndex = 0; rowIndex < NumRow; rowIndex++)
  {
    using RowType = vtkm::Vec<T, NumCol>;
    RowType row = TestValue(index, RowType()) +
      RowType(static_cast<typename RowType::ComponentType>(10 * rowIndex));
    vtkm::MatrixSetRow(value, rowIndex, row);
  }
  return value;
}

namespace
{

#define FOR_ROW_COL(matrix)                                                                        \
  for (vtkm::IdComponent row = 0; row < (matrix).NUM_ROWS; row++)                                  \
    for (vtkm::IdComponent col = 0; col < (matrix).NUM_COLUMNS; col++)

template <typename T, vtkm::IdComponent NumRow, vtkm::IdComponent NumCol>
struct MatrixTest
{
  static constexpr vtkm::IdComponent NUM_ROWS = NumRow;
  static constexpr vtkm::IdComponent NUM_COLS = NumCol;
  using MatrixType = vtkm::Matrix<T, NUM_ROWS, NUM_COLS>;
  using ComponentType = typename MatrixType::ComponentType;

  static void BasicCreation()
  {
    std::cout << "Basic creation." << std::endl;
    MatrixType matrix(5);
    FOR_ROW_COL(matrix)
    {
      VTKM_TEST_ASSERT(test_equal(matrix(row, col), static_cast<T>(5)), "Constant set incorrect.");
    }
  }

  static void BasicAccessors()
  {
    std::cout << "Basic accessors." << std::endl;
    MatrixType matrix;
    MatrixType value = TestValue(0, MatrixType());
    FOR_ROW_COL(matrix) { matrix[row][col] = ComponentType(value(row, col) * 2); }
    FOR_ROW_COL(matrix)
    {
      VTKM_TEST_ASSERT(test_equal(matrix(row, col), value(row, col) * 2), "Bad set or retrieve.");
      const MatrixType const_matrix = matrix;
      VTKM_TEST_ASSERT(test_equal(const_matrix(row, col), value(row, col) * 2),
                       "Bad set or retrieve.");
    }

    FOR_ROW_COL(matrix) { matrix(row, col) = value(row, col); }
    const MatrixType const_matrix = matrix;
    FOR_ROW_COL(matrix)
    {
      VTKM_TEST_ASSERT(test_equal(matrix[row][col], value(row, col)), "Bad set or retrieve.");
      VTKM_TEST_ASSERT(test_equal(const_matrix[row][col], value(row, col)), "Bad set or retrieve.");
    }
    VTKM_TEST_ASSERT(matrix == const_matrix, "Equal test operator not working.");
    VTKM_TEST_ASSERT(!(matrix != const_matrix), "Not-Equal test operator not working.");
    VTKM_TEST_ASSERT(test_equal(matrix, const_matrix), "Vector-based equal test not working.");
  }

  static void RowColAccessors()
  {
    using ColumnType = vtkm::Vec<T, NUM_ROWS>;
    using RowType = vtkm::Vec<T, NUM_COLS>;
    const MatrixType const_matrix = TestValue(0, MatrixType());
    MatrixType matrix;

    std::cout << "Access by row or column" << std::endl;
    FOR_ROW_COL(matrix)
    {
      RowType rowvec = vtkm::MatrixGetRow(const_matrix, row);
      VTKM_TEST_ASSERT(test_equal(rowvec[col], const_matrix(row, col)), "Bad get row.");
      ColumnType columnvec = vtkm::MatrixGetColumn(const_matrix, col);
      VTKM_TEST_ASSERT(test_equal(columnvec[row], const_matrix(row, col)), "Bad get col.");
    }

    std::cout << "Set a row." << std::endl;
    for (vtkm::IdComponent row = 0; row < NUM_ROWS; row++)
    {
      RowType rowvec = vtkm::MatrixGetRow(const_matrix, NUM_ROWS - row - 1);
      vtkm::MatrixSetRow(matrix, row, rowvec);
    }
    FOR_ROW_COL(matrix)
    {
      VTKM_TEST_ASSERT(test_equal(matrix(NUM_ROWS - row - 1, col), const_matrix(row, col)),
                       "Rows not set right.");
    }

    std::cout << "Set a column." << std::endl;
    for (vtkm::IdComponent col = 0; col < NUM_COLS; col++)
    {
      ColumnType colvec = vtkm::MatrixGetColumn(const_matrix, NUM_COLS - col - 1);
      vtkm::MatrixSetColumn(matrix, col, colvec);
    }
    FOR_ROW_COL(matrix)
    {
      VTKM_TEST_ASSERT(test_equal(matrix(row, NUM_COLS - col - 1), const_matrix(row, col)),
                       "Columns not set right.");
    }
  }

  static void Multiply()
  {
    std::cout << "Try multiply." << std::endl;
    const MatrixType leftFactor = TestValue(0, MatrixType());
    vtkm::Matrix<T, NUM_COLS, 4> rightFactor = TestValue(1, vtkm::Matrix<T, NUM_COLS, 4>());

    vtkm::Matrix<T, NUM_ROWS, 4> product = vtkm::MatrixMultiply(leftFactor, rightFactor);

    FOR_ROW_COL(product)
    {
      vtkm::Vec<T, NUM_COLS> leftVector = vtkm::MatrixGetRow(leftFactor, row);
      vtkm::Vec<T, NUM_COLS> rightVector = vtkm::MatrixGetColumn(rightFactor, col);
      VTKM_TEST_ASSERT(test_equal(product(row, col), vtkm::Dot(leftVector, rightVector)),
                       "Matrix multiple wrong.");
    }

    std::cout << "Vector multiply." << std::endl;
    MatrixType matrixFactor;
    vtkm::Vec<T, NUM_ROWS> leftVector(2);
    vtkm::Vec<T, NUM_COLS> rightVector;
    FOR_ROW_COL(matrixFactor)
    {
      matrixFactor(row, col) = T(row + 1);
      rightVector[col] = T(col + 1);
    }

    vtkm::Vec<T, NUM_COLS> leftResult = vtkm::MatrixMultiply(leftVector, matrixFactor);
    for (vtkm::IdComponent index = 0; index < NUM_COLS; index++)
    {
      VTKM_TEST_ASSERT(test_equal(leftResult[index], T(NUM_ROWS * (NUM_ROWS + 1))),
                       "Vector/matrix multiple wrong.");
    }

    vtkm::Vec<T, NUM_ROWS> rightResult = vtkm::MatrixMultiply(matrixFactor, rightVector);
    for (vtkm::IdComponent index = 0; index < NUM_ROWS; index++)
    {
      VTKM_TEST_ASSERT(
        test_equal(rightResult[index], T(((index + 1) * NUM_COLS * (NUM_COLS + 1)) / 2)),
        "Matrix/vector multiple wrong.");
    }
  }

  static void Identity()
  {
    std::cout << "Check identity" << std::endl;

    MatrixType originalMatrix = TestValue(0, MatrixType());

    vtkm::Matrix<T, NUM_COLS, NUM_COLS> identityMatrix;
    vtkm::MatrixIdentity(identityMatrix);

    MatrixType multMatrix = vtkm::MatrixMultiply(originalMatrix, identityMatrix);

    VTKM_TEST_ASSERT(test_equal(originalMatrix, multMatrix), "Identity is not really identity.");
  }

  static void Transpose()
  {
    std::cout << "Check transpose" << std::endl;

    MatrixType originalMatrix = TestValue(0, MatrixType());

    vtkm::Matrix<T, NUM_COLS, NUM_ROWS> transMatrix = vtkm::MatrixTranspose(originalMatrix);
    FOR_ROW_COL(originalMatrix)
    {
      VTKM_TEST_ASSERT(test_equal(originalMatrix(row, col), transMatrix(col, row)),
                       "Transpose wrong.");
    }
  }

  static void Run()
  {
    std::cout << "-- " << NUM_ROWS << " x " << NUM_COLS << std::endl;

    BasicCreation();
    BasicAccessors();
    RowColAccessors();
    Multiply();
    Identity();
    Transpose();
  }

private:
  MatrixTest() = delete;
};

template <typename T, int NumRow>
void MatrixTest1()
{
  MatrixTest<T, NumRow, 1>::Run();
  MatrixTest<T, NumRow, 2>::Run();
  MatrixTest<T, NumRow, 3>::Run();
  MatrixTest<T, NumRow, 4>::Run();
  MatrixTest<T, NumRow, 5>::Run();
}

template <typename T>
void NonSingularMatrix(vtkm::Matrix<T, 1, 1>& matrix)
{
  matrix(0, 0) = 1;
}

template <typename T>
void NonSingularMatrix(vtkm::Matrix<T, 2, 2>& matrix)
{
  matrix(0, 0) = -5;
  matrix(0, 1) = 6;
  matrix(1, 0) = -7;
  matrix(1, 1) = -2;
}

template <typename T>
void NonSingularMatrix(vtkm::Matrix<T, 3, 3>& matrix)
{
  matrix(0, 0) = 1;
  matrix(0, 1) = -2;
  matrix(0, 2) = 3;
  matrix(1, 0) = 6;
  matrix(1, 1) = 7;
  matrix(1, 2) = -1;
  matrix(2, 0) = -3;
  matrix(2, 1) = 1;
  matrix(2, 2) = 4;
}

template <typename T>
void NonSingularMatrix(vtkm::Matrix<T, 4, 4>& matrix)
{
  matrix(0, 0) = 2;
  matrix(0, 1) = 1;
  matrix(0, 2) = 0;
  matrix(0, 3) = 3;
  matrix(1, 0) = -1;
  matrix(1, 1) = 0;
  matrix(1, 2) = 2;
  matrix(1, 3) = 4;
  matrix(2, 0) = 4;
  matrix(2, 1) = -2;
  matrix(2, 2) = 7;
  matrix(2, 3) = 0;
  matrix(3, 0) = -4;
  matrix(3, 1) = 3;
  matrix(3, 2) = 5;
  matrix(3, 3) = 1;
}

template <typename T>
void NonSingularMatrix(vtkm::Matrix<T, 5, 5>& mat)
{
  mat(0, 0) = 2;
  mat(0, 1) = 1;
  mat(0, 2) = 3;
  mat(0, 3) = 7;
  mat(0, 4) = 5;
  mat(1, 0) = 3;
  mat(1, 1) = 8;
  mat(1, 2) = 7;
  mat(1, 3) = 9;
  mat(1, 4) = 8;
  mat(2, 0) = 3;
  mat(2, 1) = 4;
  mat(2, 2) = 1;
  mat(2, 3) = 6;
  mat(2, 4) = 2;
  mat(3, 0) = 4;
  mat(3, 1) = 0;
  mat(3, 2) = 2;
  mat(3, 3) = 2;
  mat(3, 4) = 3;
  mat(4, 0) = 7;
  mat(4, 1) = 9;
  mat(4, 2) = 1;
  mat(4, 3) = 5;
  mat(4, 4) = 4;
}

template <typename T, vtkm::IdComponent S>
void PrintMatrix(const vtkm::Matrix<T, S, S>& m)
{
  std::cout << "matrix\n";
  for (vtkm::IdComponent i = 0; i < S; ++i)
  {
    std::cout << "\t" << m[i] << "\n";
  }
  std::cout << std::flush;
}

template <typename T, int Size>
void SingularMatrix(vtkm::Matrix<T, Size, Size>& singularMatrix)
{
  FOR_ROW_COL(singularMatrix) { singularMatrix(row, col) = static_cast<T>(row + col); }
  constexpr bool larger_than_1 = Size > 1;
  if (larger_than_1)
  {
    vtkm::MatrixSetRow(singularMatrix, 0, vtkm::MatrixGetRow(singularMatrix, (Size + 1) / 2));
  }
}

// A simple but slow implementation of finding a determinant for comparison
// purposes.
template <typename T>
T RecursiveDeterminant(const vtkm::Matrix<T, 1, 1>& A)
{
  return A(0, 0);
}

template <typename T, vtkm::IdComponent Size>
T RecursiveDeterminant(const vtkm::Matrix<T, Size, Size>& A)
{
  vtkm::Matrix<T, Size - 1, Size - 1> cofactorMatrix;
  T sum = 0.0;
  T sign = 1.0;
  for (vtkm::IdComponent rowIndex = 0; rowIndex < Size; rowIndex++)
  {
    // Create the cofactor matrix for entry A(rowIndex,0)
    for (vtkm::IdComponent cofactorRowIndex = 0; cofactorRowIndex < rowIndex; cofactorRowIndex++)
    {
      for (vtkm::IdComponent colIndex = 1; colIndex < Size; colIndex++)
      {
        cofactorMatrix(cofactorRowIndex, colIndex - 1) = A(cofactorRowIndex, colIndex);
      }
    }
    for (vtkm::IdComponent cofactorRowIndex = rowIndex + 1; cofactorRowIndex < Size;
         cofactorRowIndex++)
    {
      for (vtkm::IdComponent colIndex = 1; colIndex < Size; colIndex++)
      {
        cofactorMatrix(cofactorRowIndex - 1, colIndex - 1) = A(cofactorRowIndex, colIndex);
      }
    }
    sum += sign * A(rowIndex, 0) * RecursiveDeterminant(cofactorMatrix);
    sign = -sign;
  }
  return sum;
}

template <typename T, vtkm::IdComponent Size>
struct SquareMatrixTest
{
  static constexpr vtkm::IdComponent SIZE = Size;
  using MatrixType = vtkm::Matrix<T, Size, Size>;

  static void CheckMatrixSize()
  {
    std::cout << "Check reported matrix size." << std::endl;
    VTKM_TEST_ASSERT(MatrixType::NUM_ROWS == SIZE, "Matrix has wrong size.");
    VTKM_TEST_ASSERT(MatrixType::NUM_COLUMNS == SIZE, "Matrix has wrong size.");
  }

  static void LUPFactor()
  {
    std::cout << "Test LUP-factorization" << std::endl;

    MatrixType A;
    NonSingularMatrix(A);
    const MatrixType originalMatrix = A;
    vtkm::Vec<vtkm::IdComponent, SIZE> permutationVector;
    T inversionParity;
    bool valid;

    vtkm::detail::MatrixLUPFactor(A, permutationVector, inversionParity, valid);
    VTKM_TEST_ASSERT(valid, "Matrix declared singular?");

    // Reconstruct L and U matrices from A.
    MatrixType L(0);
    MatrixType U(0);
    FOR_ROW_COL(A)
    {
      if (row < col)
      {
        U(row, col) = A(row, col);
      }
      else //(row >= col)
      {
        L(row, col) = A(row, col);
        if (row == col)
        {
          U(row, col) = 1;
        }
      }
    }

    // Check parity of permutation.
    T computedParity = 1.0;
    for (int i = 0; i < SIZE; i++)
    {
      for (int j = i + 1; j < SIZE; j++)
      {
        if (permutationVector[i] > permutationVector[j])
        {
          computedParity = -computedParity;
        }
      }
    }
    VTKM_TEST_ASSERT(test_equal(inversionParity, computedParity), "Got bad inversion parity.");

    // Reconstruct permutation matrix P.
    MatrixType P(0);
    for (vtkm::IdComponent index = 0; index < Size; index++)
    {
      P(index, permutationVector[index]) = 1;
    }

    // Check that PA = LU is actually correct.
    MatrixType permutedMatrix = vtkm::MatrixMultiply(P, originalMatrix);
    MatrixType productMatrix = vtkm::MatrixMultiply(L, U);
    VTKM_TEST_ASSERT(test_equal(permutedMatrix, productMatrix),
                     "LUP-factorization gave inconsistent answer.");

    // Check that a singular matrix is identified.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);
    vtkm::detail::MatrixLUPFactor(singularMatrix, permutationVector, inversionParity, valid);
    VTKM_TEST_ASSERT(!valid, "Expected matrix to be declared singular.");
  }

  static void SolveLinearSystem()
  {
    std::cout << "Solve a linear system" << std::endl;

    MatrixType A;
    vtkm::Vec<T, SIZE> b;
    NonSingularMatrix(A);
    for (vtkm::IdComponent index = 0; index < SIZE; index++)
    {
      b[index] = static_cast<T>(index + 1);
    }
    bool valid;

    vtkm::Vec<T, SIZE> x = vtkm::SolveLinearSystem(A, b, valid);
    VTKM_TEST_ASSERT(valid, "Matrix declared singular?");

    // Check result.
    vtkm::Vec<T, SIZE> check = vtkm::MatrixMultiply(A, x);
    VTKM_TEST_ASSERT(test_equal(b, check), "Linear solution does not solve equation.");

    // Check that a singular matrix is identified.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);

    // On some some compilers in release mode this creation of a matrix and
    // than solving the linear system breaks if we don't first print the values.
    // I believe this somehow was tickling a compiler optimization bug.
    // But for now we will live with a bit more console output to work around
    // the issue
    PrintMatrix(singularMatrix);
    x = vtkm::SolveLinearSystem(singularMatrix, b, valid);
    //
    // We need to print the results of the SolveLinearSystem to screen to
    // make sure the compiler doesn't optimize out the operation which
    // previously was happening
    std::cout << "Result: " << x << std::endl;

    VTKM_TEST_ASSERT(!valid, "Expected matrix to be declared singular.");
  }

  static void Invert()
  {
    std::cout << "Invert a matrix." << std::endl;

    MatrixType A;
    NonSingularMatrix(A);
    bool valid;

    vtkm::Matrix<T, SIZE, SIZE> inverse = vtkm::MatrixInverse(A, valid);
    VTKM_TEST_ASSERT(valid, "Matrix declared singular?");

    // Check result.
    vtkm::Matrix<T, SIZE, SIZE> product = vtkm::MatrixMultiply(A, inverse);
    VTKM_TEST_ASSERT(test_equal(product, vtkm::MatrixIdentity<T, SIZE>()),
                     "Matrix inverse did not give identity.");

    // Check that a singular matrix is identified.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);
    vtkm::MatrixInverse(singularMatrix, valid);
    VTKM_TEST_ASSERT(!valid, "Expected matrix to be declared singular.");
  }

  static void Determinant()
  {
    std::cout << "Compute a determinant." << std::endl;

    MatrixType A;
    NonSingularMatrix(A);

    T determinant = vtkm::MatrixDeterminant(A);

    // Check result.
    T determinantCheck = RecursiveDeterminant(A);
    VTKM_TEST_ASSERT(test_equal(determinant, determinantCheck),
                     "Determinant computations do not agree.");

    // Check that a singular matrix has a zero determinant.
    MatrixType singularMatrix;
    SingularMatrix(singularMatrix);
    determinant = vtkm::MatrixDeterminant(singularMatrix);
    VTKM_TEST_ASSERT(test_equal(determinant, T(0.0)), "Non-zero determinant for singular matrix.");
  }

  static void Run()
  {
    std::cout << "-- " << SIZE << " x " << SIZE << std::endl;

    CheckMatrixSize();
    LUPFactor();
    SolveLinearSystem();
    Invert();
    Determinant();
  }

private:
  SquareMatrixTest() = delete;
};

struct MatrixTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    MatrixTest1<T, 1>();
    MatrixTest1<T, 2>();
    MatrixTest1<T, 3>();
    MatrixTest1<T, 4>();
    MatrixTest1<T, 5>();
  }
};

struct SquareMatrixTestFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    SquareMatrixTest<T, 1>::Run();
    SquareMatrixTest<T, 2>::Run();
    SquareMatrixTest<T, 3>::Run();
    SquareMatrixTest<T, 4>::Run();
    SquareMatrixTest<T, 5>::Run();
  }
};

struct VectorMultFunctor
{
  template <class VectorType>
  void operator()(const VectorType&) const
  {
    // This is mostly to make sure the compile can convert from Tuples
    // to vectors.
    const int SIZE = vtkm::VecTraits<VectorType>::NUM_COMPONENTS;
    using ComponentType = typename vtkm::VecTraits<VectorType>::ComponentType;

    vtkm::Matrix<ComponentType, SIZE, SIZE> matrix(0);
    VectorType inVec;
    VectorType outVec;
    for (vtkm::IdComponent index = 0; index < SIZE; index++)
    {
      matrix(index, index) = 1;
      inVec[index] = ComponentType(index + 1);
    }

    outVec = vtkm::MatrixMultiply(matrix, inVec);
    VTKM_TEST_ASSERT(test_equal(inVec, outVec), "Bad identity multiply.");

    outVec = vtkm::MatrixMultiply(inVec, matrix);
    VTKM_TEST_ASSERT(test_equal(inVec, outVec), "Bad identity multiply.");
  }
};

void TestMatrices()
{
  //  std::cout << "****** Rectangle tests" << std::endl;
  //  vtkm::testing::Testing::TryTypes(MatrixTestFunctor(),
  //                                   vtkm::TypeListScalarAll());

  std::cout << "****** Square tests" << std::endl;
  vtkm::testing::Testing::TryTypes(SquareMatrixTestFunctor(), vtkm::TypeListFieldScalar());

  //  std::cout << "***** Vector multiply tests" << std::endl;
  //  vtkm::testing::Testing::TryTypes(VectorMultFunctor(),
  //                                   vtkm::TypeListVecAll());
}

} // anonymous namespace

int UnitTestMatrix(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestMatrices, argc, argv);
}
