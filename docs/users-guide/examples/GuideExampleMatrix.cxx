//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================

#include <viskores/Matrix.h>

#include <viskores/testing/Testing.h>

namespace
{

VISKORES_CONT
void BuildMatrix()
{
  std::cout << "Building matrix containing " << std::endl
            << "|  0  1  2 |" << std::endl
            << "| 10 11 12 |" << std::endl;

  ////
  //// BEGIN-EXAMPLE BuildMatrix
  ////
  viskores::Matrix<viskores::Float32, 2, 3> matrix;

  // Using parenthesis notation.
  matrix(0, 0) = 0.0f;
  matrix(0, 1) = 1.0f;
  matrix(0, 2) = 2.0f;

  // Using bracket notation.
  matrix[1][0] = 10.0f;
  matrix[1][1] = 11.0f;
  matrix[1][2] = 12.0f;
  ////
  //// END-EXAMPLE BuildMatrix
  ////

  viskores::Vec2f_32 termVec(1.0f, 0.1f);
  viskores::Vec3f_32 multVec = viskores::MatrixMultiply(termVec, matrix);
  //  std::cout << multVec << std::endl;
  VISKORES_TEST_ASSERT(test_equal(multVec, viskores::make_Vec(1.0, 2.1, 3.2)),
                       "Unexpected product.");
}

void Run()
{
  BuildMatrix();
}

} // anonymous namespace

int GuideExampleMatrix(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(Run, argc, argv);
}
