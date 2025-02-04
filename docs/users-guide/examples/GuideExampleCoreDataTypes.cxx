//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/Range.h>
#include <viskores/Types.h>
#include <viskores/VecVariable.h>

#include <viskores/testing/Testing.h>

namespace
{

void SimpleVectorTypes()
{
  ////
  //// BEGIN-EXAMPLE SimpleVectorTypes
  ////
  viskores::Vec2f A(1);         // A is (1, 1)
  A[1] = 3;                 // A is (1, 3) now
  viskores::Vec2f B = { 4, 5 }; // B is (4, 5)
  viskores::Vec2f C = A + B;    // C is (5, 8)
  viskores::FloatDefault manhattanDistance = C[0] + C[1];
  ////
  //// END-EXAMPLE SimpleVectorTypes
  ////

  VISKORES_TEST_ASSERT(test_equal(A, viskores::make_Vec(1, 3)));
  VISKORES_TEST_ASSERT(test_equal(B, viskores::make_Vec(4, 5)));
  VISKORES_TEST_ASSERT(test_equal(C, viskores::make_Vec(5, 8)));
  VISKORES_TEST_ASSERT(test_equal(manhattanDistance, 13));
}

void CreatingVectorTypes()
{
  ////
  //// BEGIN-EXAMPLE CreatingVectorTypes
  ////
  viskores::Vec3f_32 A{ 1 };                      // A is (1, 1, 1)
  A[1] = 2;                                   // A is now (1, 2, 1)
  viskores::Vec3f_32 B{ 1, 2, 3 };                // B is (1, 2, 3)
  viskores::Vec3f_32 C = viskores::make_Vec(3, 4, 5); // C is (3, 4, 5)
  // Longer Vecs specified with template.
  viskores::Vec<viskores::Float32, 5> D{ 1 };                 // D is (1, 1, 1, 1, 1)
  viskores::Vec<viskores::Float32, 5> E{ 1, 2, 3, 4, 5 };     // E is (1, 2, 3, 4, 5)
  viskores::Vec<viskores::Float32, 5> F = { 6, 7, 8, 9, 10 }; // F is (6, 7, 8, 9, 10)
  auto G = viskores::make_Vec(1, 3, 5, 7, 9);             // G is (1, 3, 5, 7, 9)
  ////
  //// END-EXAMPLE CreatingVectorTypes
  ////

  VISKORES_TEST_ASSERT((A[0] == 1) && (A[1] == 2) && (A[2] == 1),
                   "A is different than expected.");
  VISKORES_TEST_ASSERT((B[0] == 1) && (B[1] == 2) && (B[2] == 3),
                   "B is different than expected.");
  VISKORES_TEST_ASSERT((C[0] == 3) && (C[1] == 4) && (C[2] == 5),
                   "C is different than expected.");
  VISKORES_TEST_ASSERT((D[0] == 1) && (D[1] == 1) && (D[2] == 1) && (D[3] == 1) &&
                     (D[4] == 1),
                   "D is different than expected.");
  VISKORES_TEST_ASSERT((E[0] == 1) && (E[1] == 2) && (E[2] == 3) && (E[3] == 4) &&
                     (E[4] == 5),
                   "E is different than expected.");
  VISKORES_TEST_ASSERT((F[0] == 6) && (F[1] == 7) && (F[2] == 8) && (F[3] == 9) &&
                     (F[4] == 10),
                   "F is different than expected.");
  VISKORES_TEST_ASSERT((G[0] == 1) && (G[1] == 3) && (G[2] == 5) && (G[3] == 7) &&
                     (G[4] == 9),
                   "F is different than expected.");
}

void VectorOperations()
{
  ////
  //// BEGIN-EXAMPLE VectorOperations
  ////
  viskores::Vec3f_32 A{ 1, 2, 3 };
  viskores::Vec3f_32 B{ 4, 5, 6.5 };
  viskores::Vec3f_32 C = A + B;                 // C is (5, 7, 9.5)
  viskores::Vec3f_32 D = 2.0f * C;              // D is (10, 14, 19)
  viskores::Float32 s = viskores::Dot(A, B);        // s is 33.5
  bool b1 = (A == B);                       // b1 is false
  bool b2 = (A == viskores::make_Vec(1, 2, 3)); // b2 is true

  viskores::Vec<viskores::Float32, 5> E{ 1, 2.5, 3, 4, 5 };    // E is (1, 2, 3, 4, 5)
  viskores::Vec<viskores::Float32, 5> F{ 6, 7, 8.5, 9, 10.5 }; // F is (6, 7, 8, 9, 10)
  viskores::Vec<viskores::Float32, 5> G = E + F;               // G is (7, 9.5, 11.5, 13, 15.5)
  bool b3 = (E == F);                                  // b3 is false
  bool b4 = (G == viskores::make_Vec(7.f, 9.5f, 11.5f, 13.f, 15.5f)); // b4 is true
  ////
  //// END-EXAMPLE VectorOperations
  ////

  VISKORES_TEST_ASSERT(test_equal(C, viskores::Vec3f_32(5, 7, 9.5)), "C is wrong");
  VISKORES_TEST_ASSERT(test_equal(D, viskores::Vec3f_32(10, 14, 19)), "D is wrong");
  VISKORES_TEST_ASSERT(test_equal(s, 33.5), "s is wrong");
  VISKORES_TEST_ASSERT(!b1, "b1 is wrong");
  VISKORES_TEST_ASSERT(b2, "b2 is wrong");
  VISKORES_TEST_ASSERT(!b3, "b3 is wrong");
  VISKORES_TEST_ASSERT(b4, "b4 is wrong");
}

void EquilateralTriangle()
{
  ////
  //// BEGIN-EXAMPLE EquilateralTriangle
  ////
  viskores::Vec<viskores::Vec2f_32, 3> equilateralTriangle = { { 0.0f, 0.0f },
                                                       { 1.0f, 0.0f },
                                                       { 0.5f, 0.8660254f } };
  ////
  //// END-EXAMPLE EquilateralTriangle
  ////

  viskores::Float32 edgeLengthSqr = 1.0;
  viskores::Vec<viskores::Vec2f_32, 3> edges(equilateralTriangle[1] - equilateralTriangle[0],
                                     equilateralTriangle[2] - equilateralTriangle[0],
                                     equilateralTriangle[2] - equilateralTriangle[1]);
  VISKORES_TEST_ASSERT(test_equal(viskores::Dot(edges[0], edges[0]), edgeLengthSqr),
                   "Bad edge length.");
  VISKORES_TEST_ASSERT(test_equal(viskores::Dot(edges[1], edges[1]), edgeLengthSqr),
                   "Bad edge length.");
  VISKORES_TEST_ASSERT(test_equal(viskores::Dot(edges[2], edges[2]), edgeLengthSqr),
                   "Bad edge length.");
}

////
//// BEGIN-EXAMPLE VecCExample
////
VISKORES_EXEC viskores::VecCConst<viskores::IdComponent> HexagonIndexToIJK(viskores::IdComponent index)
{
  static const viskores::IdComponent HexagonIndexToIJKTable[8][3] = {
    { 0, 0, 0 }, { 1, 0, 0 }, { 1, 1, 0 }, { 0, 1, 0 },
    { 0, 0, 1 }, { 1, 0, 1 }, { 1, 1, 1 }, { 0, 1, 1 }
  };

  return viskores::make_VecC(HexagonIndexToIJKTable[index], 3);
}

VISKORES_EXEC viskores::IdComponent HexagonIJKToIndex(viskores::VecCConst<viskores::IdComponent> ijk)
{
  static const viskores::IdComponent HexagonIJKToIndexTable[2][2][2] = { {
                                                                       // i=0
                                                                       { 0, 4 }, // j=0
                                                                       { 3, 7 }, // j=1
                                                                     },
                                                                     {
                                                                       // i=1
                                                                       { 1, 5 }, // j=0
                                                                       { 2, 6 }, // j=1
                                                                     } };

  return HexagonIJKToIndexTable[ijk[0]][ijk[1]][ijk[2]];
}
////
//// END-EXAMPLE VecCExample
////

////
//// BEGIN-EXAMPLE VecVariableExample
////
viskores::VecVariable<viskores::IdComponent, 4> HexagonShortestPath(viskores::IdComponent startPoint,
                                                            viskores::IdComponent endPoint)
{
  viskores::VecCConst<viskores::IdComponent> startIJK = HexagonIndexToIJK(startPoint);
  viskores::VecCConst<viskores::IdComponent> endIJK = HexagonIndexToIJK(endPoint);

  viskores::IdComponent3 currentIJK;
  startIJK.CopyInto(currentIJK);

  viskores::VecVariable<viskores::IdComponent, 4> path;
  path.Append(startPoint);
  for (viskores::IdComponent dimension = 0; dimension < 3; dimension++)
  {
    if (currentIJK[dimension] != endIJK[dimension])
    {
      currentIJK[dimension] = endIJK[dimension];
      path.Append(HexagonIJKToIndex(currentIJK));
    }
  }

  return path;
}
////
//// END-EXAMPLE VecVariableExample
////

void UsingVecCAndVecVariable()
{
  viskores::VecVariable<viskores::IdComponent, 4> path;

  path = HexagonShortestPath(2, 2);
  VISKORES_TEST_ASSERT(test_equal(path, viskores::Vec<viskores::IdComponent, 1>(2)), "Bad path");

  path = HexagonShortestPath(0, 7);
  VISKORES_TEST_ASSERT(test_equal(path, viskores::IdComponent3(0, 3, 7)), "Bad path");

  path = HexagonShortestPath(5, 3);
  VISKORES_TEST_ASSERT(test_equal(path, viskores::IdComponent4(5, 4, 7, 3)), "Bad path");
}

void UsingRange()
{
  ////
  //// BEGIN-EXAMPLE UsingRange
  ////
  viskores::Range range;            // default constructor is empty range
  bool b1 = range.IsNonEmpty(); // b1 is false

  range.Include(0.5);            // range now is [0.5 .. 0.5]
  bool b2 = range.IsNonEmpty();  // b2 is true
  bool b3 = range.Contains(0.5); // b3 is true
  bool b4 = range.Contains(0.6); // b4 is false

  range.Include(2.0);            // range is now [0.5 .. 2]
  bool b5 = range.Contains(0.5); // b3 is true
  bool b6 = range.Contains(0.6); // b4 is true

  range.Include(viskores::Range(-1, 1)); // range is now [-1 .. 2]
  //// PAUSE-EXAMPLE
  VISKORES_TEST_ASSERT(test_equal(range, viskores::Range(-1, 2)), "Bad range");
  //// RESUME-EXAMPLE

  range.Include(viskores::Range(3, 4)); // range is now [-1 .. 4]
  //// PAUSE-EXAMPLE
  VISKORES_TEST_ASSERT(test_equal(range, viskores::Range(-1, 4)), "Bad range");
  //// RESUME-EXAMPLE

  viskores::Float64 lower = range.Min;       // lower is -1
  viskores::Float64 upper = range.Max;       // upper is 4
  viskores::Float64 length = range.Length(); // length is 5
  viskores::Float64 center = range.Center(); // center is 1.5
  ////
  //// END-EXAMPLE UsingRange
  ////

  VISKORES_TEST_ASSERT(!b1, "Bad non empty.");
  VISKORES_TEST_ASSERT(b2, "Bad non empty.");
  VISKORES_TEST_ASSERT(b3, "Bad contains.");
  VISKORES_TEST_ASSERT(!b4, "Bad contains.");
  VISKORES_TEST_ASSERT(b5, "Bad contains.");
  VISKORES_TEST_ASSERT(b6, "Bad contains.");

  VISKORES_TEST_ASSERT(test_equal(lower, -1), "Bad lower");
  VISKORES_TEST_ASSERT(test_equal(upper, 4), "Bad upper");
  VISKORES_TEST_ASSERT(test_equal(length, 5), "Bad length");
  VISKORES_TEST_ASSERT(test_equal(center, 1.5), "Bad center");
}

void UsingBounds()
{
  ////
  //// BEGIN-EXAMPLE UsingBounds
  ////
  viskores::Bounds bounds;           // default constructor makes empty
  bool b1 = bounds.IsNonEmpty(); // b1 is false

  bounds.Include(viskores::make_Vec(0.5, 2.0, 0.0));            // bounds contains only
                                                            // the point [0.5, 2, 0]
  bool b2 = bounds.IsNonEmpty();                            // b2 is true
  bool b3 = bounds.Contains(viskores::make_Vec(0.5, 2.0, 0.0)); // b3 is true
  bool b4 = bounds.Contains(viskores::make_Vec(1, 1, 1));       // b4 is false
  bool b5 = bounds.Contains(viskores::make_Vec(0, 0, 0));       // b5 is false

  bounds.Include(viskores::make_Vec(4, -1, 2)); // bounds is region [0.5 .. 4] in X,
                                            //                  [-1 .. 2] in Y,
                                            //              and [0 .. 2] in Z
  //// PAUSE-EXAMPLE
  VISKORES_TEST_ASSERT(test_equal(bounds, viskores::Bounds(0.5, 4, -1, 2, 0, 2)), "");
  //// RESUME-EXAMPLE
  bool b6 = bounds.Contains(viskores::make_Vec(0.5, 2.0, 0.0)); // b6 is true
  bool b7 = bounds.Contains(viskores::make_Vec(1, 1, 1));       // b7 is true
  bool b8 = bounds.Contains(viskores::make_Vec(0, 0, 0));       // b8 is false

  viskores::Bounds otherBounds(viskores::make_Vec(0, 0, 0), viskores::make_Vec(3, 3, 3));
  // otherBounds is region [0 .. 3] in X, Y, and Z
  bounds.Include(otherBounds); // bounds is now region [0 .. 4] in X,
                               //                      [-1 .. 3] in Y,
                               //                  and [0 .. 3] in Z
  //// PAUSE-EXAMPLE
  VISKORES_TEST_ASSERT(test_equal(bounds, viskores::Bounds(0, 4, -1, 3, 0, 3)), "");
  //// RESUME-EXAMPLE

  viskores::Vec3f_64 lower(bounds.X.Min, bounds.Y.Min, bounds.Z.Min);
  // lower is [0, -1, 0]
  viskores::Vec3f_64 upper(bounds.X.Max, bounds.Y.Max, bounds.Z.Max);
  // upper is [4, 3, 3]

  viskores::Vec3f_64 center = bounds.Center(); // center is [2, 1, 1.5]
  ////
  //// END-EXAMPLE UsingBounds
  ////

  VISKORES_TEST_ASSERT(!b1, "Bad non empty.");
  VISKORES_TEST_ASSERT(b2, "Bad non empty.");
  VISKORES_TEST_ASSERT(b3, "Bad contains.");
  VISKORES_TEST_ASSERT(!b4, "Bad contains.");
  VISKORES_TEST_ASSERT(!b5, "Bad contains.");
  VISKORES_TEST_ASSERT(b6, "Bad contains.");
  VISKORES_TEST_ASSERT(b7, "Bad contains.");
  VISKORES_TEST_ASSERT(!b8, "Bad contains.");
  VISKORES_TEST_ASSERT(test_equal(lower, viskores::make_Vec(0, -1, 0)), "");
  VISKORES_TEST_ASSERT(test_equal(upper, viskores::make_Vec(4, 3, 3)), "");
  VISKORES_TEST_ASSERT(test_equal(center, viskores::make_Vec(2.0, 1.0, 1.5)), "");
}

void Test()
{
  SimpleVectorTypes();
  CreatingVectorTypes();
  VectorOperations();
  EquilateralTriangle();
  UsingVecCAndVecVariable();
  UsingRange();
  UsingBounds();
}

} // anonymous namespace

int GuideExampleCoreDataTypes(int argc, char* argv[])
{
  return viskores::testing::Testing::Run(Test, argc, argv);
}
