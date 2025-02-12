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
#include <viskores/cont/DataSetBuilderUniform.h>
#include <viskores/io/ImageWriterPNG.h>
#include <viskores/worklet/WorkletMapField.h>


// The logistic map is xᵢ₊₁ = rxᵢ(1-xᵢ).
// If we start this iteration out at (say) x₀ = 0.5, the map has "transients",
// which we must iterate away to produce the final image.
template <typename T>
class LogisticBurnIn : public viskores::worklet::WorkletMapField
{
public:
  LogisticBurnIn(T rmin, viskores::Id width)
    : rmin_(rmin)
    , width_(width)
  {
  }

  using ControlSignature = void(FieldOut);
  using ExecutionSignature = _1(WorkIndex);

  VISKORES_EXEC T operator()(viskores::Id workIndex) const
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
  viskores::Id width_;
};

// After burn-in, the iteration is periodic but in general not convergent,
// i.e., for large enough i, there exists an integer p > 0 such that
// xᵢ₊ₚ = xᵢ for all i.
// So color the pixels corresponding to xᵢ, xᵢ₊₁, .. xᵢ₊ₚ.
template <typename T>
class LogisticLimitPoints : public viskores::worklet::WorkletMapField
{
public:
  LogisticLimitPoints(T rmin, viskores::Id width, viskores::Id height)
    : rmin_(rmin)
    , width_(width)
    , height_(height)
  {
    orange_ = viskores::Vec4f(1.0, 0.5, 0.0, 0.0);
  }

  using ControlSignature = void(FieldIn, WholeArrayOut);
  using ExecutionSignature = void(_1, _2, WorkIndex);

  template <typename OutputArrayPortalType>
  VISKORES_EXEC void operator()(T x, OutputArrayPortalType& outputArrayPortal, viskores::Id workIndex) const
  {
    T r = rmin_ + (4.0 - rmin_) * workIndex / (width_ - 1);
    // We can't display need more limit points than pixels of height:
    viskores::Id limit_points = 0;
    while (limit_points++ < height_)
    {
      viskores::Id j = viskores::Round(x * (height_ - 1));
      outputArrayPortal.Set(j * width_ + workIndex, orange_);
      x = r * x * (1 - x);
    }
  }

private:
  T rmin_;
  viskores::Id width_;
  viskores::Id height_;
  viskores::Vec4f orange_;
};


int main()
{
  viskores::Id height = 1800;
  viskores::Id width = height * 1.618;
  viskores::cont::DataSetBuilderUniform dsb;
  viskores::cont::DataSet ds = dsb.Create(viskores::Id2(width, height));

  viskores::cont::ArrayHandle<double> x;
  x.Allocate(width);
  viskores::cont::Invoker invoke;
  double rmin = 2.9;
  auto burnIn = LogisticBurnIn<double>(rmin, width);
  invoke(burnIn, x);

  viskores::cont::ArrayHandle<viskores::Vec4f> pixels;
  pixels.Allocate(width * height);
  auto wp = pixels.WritePortal();
  for (viskores::Id i = 0; i < pixels.GetNumberOfValues(); ++i)
  {
    wp.Set(i, viskores::Vec4f(0, 0, 0, 0));
  }
  auto limitPoints = LogisticLimitPoints<double>(rmin, width, height);

  invoke(limitPoints, x, pixels);
  std::string colorFieldName = "pixels";
  ds.AddPointField(colorFieldName, pixels);
  std::string filename = "logistic.png";
  viskores::io::ImageWriterPNG writer(filename);
  writer.WriteDataSet(ds, colorFieldName);
  std::cout << "Now open " << filename << "\n";
}
