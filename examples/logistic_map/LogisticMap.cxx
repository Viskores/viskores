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
template <typename T>
class LogisticBurnIn : public vtkm::worklet::WorkletMapField
{
public:
  LogisticBurnIn(T rmin, vtkm::Id width)
    : rmin_(rmin)
    , width_(width)
  {
  }

  using ControlSignature = void(FieldOut);
  using ExecutionSignature = _1(WorkIndex);

  VTKM_EXEC T operator()(vtkm::Id workIndex) const
  {
    T r = rmin_ + (4.0 - rmin_) * workIndex / (width_ - 1);
    T x = 0.5;
    // 2048 should be enough iterations to get rid of the transients:
    int n = 0;
    while (n++ < 2048)
    {
      x = r * x * (1 - x);
    }
    return x;
  }

private:
  T rmin_;
  vtkm::Id width_;
};

// After burn-in, the iteration is periodic but in general not convergent,
// i.e., for large enough i, there exists an integer p > 0 such that
// xᵢ₊ₚ = xᵢ for all i.
// So color the pixels corresponding to xᵢ, xᵢ₊₁, .. xᵢ₊ₚ.
template <typename T>
class LogisticLimitPoints : public vtkm::worklet::WorkletMapField
{
public:
  LogisticLimitPoints(T rmin, vtkm::Id width, vtkm::Id height)
    : rmin_(rmin)
    , width_(width)
    , height_(height)
  {
    orange_ = vtkm::Vec4f(1.0, 0.5, 0.0, 0.0);
  }

  using ControlSignature = void(FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, WorkIndex);

  template <typename OutputArrayPortalType>
  VTKM_EXEC void operator()(T x, OutputArrayPortalType& outputArrayPortal, vtkm::Id workIndex) const
  {
    T r = rmin_ + (4.0 - rmin_) * workIndex / (width_ - 1);
    // We can't display need more limit points than pixels of height:
    vtkm::Id limit_points = 0;
    while (limit_points++ < height_)
    {
      vtkm::Id j = vtkm::Round(x * (height_ - 1));
      outputArrayPortal.Set(j * width_ + workIndex, orange_);
      x = r * x * (1 - x);
    }
  }

private:
  T rmin_;
  vtkm::Id width_;
  vtkm::Id height_;
  vtkm::Vec4f orange_;
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
  double rmin = 2.9;
  auto burnIn = LogisticBurnIn<double>(rmin, width);
  invoke(burnIn, x);

  vtkm::cont::ArrayHandle<vtkm::Vec4f> pixels;
  pixels.Allocate(width * height);
  auto wp = pixels.WritePortal();
  for (vtkm::Id i = 0; i < pixels.GetNumberOfValues(); ++i)
  {
    wp.Set(i, vtkm::Vec4f(0, 0, 0, 0));
  }
  auto limitPoints = LogisticLimitPoints<double>(rmin, width, height);

  invoke(limitPoints, x, pixels);
  std::string colorFieldName = "pixels";
  ds.AddPointField(colorFieldName, pixels);
  std::string filename = "logistic.png";
  vtkm::io::ImageWriterPNG writer(filename);
  writer.WriteDataSet(ds, colorFieldName);
  std::cout << "Now open " << filename << "\n";
}
