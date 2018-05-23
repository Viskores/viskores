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
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/MarchingCubes.h>

#include <vtkm/worklet/connectivities/ImageConnectivity.h>


template <typename DeviceAdapter>
class TestImageConnectivity
{
public:
  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  void operator()() const
  {
    // example image from Connected Component Labeling in CUDA by OndˇrejˇŚtava,
    // Bedˇrich Beneˇ
    std::vector<vtkm::UInt8> pixels(8 * 4, 0);
    pixels[3] = pixels[4] = pixels[10] = pixels[11] = 1;
    pixels[1] = pixels[9] = pixels[16] = pixels[17] = pixels[24] = pixels[25] = 1;
    pixels[7] = pixels[15] = pixels[21] = pixels[23] = pixels[28] = pixels[29] = pixels[30] =
      pixels[31] = 1;

    vtkm::cont::DataSetBuilderUniform builder;
    vtkm::cont::DataSet data = builder.Create(vtkm::Id3(8, 4, 1));

    auto colorField =
      vtkm::cont::make_Field("color", vtkm::cont::Field::Association::POINTS, pixels);
    data.AddField(colorField);

    vtkm::cont::ArrayHandle<vtkm::Id> component;
    vtkm::worklet::connectivity::ImageConnectivity().Run(
      data.GetCellSet(0).Cast<vtkm::cont::CellSetStructured<2>>(),
      colorField.GetData(),
      component,
      DeviceAdapter());

    std::vector<vtkm::Id> componentExpected = { 0, 1, 2, 1, 1, 3, 3, 4, 0, 1, 1, 1, 3, 3, 3, 4,
                                                1, 1, 3, 3, 3, 4, 3, 4, 1, 1, 3, 3, 4, 4, 4, 4 };

    std::size_t i = 0;
    for (vtkm::Id index = 0; index < component.GetNumberOfValues(); index++, i++)
    {
      VTKM_TEST_ASSERT(component.GetPortalConstControl().Get(index) == componentExpected[i],
                       "Components has unexpected value.");
    }
  }
};

int UnitTestImageConnectivity(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(
    TestImageConnectivity<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>());
}
