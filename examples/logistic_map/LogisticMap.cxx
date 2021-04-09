//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <cmath>
#include <iostream>
#include <vector>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/io/ImageWriterPNG.h>
#include <vtkm/worklet/WorkletMapField.h>


// The logistic map is xᵢ₊₁ = rxᵢ(1-xᵢ).
// If we start this iteration out at (say) x₀ = 0.5, the map has "transients",
// which we must iterate away to produce the final image.
struct LogisticBurnIn : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(WholeArrayOut);
  using ExecutionSignature = void(_1, WorkIndex);

  template <typename OutputArrayPortalType>
  VTKM_EXEC void operator()(OutputArrayPortalType& outputArrayPortal, vtkm::Id workIndex) const
  {
    vtkm::Id width = outputArrayPortal.GetNumberOfValues();
    double rmin = 2.9;
    double r = rmin + (4.0 - rmin) * workIndex / (width - 1);
    double x = 0.5;
    // 2048 should be enough iterations to get rid of the transients:
    int n = 0;
    while (n++ < 2048)
    {
      x = r * x * (1 - x);
    }
    outputArrayPortal.Set(workIndex, x);
  }
};

// After burn-in, the iteration is periodic but in general not convergent,
// i.e., for large enough i, there exists an integer p > 0 such that
// xᵢ₊ₚ = xᵢ for all i.
// So color the pixels corresponding to xᵢ, xᵢ₊₁, .. xᵢ₊ₚ.
struct LogisticLimitPoints : public vtkm::worklet::WorkletMapField
{
  using ControlSignature = void(WholeArrayIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, WorkIndex);

  template <typename InputArrayPortalType, typename OutputArrayPortalType>
  VTKM_EXEC void operator()(const InputArrayPortalType& inputArrayPortal,
                            OutputArrayPortalType& outputArrayPortal,
                            vtkm::Id workIndex) const
  {
    vtkm::Id width = inputArrayPortal.GetNumberOfValues();
    double x = inputArrayPortal.Get(workIndex);
    double rmin = 2.9;
    double r = rmin + (4.0 - rmin) * workIndex / (width - 1);
    vtkm::Vec4f orange(1.0, 0.5, 0.0, 0.0);

    // We can't display need more limit points than pixels of height:
    vtkm::Id limit_points = 0;
    vtkm::Id height = 1800;
    while (limit_points++ < height)
    {
      vtkm::Id j = vtkm::Round(x * (height - 1));
      outputArrayPortal.Set(j * width + workIndex, orange);
      x = r * x * (1 - x);
    }
  }
};


int main()
{
  vtkm::Id height = 1800;
  vtkm::Id width = height * 1.618;
  vtkm::cont::DataSetBuilderUniform dsb;
  vtkm::cont::DataSet ds = dsb.Create(vtkm::Id2(width, height));

  vtkm::cont::ArrayHandle<double> x;
  x.Allocate(width);
  vtkm::cont::Invoker invoke;
  invoke(LogisticBurnIn{}, x);

  vtkm::cont::ArrayHandle<vtkm::Vec4f> pixels;
  pixels.Allocate(width * height);
  auto wp = pixels.WritePortal();
  for (vtkm::Id i = 0; i < pixels.GetNumberOfValues(); ++i)
  {
    wp.Set(i, vtkm::Vec4f(0, 0, 0, 0));
  }

  invoke(LogisticLimitPoints{}, x, pixels);
  std::string colorFieldName = "pixels";
  ds.AddPointField(colorFieldName, pixels);
  std::string filename = "logistic.png";
  vtkm::io::ImageWriterPNG writer(filename);
  writer.WriteDataSet(ds, colorFieldName);
  std::cout << "Now open " << filename << "\n";
}
