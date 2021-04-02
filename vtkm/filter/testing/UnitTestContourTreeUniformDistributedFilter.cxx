//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
// Copyright (c) 2018, The Regents of the University of California, through
// Lawrence Berkeley National Laboratory (subject to receipt of any required approvals
// from the U.S. Dept. of Energy).  All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// (1) Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
// (2) Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
// (3) Neither the name of the University of California, Lawrence Berkeley National
//     Laboratory, U.S. Dept. of Energy nor the names of its contributors may be
//     used to endorse or promote products derived from this software without
//     specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//
//=============================================================================
//
//  This code is an extension of the algorithm presented in the paper:
//  Parallel Peak Pruning for Scalable SMP Contour Tree Computation.
//  Hamish Carr, Gunther Weber, Christopher Sewell, and James Ahrens.
//  Proceedings of the IEEE Symposium on Large Data Analysis and Visualization
//  (LDAV), October 2016, Baltimore, Maryland.
//
//  The PPP2 algorithm and software were jointly developed by
//  Hamish Carr (University of Leeds), Gunther H. Weber (LBNL), and
//  Oliver Ruebel (LBNL)
//==============================================================================

#include "TestingContourTreeUniformDistributedFilter.h"

namespace
{
using vtkm::filter::testing::contourtree_uniform_distributed::TestContourTreeFile;
using vtkm::filter::testing::contourtree_uniform_distributed::
  TestContourTreeUniformDistributed5x6x7;
using vtkm::filter::testing::contourtree_uniform_distributed::TestContourTreeUniformDistributed8x9;

class TestContourTreeUniformDistributedFilter
{
public:
  void operator()() const
  {
    using vtkm::cont::testing::Testing;
    TestContourTreeUniformDistributed8x9(2);
    // TestContourTreeUniformDistributed8x9(3); CRASH???
    TestContourTreeUniformDistributed8x9(4);
    TestContourTreeUniformDistributed8x9(8);
    TestContourTreeUniformDistributed8x9(16);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        2);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        4);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        8);
    TestContourTreeFile(Testing::DataPath("rectilinear/vanc.vtk"),
                        "var",
                        Testing::RegressionImagePath("vanc.ct_txt"),
                        16);
    TestContourTreeUniformDistributed5x6x7(2, false);
    TestContourTreeUniformDistributed5x6x7(4, false);
    TestContourTreeUniformDistributed5x6x7(8, false);
    TestContourTreeUniformDistributed5x6x7(16, false);
    TestContourTreeUniformDistributed5x6x7(2, true);
    TestContourTreeUniformDistributed5x6x7(4, true);
    TestContourTreeUniformDistributed5x6x7(8, true);
    TestContourTreeUniformDistributed5x6x7(16, true);
  }
};
}

int UnitTestContourTreeUniformDistributedFilter(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestContourTreeUniformDistributedFilter(), argc, argv);
}
