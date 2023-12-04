//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_cont_testing_MakeTestDataSet_h
#define vtk_m_cont_testing_MakeTestDataSet_h

// The relative path of Testing.h is unknown, the only thing that we can assume
// is that it is located in the same directory as this header file. This is
// because the testing directory is reserved for test executables and not
// libraries, the vtkm_cont_testing module has to put this file in
// vtkm/cont/testlib instead of vtkm/cont/testing where you normally would
// expect it.
#include "Testing.h"

#include <vtkm/cont/DataSet.h>

#include <vtkm/cont/testlib/vtkm_cont_testing_export.h>

#include <numeric>

namespace vtkm
{
namespace cont
{
namespace testing
{

class VTKM_CONT_TESTING_EXPORT MakeTestDataSet
{
public:
  // 1D uniform datasets.
  vtkm::cont::DataSet Make1DUniformDataSet0();
  vtkm::cont::DataSet Make1DUniformDataSet1();
  vtkm::cont::DataSet Make1DUniformDataSet2();

  // 1D explicit datasets.
  vtkm::cont::DataSet Make1DExplicitDataSet0();

  // 2D uniform datasets.
  vtkm::cont::DataSet Make2DUniformDataSet0();
  vtkm::cont::DataSet Make2DUniformDataSet1();
  vtkm::cont::DataSet Make2DUniformDataSet2();
  vtkm::cont::DataSet Make2DUniformDataSet3();

  // 3D uniform datasets.
  vtkm::cont::DataSet Make3DUniformDataSet0();
  vtkm::cont::DataSet Make3DUniformDataSet1();
  vtkm::cont::DataSet Make3DUniformDataSet2();
  vtkm::cont::DataSet Make3DUniformDataSet3(vtkm::Id3 dims = vtkm::Id3(10));
  vtkm::cont::DataSet Make3DUniformDataSet4();
  vtkm::cont::DataSet Make3DRegularDataSet0();
  vtkm::cont::DataSet Make3DRegularDataSet1();

  //2D rectilinear
  vtkm::cont::DataSet Make2DRectilinearDataSet0();

  //3D rectilinear
  vtkm::cont::DataSet Make3DRectilinearDataSet0();

  // 2D explicit datasets.
  vtkm::cont::DataSet Make2DExplicitDataSet0();

  // 3D explicit datasets.
  vtkm::cont::DataSet Make3DExplicitDataSet0();
  vtkm::cont::DataSet Make3DExplicitDataSet1();
  vtkm::cont::DataSet Make3DExplicitDataSet2();
  vtkm::cont::DataSet Make3DExplicitDataSet3();
  vtkm::cont::DataSet Make3DExplicitDataSet4();
  vtkm::cont::DataSet Make3DExplicitDataSet5();
  vtkm::cont::DataSet Make3DExplicitDataSet6();
  vtkm::cont::DataSet Make3DExplicitDataSet7();
  vtkm::cont::DataSet Make3DExplicitDataSet8();
  vtkm::cont::DataSet Make3DExplicitDataSetZoo();
  vtkm::cont::DataSet Make3DExplicitDataSetPolygonal();
  vtkm::cont::DataSet Make3DExplicitDataSetCowNose();
};

} // namespace testing
} // namespace cont
} // namespace vtkm

#endif //vtk_m_cont_testing_MakeTestDataSet_h
