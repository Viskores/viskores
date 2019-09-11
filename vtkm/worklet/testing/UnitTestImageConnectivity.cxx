//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/cont/testing/MakeTestDataSet.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/Contour.h>

#include <vtkm/worklet/connectivities/ImageConnectivity.h>

class TestImageConnectivity
{
public:
  using Algorithm = vtkm::cont::Algorithm;

  void operator()() const
  {
    CCL_CUDA8x4();
    CCL_CUDA8x8();
  }

  void CCL_CUDA8x4() const
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

    auto colorField = vtkm::cont::make_FieldPoint("color", vtkm::cont::make_ArrayHandle(pixels));
    data.AddField(colorField);

    vtkm::cont::ArrayHandle<vtkm::Id> component;
    vtkm::worklet::connectivity::ImageConnectivity().Run(
      data.GetCellSet().Cast<vtkm::cont::CellSetStructured<2>>(), colorField.GetData(), component);

    std::vector<vtkm::Id> componentExpected = { 0, 1, 2, 1, 1, 3, 3, 4, 0, 1, 1, 1, 3, 3, 3, 4,
                                                1, 1, 3, 3, 3, 4, 3, 4, 1, 1, 3, 3, 4, 4, 4, 4 };


    std::size_t i = 0;
    for (vtkm::Id index = 0; index < component.GetNumberOfValues(); index++, i++)
    {
      VTKM_TEST_ASSERT(component.GetPortalConstControl().Get(index) == componentExpected[i],
                       "Components has unexpected value.");
    }
  }

  void CCL_CUDA8x8() const
  {
    // example from Figure 35.7 of Connected Component Labeling in CUDA by OndˇrejˇŚtava,
    // Bedˇrich Beneˇ
    std::vector<vtkm::UInt8> pixels{
      0, 1, 1, 1, 0, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1,
      1, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1,
      1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0,
    };

    vtkm::cont::DataSetBuilderUniform builder;
    vtkm::cont::DataSet data = builder.Create(vtkm::Id3(8, 8, 1));

    auto colorField =
      vtkm::cont::make_Field("color", vtkm::cont::Field::Association::POINTS, pixels);
    data.AddField(colorField);

    vtkm::cont::ArrayHandle<vtkm::Id> component;
    vtkm::worklet::connectivity::ImageConnectivity().Run(
      data.GetCellSet().Cast<vtkm::cont::CellSetStructured<2>>(), colorField.GetData(), component);

    std::vector<vtkm::UInt8> componentExpected = { 0, 1, 1, 1, 0, 1, 1, 2, 0, 0, 0, 1, 0, 1, 1, 2,
                                                   0, 1, 1, 0, 0, 1, 1, 2, 0, 1, 0, 0, 0, 1, 1, 2,
                                                   0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1,
                                                   0, 1, 0, 1, 1, 1, 3, 3, 0, 1, 1, 1, 1, 1, 3, 3 };

    for (vtkm::Id i = 0; i < component.GetNumberOfValues(); ++i)
    {
      VTKM_TEST_ASSERT(component.GetPortalConstControl().Get(i) == componentExpected[size_t(i)],
                       "Components has unexpected value.");
    }
  }
};


int UnitTestImageConnectivity(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestImageConnectivity(), argc, argv);
}
