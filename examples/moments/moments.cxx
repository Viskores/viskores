//
// Created by ollie on 10/10/18.
//

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/io/reader/VTKDataSetReader.h>

#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <iostream>
#include <vtkm/io/writer/VTKDataSetWriter.h>

namespace detail
{

template <int... factors>
vtkm::Float64 eval_functor(vtkm::Float64 x, vtkm::Float64 y);

template <int factor, int... factors>
vtkm::Float64 eval_functor_impl(vtkm::Float64 x, vtkm::Float64 y)
{
  return eval_functor<factor>(x, y) * eval_functor<factors...>(x, y);
}

template <int... factors>
vtkm::Float64 eval_functor(vtkm::Float64 x, vtkm::Float64 y)
{
  return eval_functor_impl<factors...>(x, y);
}

template <>
vtkm::Float64 eval_functor<0>(vtkm::Float64 x, vtkm::Float64 y)
{
  return x;
}

template <>
vtkm::Float64 eval_functor<1>(vtkm::Float64 x, vtkm::Float64 y)
{
  return y;
}

template <>
vtkm::Float64 eval_functor<>(vtkm::Float64 x, vtkm::Float64 y)
{
  return 1;
}

} // detail

struct Monomial
{
public:
  Monomial(int _p, int _q)
    : p(_p)
    , q(_q)
  {
  }

  VTKM_EXEC
  vtkm::Float64 eval(vtkm::Float64 x, vtkm::Float64 y) const
  {
    // return detail::eval_functor<factors...>(x, y);
    return pow(x, p) * pow(y, q);
  }

private:
  int p;
  int q;
};

// FIXME: Why can't we pass the template parameter Radius to WorkletPointNeighborhood?
// TODO: figure out how to make Radius runtime configurable. We need to update
// VTKM's implementation of WorkletPointNeighborhood
template <int Radius>
struct Moments : public vtkm::worklet::WorkletPointNeighborhood<2>
{
public:
  Moments(int _p, int _q)
    : monomial(_p, _q)
  {
    assert(_p >= 0);
    assert(_q >= 0);
  }

  using ControlSignature = void(CellSetIn, FieldInNeighborhood<>, FieldOut<>);

  using ExecutionSignature = void(_2, _3);

  template <typename NeighIn>
  VTKM_EXEC void operator()(const NeighIn& image, vtkm::Float64& moment) const
  {
    vtkm::Float64 sum = 0;
    vtkm::Float64 recp = 1.0 / Radius;

    for (int j = -Radius; j <= Radius; j++)
    {
      for (int i = -Radius; i <= Radius; i++)
      {
        if (i * i + j * j <= Radius * Radius)
          // TODO: there is several order of magnitude of difference between this and the
          // the reference implementation. Scale by dx dy?
          sum += monomial.eval(i * recp, j * recp) * image.Get(i, j, 0);
      }
    }

    moment = sum;
  }

private:
  Monomial monomial;
};

int main(int argc, char* argv[])
{
  vtkm::io::reader::VTKDataSetReader reader(argv[1]);
  vtkm::cont::DataSet input = reader.ReadDataSet();

  auto field = input.GetPointField("values");
  vtkm::cont::ArrayHandle<vtkm::Float64> pixels;
  field.GetData().CopyTo(pixels);

  vtkm::cont::DataSet output;
  output.AddCellSet(input.GetCellSet(0));
  output.AddCoordinateSystem(input.GetCoordinateSystem(0));

  for (int order = 0; order <= 2; ++order)
  {
    for (int p = order; p >= 0; --p)
    {
      std::cout << "(" << p << ", " << order - p << ")" << std::endl;

      vtkm::cont::ArrayHandle<vtkm::Float64> moments;

      using DispatcherType = vtkm::worklet::DispatcherPointNeighborhood<Moments<2>>;
      DispatcherType dispatcher(Moments<2>{ p, order - p });
      dispatcher.SetDevice(vtkm::cont::DeviceAdapterTagSerial());
      dispatcher.Invoke(input.GetCellSet(0), pixels, moments);

      std::string fieldName = "moments_";
      fieldName += std::to_string(p) + std::to_string(order - p);

      vtkm::cont::Field momentsField(fieldName, vtkm::cont::Field::Association::POINTS, moments);
      output.AddField(momentsField);
    }
  }

  vtkm::io::writer::VTKDataSetWriter writer("moments_all.vtk");
  writer.WriteDataSet(output);
}