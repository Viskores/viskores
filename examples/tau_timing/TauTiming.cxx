//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <vtkm/filter/MarchingCubes.h>
#include <vtkm/io/reader/VTKDataSetReader.h>
#include <vtkm/io/writer/VTKDataSetWriter.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/worklet/ExternalFaces.h>
#include <vtkm/worklet/StreamLineUniformGrid.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DataSetFieldAdd.h>
#include <vtkm/cont/Initiailize.h>

#include <vtkm/cont/testing/Testing.h>
#include <vtkm/rendering/Actor.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/MapperRayTracer.h>
#include <vtkm/rendering/MapperVolume.h>
#include <vtkm/rendering/Scene.h>
#include <vtkm/rendering/View3D.h>
#include <vtkm/rendering/testing/RenderTest.h>

#include <vtkm/worklet/StreamLineUniformGrid.h>

#include <vtkm/cont/testing/MakeTestDataSet.h>

#include <vtkm/io/writer/VTKDataSetWriter.h>

static bool printProgress = true;

template <typename T>
VTKM_EXEC_CONT vtkm::Vec<T, 3> Normalize(vtkm::Vec<T, 3> v)
{
  T magnitude = static_cast<T>(sqrt(vtkm::dot(v, v)));
  T zero = static_cast<T>(0.0);
  T one = static_cast<T>(1.0);
  if (magnitude == zero)
    return vtkm::make_Vec(zero, zero, zero);
  else
    return one / magnitude * v;
}

class TangleField : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn vertexId, FieldOut v);
  using ExecutionSignature = void(_1, _2);
  using InputDomain = _1;

  const vtkm::Id xdim, ydim, zdim;
  const vtkm::FloatDefault xmin, ymin, zmin, xmax, ymax, zmax;
  const vtkm::Id cellsPerLayer;

  VTKM_CONT
  TangleField(const vtkm::Id3 dims,
              const vtkm::FloatDefault mins[3],
              const vtkm::FloatDefault maxs[3])
    : xdim(dims[0])
    , ydim(dims[1])
    , zdim(dims[2])
    , xmin(mins[0])
    , ymin(mins[1])
    , zmin(mins[2])
    , xmax(maxs[0])
    , ymax(maxs[1])
    , zmax(maxs[2])
    , cellsPerLayer((xdim) * (ydim))
  {
  }

  VTKM_EXEC
  void operator()(const vtkm::Id& vertexId, vtkm::Float32& v) const
  {
    const vtkm::Id x = vertexId % (xdim);
    const vtkm::Id y = (vertexId / (xdim)) % (ydim);
    const vtkm::Id z = vertexId / cellsPerLayer;

    const vtkm::FloatDefault fx =
      static_cast<vtkm::FloatDefault>(x) / static_cast<vtkm::FloatDefault>(xdim - 1);
    const vtkm::FloatDefault fy =
      static_cast<vtkm::FloatDefault>(y) / static_cast<vtkm::FloatDefault>(xdim - 1);
    const vtkm::FloatDefault fz =
      static_cast<vtkm::FloatDefault>(z) / static_cast<vtkm::FloatDefault>(xdim - 1);

    const vtkm::Float32 xx = 3.0f * vtkm::Float32(xmin + (xmax - xmin) * (fx));
    const vtkm::Float32 yy = 3.0f * vtkm::Float32(ymin + (ymax - ymin) * (fy));
    const vtkm::Float32 zz = 3.0f * vtkm::Float32(zmin + (zmax - zmin) * (fz));

    v = (xx * xx * xx * xx - 5.0f * xx * xx + yy * yy * yy * yy - 5.0f * yy * yy +
         zz * zz * zz * zz - 5.0f * zz * zz + 11.8f) *
        0.2f +
      0.5f;
  }
};

vtkm::cont::DataSet CreateTestDataSet(vtkm::Id3 dims)
{
  vtkm::cont::DataSet dataSet;

  const vtkm::Id3 vdims(dims[0] + 1, dims[1] + 1, dims[2] + 1);

  vtkm::FloatDefault mins[3] = { -1.0f, -1.0f, -1.0f };
  vtkm::FloatDefault maxs[3] = { 1.0f, 1.0f, 1.0f };


  static const vtkm::IdComponent ndim = 3;
  vtkm::cont::CellSetStructured<ndim> cellSet("cells");
  cellSet.SetPointDimensions(vdims);
  dataSet.AddCellSet(cellSet);

  vtkm::Vec<vtkm::FloatDefault, 3> origin(0.0f, 0.0f, 0.0f);
  vtkm::Vec<vtkm::FloatDefault, 3> spacing(1.0f / static_cast<vtkm::FloatDefault>(dims[0]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[2]),
                                           1.0f / static_cast<vtkm::FloatDefault>(dims[1]));

  vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims, origin, spacing);
  dataSet.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));

  vtkm::cont::ArrayHandle<vtkm::Float32> scalarVar;
  vtkm::cont::ArrayHandleIndex vertexCountImplicitArray(vdims[0] * vdims[1] * vdims[2]);
  vtkm::worklet::DispatcherMapField<TangleField> tangleFieldDispatcher(
    TangleField(vdims, mins, maxs));
  tangleFieldDispatcher.Invoke(vertexCountImplicitArray, scalarVar);

  dataSet.AddField(
    vtkm::cont::Field(std::string("scalar"), vtkm::cont::Field::Association::POINTS, scalarVar));

  return dataSet;
}

void TestMarchingCubesUniformGrid(int d)
{
  vtkm::Id3 dims(d, d, d);
  std::cout << "Marching cubes with gridsize = " << dims << std::endl;

  vtkm::cont::DataSet dataSet = CreateTestDataSet(dims);

  //vtkm::io::writer::VTKDataSetWriter wrt("ds.vtk");
  //wrt.WriteDataSet(dataSet);

  int N = 100;
  vtkm::Float32 v0 = -.8f, v1 = 24.0f;
  vtkm::Float32 dv = (v1 - v0) / (vtkm::Float32)(N - 1);

  for (int i = 0; i < N; i++)
  {
    vtkm::filter::ResultDataSet result;
    vtkm::filter::MarchingCubes mc;
    mc.SetGenerateNormals(true);

    vtkm::Float32 val = v0 + i * dv;
    //std::cout<<i<<": "<<val<<std::endl;
    if (N == 1)
      val = 0.5f;
    mc.SetIsoValue(val);
    //mc.SetMergeDuplicatePoints(false);
    result = mc.Execute(dataSet, dataSet.GetField("nodevar"));
    const vtkm::cont::DataSet& out = result.GetDataSet();
  }
  //std::cout<<"Number of points in isosurface: "<<out.GetCoordinateSystem().GetData().GetNumberOfValues()<<std::endl;
}

void TestStreamlineUniformGrid(int d)
{
  /*
    vtkm::Id3 dims(d,d,d);
    std::cout<<"Streamline with gridsize = "<<dims<<std::endl;
    vtkm::cont::DataSet dataSet = MakeIsosurfaceTestDataSet(dims);

    vtkm::cont::ArrayHandle< vtkm::Vec<vtkm::Float32,3> > result;

    PointGrad<vtkm::Float32> func(dataSet, "nodevar", result);
    vtkm::cont::CastAndCall(dataSet.GetCellSet(), func);

    printSummary_ArrayHandle(result, std::cout);
    */
}

void CreateData(int d)
{

  vtkm::Id3 dims(d, d, d);
  vtkm::cont::DataSet dataSet = CreateTestDataSet(dims);

  char tmp[64];
  sprintf(tmp, "regular_%d.vtk", d);
  std::string fname = tmp;

  std::cout << fname << std::endl;
  vtkm::io::writer::VTKDataSetWriter wrt(fname);
  wrt.WriteDataSet(dataSet);
}

void MarchingCubesTest(const vtkm::cont::DataSet& ds, int N)
{
  std::cout << "Marching Cubes test: " << N << std::endl;
  vtkm::Range range;
  ds.GetField(0).GetRange(&range, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());

  int nv = 10;
  vtkm::Float32 v0 = range.Min, v1 = range.Max;
  vtkm::Float32 dv = (v1 - v0) / (vtkm::Float32)(nv - 1);
  std::cout << "Field range: (" << v0 << "," << v1 << ") dv= " << dv << std::endl;

  vtkm::filter::MarchingCubes mc;
  mc.SetGenerateNormals(true);
  for (int i = 0; i < N; i++)
  {
    if (printProgress && i % 10 == 0)
      std::cout << "   " << i << " of " << N << std::endl;

    for (int j = 0; j < nv; j++)
    {
      vtkm::filter::ResultDataSet result;
      //vtkm::Float32 val = v0 + i*dv;
      //std::cout<<i<<": "<<val<<std::endl;
      mc.SetIsoValue(v0 + j * dv);
      //mc.SetMergeDuplicatePoints(false);
      result = mc.Execute(ds, ds.GetField(0));
      const vtkm::cont::DataSet& out = result.GetDataSet();
    }
  }



#if 0

  vtkm::Float32 v0 = range.Min, v1 = range.Max;
  vtkm::Float32 dv = (v1-v0)/(vtkm::Float32)(N-1);
  std::cout<<"Field range: ("<<v0<<","<<v1<<") dv= "<<dv<<std::endl;


#if 0
  vtkm::filter::MarchingCubes mc;
  mc.SetGenerateNormals(true);

  for (int i = 0; i < N; i++)
  {
      if (printProgress && i % 10 == 0)
          std::cout<<"   "<<i<<" of "<<N<<std::endl;
      vtkm::filter::ResultDataSet result;
      //vtkm::Float32 val = v0 + i*dv;
      //std::cout<<i<<": "<<val<<std::endl;
      mc.SetIsoValue(v0 + i*dv);
      //mc.SetMergeDuplicatePoints(false);
      result = mc.Execute(ds, ds.GetField(0));
      const vtkm::cont::DataSet &out = result.GetDataSet();
  }
#endif

  for (int i = 0; i < N; i++)
  {
      if (printProgress && i % 10 == 0)
          std::cout<<"   "<<i<<" of "<<N<<std::endl;
      vtkm::filter::ResultDataSet result;
      vtkm::filter::MarchingCubes mc;
      mc.SetGenerateNormals(true);
      //vtkm::Float32 val = v0 + i*dv;
      //std::cout<<i<<": "<<val<<std::endl;
      mc.SetIsoValue(v0 + i*dv);
      //mc.SetMergeDuplicatePoints(false);
      result = mc.Execute(ds, ds.GetField(0));
      const vtkm::cont::DataSet &out = result.GetDataSet();
  }
#endif

  /*
  vtkm::filter::ResultDataSet result;
  vtkm::filter::MarchingCubes mc;
  mc.SetGenerateNormals(true);
  //vtkm::Float32 val = v0 + i*dv;
  //std::cout<<i<<": "<<val<<std::endl;
  mc.SetIsoValue(0.5f);
  result = mc.Execute(ds, ds.GetField(0));
  const vtkm::cont::DataSet &out = result.GetDataSet();
  vtkm::io::writer::VTKDataSetWriter wrt("iso.vtk");
  wrt.WriteDataSet(out);
*/
}

static vtkm::cont::DataSet createUniform(const vtkm::cont::DataSet& ds)
{
  vtkm::cont::DataSetBuilderUniform builder;
  vtkm::cont::DataSetFieldAdd fieldAdd;

  vtkm::cont::DataSet out = builder.Create(vtkm::Id3(50, 50, 50));
  fieldAdd.AddPointField(out, ds.GetField(0).GetName(), ds.GetField(0).GetData());
  out.PrintSummary(std::cout);
  return out;
}

void StreamlineTest(vtkm::cont::DataSet& ds, int N)
{
  const vtkm::Id nSeeds = 25000;
  const vtkm::Id nSteps = 20000;
  const vtkm::Float32 tStep = 0.05f;
  const vtkm::Id direction = vtkm::worklet::internal::FORWARD; //vtkm::worklet::internal::BOTH;

  vtkm::worklet::StreamLineFilterUniformGrid<vtkm::Float32>* streamLineFilter;
  streamLineFilter = new vtkm::worklet::StreamLineFilterUniformGrid<vtkm::Float32>();

  std::cout << "Streamline test: " << N << std::endl;
  for (int i = 0; i < N; i++)
  {
    if (printProgress && i % 10 == 0)
      std::cout << "   " << i << " of " << N << std::endl;
    vtkm::cont::DataSet out;
    out = streamLineFilter->Run(ds, direction, nSeeds, nSteps, tStep);
  }
}

void RenderRTTest(const vtkm::cont::DataSet& ds, int N)
{
  std::cout << "Ray Tracing test: " << N << std::endl;
  for (int i = 0; i < N; i++)
  {
    if (printProgress && i % 10 == 0)
      std::cout << "   " << i << " of " << N << std::endl;

    using M = vtkm::rendering::MapperRayTracer;
    using C = vtkm::rendering::CanvasRayTracer;
    using V3 = vtkm::rendering::View3D;

    //std::cout<<"Render: "<<i<<std::endl;
    vtkm::cont::ColorTable colorTable("inferno");
    vtkm::rendering::testing::Render<M, C, V3>(ds, "scalar", colorTable);
  }
}

void RenderVolTest(const vtkm::cont::DataSet& ds, int N)
{
  std::cout << "Volume Rendering test :" << N << std::endl;
  for (int i = 0; i < N; i++)
  {
    if (printProgress && i % 10 == 0)
      std::cout << "   " << i << " of " << N << std::endl;

    using M = vtkm::rendering::MapperVolume;
    using C = vtkm::rendering::CanvasRayTracer;
    using V3 = vtkm::rendering::View3D;

    //std::cout<<"Render: "<<i<<std::endl;
    vtkm::cont::ColorTable colorTable("inferno");
    vtkm::rendering::testing::Render<M, C, V3>(ds, "scalar", colorTable);
  }
}

void ExternalFacesTest(const vtkm::cont::DataSet& ds, int N)
{
  std::cout << "External Face test: " << N << std::endl;
  for (int i = 0; i < N; i++)
  {
    if (printProgress && i % 10 == 0)
      std::cout << "   " << i << " of " << N << std::endl;

    vtkm::cont::CellSetExplicit<> inCellSet;
    //vtkm::cont::CellSetSingleType<> inCellSet;
    ds.GetCellSet(0).CopyTo(inCellSet);
    vtkm::cont::CellSetExplicit<> outCellSet("cells");
    //vtkm::cont::CellSetSingleType<> outCellSet("cells");
    //Run the External Faces worklet
    vtkm::worklet::ExternalFaces().Run(inCellSet, outCellSet, VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    vtkm::cont::DataSet outDataSet;
    for (vtkm::IdComponent i = 0; i < ds.GetNumberOfCoordinateSystems(); ++i)
      outDataSet.AddCoordinateSystem(ds.GetCoordinateSystem(i));
    outDataSet.AddCellSet(outCellSet);
  }
}

/*
Notes:
render: 256 reg and rect. 100 iterations.

expl: external faces requires non-SingleType explicit.  Forced reader to use this.

streamlines: vector field must be named "vecData"

 */
int main(int argc, char** argv)
{
  // Process vtk-m general args
  auto opts = vtkm::cont::InitializeOptions::DefaultAnyDevice;
  auto config = vtkm::cont::Initialize(argc, argv, opts);

  if (argc != 5)
  {
    std::cout << "Error: " << argv[0]
              << " [options] <algo:iso/sl/ext/rt/vol> N SZ <ftype:reg/rect/expl>" << std::endl;
    std::cout << "General Options: \n" << config.Usage << std::endl;
    return -1;
  }


  std::string alg = argv[1];
  int N = atoi(argv[2]);

  if (alg == "sl")
  {
    char fname[64];
    sprintf(fname, "../data/tornado.vec");
    std::cout << "Reading file: " << fname << std::endl;

    FILE* pFile = fopen(fname, "rb");
    int dims[3];
    size_t ret_code = fread(dims, sizeof(int), 3, pFile);
    const vtkm::Id3 vdims(dims[0], dims[1], dims[2]);
    vtkm::Id nElements = vdims[0] * vdims[1] * vdims[2] * 3;
    float* data = new float[static_cast<std::size_t>(nElements)];
    ret_code = fread(data, sizeof(float), static_cast<std::size_t>(nElements), pFile);
    fclose(pFile);

    std::vector<vtkm::Vec<vtkm::Float32, 3>> field;
    for (vtkm::Id i = 0; i < nElements; i++)
    {
      vtkm::Float32 x = data[i];
      vtkm::Float32 y = data[++i];
      vtkm::Float32 z = data[++i];
      vtkm::Vec<vtkm::Float32, 3> vecData(x, y, z);
      field.push_back(Normalize(vecData));
    }

    vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>> fieldArray;
    fieldArray = vtkm::cont::make_ArrayHandle(field);

    // Construct the input dataset (uniform) to hold the input and set vector data
    vtkm::cont::DataSet ds;
    vtkm::cont::ArrayHandleUniformPointCoordinates coordinates(vdims);
    ds.AddCoordinateSystem(vtkm::cont::CoordinateSystem("coordinates", coordinates));
    ds.AddField(vtkm::cont::Field("vec", vtkm::cont::Field::Association::POINTS, fieldArray));

    vtkm::cont::CellSetStructured<3> inCellSet("cells");
    inCellSet.SetPointDimensions(vtkm::make_Vec(vdims[0], vdims[1], vdims[2]));
    ds.AddCellSet(inCellSet);

    StreamlineTest(ds, N);
    return 0;
  }
  else
  {
    int sz = atoi(argv[3]);
    std::string ftype = argv[4];

    char fname[64];
    sprintf(fname, "../data/s%s_%d.vtk", ftype.c_str(), sz);
    std::cout << "FNAME= " << fname << std::endl;

    vtkm::cont::DataSet ds;
    if (sz < 0)
    {
      vtkm::cont::testing::MakeTestDataSet dataSetMaker;
      ds = dataSetMaker.Make3DExplicitDataSet5();
    }
    else
    {
      std::cout << "Reading file: " << fname << std::endl;
      vtkm::io::reader::VTKDataSetReader rdr(fname);
      ds = rdr.ReadDataSet();
    }

    if (alg == "iso")
      MarchingCubesTest(ds, N);
    else if (alg == "ext")
      ExternalFacesTest(ds, N);
    else if (alg == "rt")
      RenderRTTest(ds, N);
    else if (alg == "vol")
      RenderVolTest(ds, N);
    return 0;
  }
}
