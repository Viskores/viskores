//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <typeinfo>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderExplicit.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>

#include <vtkm/io/writer/VTKDataSetWriter.h>

namespace
{

vtkm::FloatDefault vecData[125 * 3] = {
  -0.00603248f, -0.0966396f,  -0.000732792f, 0.000530014f,  -0.0986189f,  -0.000806706f,
  0.00684929f,  -0.100098f,   -0.000876566f, 0.0129235f,    -0.101102f,   -0.000942341f,
  0.0187515f,   -0.101656f,   -0.00100401f,  0.0706091f,    -0.083023f,   -0.00144278f,
  0.0736404f,   -0.0801616f,  -0.00145784f,  0.0765194f,    -0.0772063f,  -0.00147036f,
  0.0792559f,   -0.0741751f,  -0.00148051f,  0.0818589f,    -0.071084f,   -0.00148843f,
  0.103585f,    -0.0342287f,  -0.001425f,    0.104472f,     -0.0316147f,  -0.00140433f,
  0.105175f,    -0.0291574f,  -0.00138057f,  0.105682f,     -0.0268808f,  -0.00135357f,
  0.105985f,    -0.0248099f,  -0.00132315f,  -0.00244603f,  -0.0989576f,  -0.000821705f,
  0.00389525f,  -0.100695f,   -0.000894513f, 0.00999301f,   -0.10193f,    -0.000963114f,
  0.0158452f,   -0.102688f,   -0.00102747f,  0.0214509f,    -0.102995f,   -0.00108757f,
  0.0708166f,   -0.081799f,   -0.00149941f,  0.0736939f,    -0.0787879f,  -0.00151236f,
  0.0764359f,   -0.0756944f,  -0.00152297f,  0.0790546f,    -0.0725352f,  -0.00153146f,
  0.0815609f,   -0.0693255f,  -0.001538f,    -0.00914287f,  -0.104658f,   -0.001574f,
  -0.00642891f, -0.10239f,    -0.00159659f,  -0.00402289f,  -0.0994835f,  -0.00160731f,
  -0.00194792f, -0.0959752f,  -0.00160528f,  -0.00022818f,  -0.0919077f,  -0.00158957f,
  -0.0134913f,  -0.0274735f,  -9.50056e-05f, -0.0188683f,   -0.023273f,   0.000194107f,
  -0.0254516f,  -0.0197589f,  0.000529693f,  -0.0312798f,   -0.0179514f,  0.00083619f,
  -0.0360426f,  -0.0177537f,  0.00110164f,   0.0259929f,    -0.0204479f,  -0.000304646f,
  0.033336f,    -0.0157385f,  -0.000505569f, 0.0403427f,    -0.0104637f,  -0.000693529f,
  0.0469371f,   -0.00477766f, -0.000865609f, 0.0530722f,    0.0011701f,   -0.00102f,
  -0.0121869f,  -0.10317f,    -0.0015868f,   -0.0096549f,   -0.100606f,   -0.00160377f,
  -0.00743038f, -0.0973796f,  -0.00160783f,  -0.00553901f,  -0.0935261f,  -0.00159792f,
  -0.00400821f, -0.0890871f,  -0.00157287f,  -0.0267803f,   -0.0165823f,  0.000454173f,
  -0.0348303f,  -0.011642f,   0.000881271f,  -0.0424964f,   -0.00870761f, 0.00129226f,
  -0.049437f,   -0.00781358f, 0.0016728f,    -0.0552635f,   -0.00888708f, 0.00200659f,
  -0.0629746f,  -0.0721524f,  -0.00160475f,  -0.0606813f,   -0.0677576f,  -0.00158427f,
  -0.0582203f,  -0.0625009f,  -0.00154304f,  -0.0555686f,   -0.0563905f,  -0.00147822f,
  -0.0526988f,  -0.0494369f,  -0.00138643f,  0.0385695f,    0.115704f,    0.00674413f,
  0.056434f,    0.128273f,    0.00869052f,   0.0775564f,    0.137275f,    0.0110399f,
  0.102515f,    0.140823f,    0.0138637f,    0.131458f,     0.136024f,    0.0171804f,
  0.0595175f,   -0.0845927f,  0.00512454f,   0.0506615f,    -0.0680369f,  0.00376604f,
  0.0434904f,   -0.0503557f,  0.00261592f,   0.0376711f,    -0.0318716f,  0.00163301f,
  0.0329454f,   -0.0128019f,  0.000785352f,  -0.0664062f,   -0.0701094f,  -0.00160644f,
  -0.0641074f,  -0.0658893f,  -0.00158969f,  -0.0616054f,   -0.0608302f,  -0.00155303f,
  -0.0588734f,  -0.0549447f,  -0.00149385f,  -0.0558797f,   -0.0482482f,  -0.00140906f,
  0.0434062f,   0.102969f,    0.00581269f,   0.0619547f,    0.112838f,    0.00742057f,
  0.0830229f,   0.118752f,    0.00927516f,   0.106603f,     0.119129f,    0.0113757f,
  0.132073f,    0.111946f,    0.0136613f,    -0.0135758f,   -0.0934604f,  -0.000533868f,
  -0.00690763f, -0.0958773f,  -0.000598878f, -0.000475275f, -0.0977838f,  -0.000660985f,
  0.00571866f,  -0.0992032f,  -0.0007201f,   0.0116724f,    -0.10016f,    -0.000776144f,
  0.0651428f,   -0.0850475f,  -0.00120243f,  0.0682895f,    -0.0823666f,  -0.00121889f,
  0.0712792f,   -0.0795772f,  -0.00123291f,  0.0741224f,    -0.0766981f,  -0.00124462f,
  0.076829f,    -0.0737465f,  -0.00125416f,  0.10019f,      -0.0375515f,  -0.00121866f,
  0.101296f,    -0.0348723f,  -0.00120216f,  0.102235f,     -0.0323223f,  -0.00118309f,
  0.102994f,    -0.0299234f,  -0.00116131f,  0.103563f,     -0.0276989f,  -0.0011367f,
  -0.00989236f, -0.0958821f,  -0.000608883f, -0.00344154f,  -0.0980645f,  -0.000673641f,
  0.00277318f,  -0.0997337f,  -0.000735354f, 0.00874908f,   -0.100914f,   -0.000793927f,
  0.0144843f,   -0.101629f,   -0.000849279f, 0.0654428f,    -0.0839355f,  -0.00125739f,
  0.0684225f,   -0.0810989f,  -0.00127208f,  0.0712599f,    -0.0781657f,  -0.00128444f,
  0.0739678f,   -0.0751541f,  -0.00129465f,  0.076558f,     -0.0720804f,  -0.00130286f,
  -0.0132841f,  -0.103948f,   -0.00131159f,  -0.010344f,    -0.102328f,   -0.0013452f,
  -0.00768637f, -0.100054f,   -0.00136938f,  -0.00533293f,  -0.0971572f,  -0.00138324f,
  -0.00330643f, -0.0936735f,  -0.00138586f,  -0.0116984f,   -0.0303752f,  -0.000229102f,
  -0.0149879f,  -0.0265231f,  -3.43823e-05f, -0.0212917f,   -0.0219544f,  0.000270283f,
  -0.0277756f,  -0.0186879f,  0.000582781f,  -0.0335115f,   -0.0171098f,  0.00086919f,
  0.0170095f,   -0.025299f,   -3.73557e-05f, 0.024552f,     -0.0214351f,  -0.000231975f,
  0.0318714f,   -0.0168568f,  -0.000417463f, 0.0388586f,    -0.0117131f,  -0.000589883f,
  0.0454388f,   -0.00615626f, -0.000746594f, -0.0160785f,   -0.102675f,   -0.00132891f,
  -0.0133174f,  -0.100785f,   -0.00135859f,  -0.0108365f,   -0.0982184f,  -0.00137801f,
  -0.00865931f, -0.0950053f,  -0.00138614f,  -0.00681126f,  -0.0911806f,  -0.00138185f,
  -0.0208973f,  -0.0216631f,  0.000111231f,  -0.0289373f,   -0.0151081f,  0.000512553f,
  -0.0368736f,  -0.0104306f,  0.000911793f,  -0.0444294f,   -0.00773838f, 0.00129762f,
  -0.0512663f,  -0.00706554f, 0.00165611f
};
}

template <typename ScalarType>
vtkm::Vec<ScalarType, 3> RandomPoint(const vtkm::Bounds& bounds)
{
  ScalarType rx = static_cast<ScalarType>(rand()) / static_cast<ScalarType>(RAND_MAX);
  ScalarType ry = static_cast<ScalarType>(rand()) / static_cast<ScalarType>(RAND_MAX);
  ScalarType rz = static_cast<ScalarType>(rand()) / static_cast<ScalarType>(RAND_MAX);

  vtkm::Vec<ScalarType, 3> p;
  p[0] = static_cast<ScalarType>(bounds.X.Min + rx * bounds.X.Length());
  p[1] = static_cast<ScalarType>(bounds.Y.Min + ry * bounds.Y.Length());
  p[2] = static_cast<ScalarType>(bounds.Z.Min + rz * bounds.Z.Length());
  return p;
}

template <typename ScalarType>
vtkm::cont::DataSet CreateUniformDataSet(const vtkm::Bounds& bounds, const vtkm::Id3& dims)
{
  vtkm::Vec<ScalarType, 3> origin(static_cast<ScalarType>(bounds.X.Min),
                                  static_cast<ScalarType>(bounds.Y.Min),
                                  static_cast<ScalarType>(bounds.Z.Min));
  vtkm::Vec<ScalarType, 3> spacing(
    static_cast<ScalarType>(bounds.X.Length()) / static_cast<ScalarType>((dims[0] - 1)),
    static_cast<ScalarType>(bounds.Y.Length()) / static_cast<ScalarType>((dims[1] - 1)),
    static_cast<ScalarType>(bounds.Z.Length()) / static_cast<ScalarType>((dims[2] - 1)));

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);
  return ds;
}

template <typename ScalarType>
vtkm::cont::DataSet CreateRectilinearDataSet(const vtkm::Bounds& bounds, const vtkm::Id3& dims)
{
  vtkm::cont::DataSetBuilderRectilinear dataSetBuilder;
  std::vector<ScalarType> xvals, yvals, zvals;

  vtkm::Vec<ScalarType, 3> spacing(
    static_cast<ScalarType>(bounds.X.Length()) / static_cast<ScalarType>((dims[0] - 1)),
    static_cast<ScalarType>(bounds.Y.Length()) / static_cast<ScalarType>((dims[1] - 1)),
    static_cast<ScalarType>(bounds.Z.Length()) / static_cast<ScalarType>((dims[2] - 1)));
  xvals.resize((size_t)dims[0]);
  xvals[0] = static_cast<ScalarType>(bounds.X.Min);
  for (size_t i = 1; i < (size_t)dims[0]; i++)
    xvals[i] = xvals[i - 1] + spacing[0];

  yvals.resize((size_t)dims[1]);
  yvals[0] = static_cast<ScalarType>(bounds.Y.Min);
  for (size_t i = 1; i < (size_t)dims[1]; i++)
    yvals[i] = yvals[i - 1] + spacing[1];

  zvals.resize((size_t)dims[2]);
  zvals[0] = static_cast<ScalarType>(bounds.Z.Min);
  for (size_t i = 1; i < (size_t)dims[2]; i++)
    zvals[i] = zvals[i - 1] + spacing[2];

  vtkm::cont::DataSet ds = dataSetBuilder.Create(xvals, yvals, zvals);
  return ds;
}

template <class CellSetType, vtkm::IdComponent NDIM>
static void MakeExplicitCells(const CellSetType& cellSet,
                              vtkm::Vec<vtkm::Id, NDIM>& cellDims,
                              vtkm::cont::ArrayHandle<vtkm::IdComponent>& numIndices,
                              vtkm::cont::ArrayHandle<vtkm::UInt8>& shapes,
                              vtkm::cont::ArrayHandle<vtkm::Id>& conn)
{
  using Connectivity = vtkm::internal::ConnectivityStructuredInternals<NDIM>;

  vtkm::Id nCells = cellSet.GetNumberOfCells();
  vtkm::IdComponent nVerts = (NDIM == 2 ? 4 : 8);
  vtkm::Id connLen = (NDIM == 2 ? nCells * 4 : nCells * 8);

  conn.Allocate(connLen);
  shapes.Allocate(nCells);
  numIndices.Allocate(nCells);

  Connectivity structured;
  structured.SetPointDimensions(cellDims + vtkm::Vec<vtkm::Id, NDIM>(1));

  vtkm::Id idx = 0;
  for (vtkm::Id i = 0; i < nCells; i++)
  {
    auto ptIds = structured.GetPointsOfCell(i);
    for (vtkm::IdComponent j = 0; j < nVerts; j++, idx++)
      conn.GetPortalControl().Set(idx, ptIds[j]);

    shapes.GetPortalControl().Set(
      i, (NDIM == 2 ? vtkm::CELL_SHAPE_QUAD : vtkm::CELL_SHAPE_HEXAHEDRON));
    numIndices.GetPortalControl().Set(i, nVerts);
  }
}

template <typename ScalarType>
vtkm::cont::DataSet CreateExplicitFromStructuredDataSet(const vtkm::cont::DataSet& input,
                                                        bool createSingleType = false)
{
  using CoordType = vtkm::Vec<ScalarType, 3>;

  auto inputCoords = input.GetCoordinateSystem(0).GetData();
  vtkm::Id numPts = inputCoords.GetNumberOfValues();
  vtkm::cont::ArrayHandle<CoordType> explCoords;

  explCoords.Allocate(numPts);
  auto explPortal = explCoords.GetPortalControl();
  auto cp = inputCoords.GetPortalConstControl();
  for (vtkm::Id i = 0; i < numPts; i++)
    explPortal.Set(i, cp.Get(i));

  vtkm::cont::DynamicCellSet cellSet = input.GetCellSet(0);
  vtkm::cont::ArrayHandle<vtkm::Id> conn;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
  vtkm::cont::ArrayHandle<vtkm::UInt8> shapes;
  vtkm::cont::DataSet output;
  vtkm::cont::DataSetBuilderExplicit dsb;

  if (cellSet.IsType<vtkm::cont::CellSetStructured<2>>())
  {
    vtkm::cont::CellSetStructured<2> cells2D = cellSet.Cast<vtkm::cont::CellSetStructured<2>>();
    vtkm::Id2 cellDims = cells2D.GetCellDimensions();
    MakeExplicitCells(cells2D, cellDims, numIndices, shapes, conn);
    if (createSingleType)
      output = dsb.Create(explCoords, vtkm::CellShapeTagQuad(), 4, conn, "coordinates", "cells");
    else
      output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates", "cells");
  }
  else if (cellSet.IsType<vtkm::cont::CellSetStructured<3>>())
  {
    vtkm::cont::CellSetStructured<3> cells3D = cellSet.Cast<vtkm::cont::CellSetStructured<3>>();
    vtkm::Id3 cellDims = cells3D.GetCellDimensions();
    MakeExplicitCells(cells3D, cellDims, numIndices, shapes, conn);
    if (createSingleType)
      output =
        dsb.Create(explCoords, vtkm::CellShapeTagHexahedron(), 8, conn, "coordinates", "cells");
    else
      output = dsb.Create(explCoords, shapes, numIndices, conn, "coordinates", "cells");
  }

  return output;
}

template <typename ScalarType>
void CreateConstantVectorField(vtkm::Id num,
                               const vtkm::Vec<ScalarType, 3>& vec,
                               vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>& vecField)
{
  vtkm::cont::ArrayHandleConstant<vtkm::Vec<ScalarType, 3>> vecConst;
  vecConst = vtkm::cont::make_ArrayHandleConstant(vec, num);
  vtkm::cont::ArrayCopy(vecConst, vecField);
}

template <typename ScalarType>
class TestEvaluatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputPoint,
                                ExecObject evaluator,
                                FieldOut validity,
                                FieldOut outputPoint);

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename EvaluatorType>
  VTKM_EXEC void operator()(vtkm::Vec<ScalarType, 3>& pointIn,
                            const EvaluatorType& evaluator,
                            bool& validity,
                            vtkm::Vec<ScalarType, 3>& pointOut) const
  {
    validity = evaluator.Evaluate(pointIn, pointOut);
  }
};

template <typename EvalType, typename ScalarType>
void ValidateEvaluator(const EvalType& eval,
                       const std::vector<vtkm::Vec<ScalarType, 3>>& pointIns,
                       const vtkm::Vec<ScalarType, 3>& vec,
                       const std::string& msg)
{
  using EvalTester = TestEvaluatorWorklet<ScalarType>;
  using EvalTesterDispatcher = vtkm::worklet::DispatcherMapField<EvalTester>;
  EvalTester evalTester;
  EvalTesterDispatcher evalTesterDispatcher(evalTester);
  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> pointsHandle =
    vtkm::cont::make_ArrayHandle(pointIns);
  vtkm::Id numPoints = pointsHandle.GetNumberOfValues();
  vtkm::cont::ArrayHandle<bool> evalStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> evalResults;
  evalTesterDispatcher.Invoke(pointsHandle, eval, evalStatus, evalResults);
  auto statusPortal = evalStatus.GetPortalConstControl();
  auto resultsPortal = evalResults.GetPortalConstControl();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    bool status = statusPortal.Get(index);
    vtkm::Vec<ScalarType, 3> result = resultsPortal.Get(index);
    VTKM_TEST_ASSERT(status, "Error in evaluator for " + msg);
    VTKM_TEST_ASSERT(result == vec, "Error in evaluator result for " + msg);
  }
  pointsHandle.ReleaseResources();
  evalStatus.ReleaseResources();
  evalResults.ReleaseResources();
}

template <typename ScalarType>
class TestIntegratorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputPoint,
                                ExecObject integrator,
                                FieldOut validity,
                                FieldOut outputPoint);

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename IntegratorType>
  VTKM_EXEC void operator()(vtkm::Vec<ScalarType, 3>& pointIn,
                            const IntegratorType* integrator,
                            vtkm::worklet::particleadvection::ParticleStatus& status,
                            vtkm::Vec<ScalarType, 3>& pointOut) const
  {
    ScalarType time = 0;
    status = integrator->Step(pointIn, time, pointOut);
  }
};


template <typename IntegratorType, typename ScalarType>
void ValidateIntegrator(const IntegratorType& integrator,
                        const std::vector<vtkm::Vec<ScalarType, 3>>& pointIns,
                        const std::vector<vtkm::Vec<ScalarType, 3>>& expStepResults,
                        const std::string& msg)
{
  using IntegratorTester = TestIntegratorWorklet<ScalarType>;
  using IntegratorTesterDispatcher = vtkm::worklet::DispatcherMapField<IntegratorTester>;
  using Status = vtkm::worklet::particleadvection::ParticleStatus;
  IntegratorTesterDispatcher integratorTesterDispatcher;
  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> pointsHandle =
    vtkm::cont::make_ArrayHandle(pointIns);
  vtkm::Id numPoints = pointsHandle.GetNumberOfValues();
  vtkm::cont::ArrayHandle<Status> stepStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> stepResults;
  integratorTesterDispatcher.Invoke(pointsHandle, integrator, stepStatus, stepResults);
  auto statusPortal = stepStatus.GetPortalConstControl();
  auto resultsPortal = stepResults.GetPortalConstControl();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);
    vtkm::Vec<ScalarType, 3> result = resultsPortal.Get(index);
    VTKM_TEST_ASSERT(status == Status::STATUS_OK || status == Status::TERMINATED ||
                       status == Status::EXITED_SPATIAL_BOUNDARY,
                     "Error in evaluator for " + msg);
    VTKM_TEST_ASSERT(result == expStepResults[(size_t)index],
                     "Error in evaluator result for " + msg);
  }
  pointsHandle.ReleaseResources();
  stepStatus.ReleaseResources();
  stepResults.ReleaseResources();
}

void TestEvaluators()
{
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;

  std::vector<vtkm::Vec<ScalarType, 3>> vecs;
  vecs.push_back(vtkm::Vec<ScalarType, 3>(1, 0, 0));
  vecs.push_back(vtkm::Vec<ScalarType, 3>(0, 1, 0));
  vecs.push_back(vtkm::Vec<ScalarType, 3>(0, 0, 1));
  vecs.push_back(vtkm::Vec<ScalarType, 3>(1, 1, 0));
  vecs.push_back(vtkm::Vec<ScalarType, 3>(0, 1, 1));
  vecs.push_back(vtkm::Vec<ScalarType, 3>(1, 0, 1));
  vecs.push_back(vtkm::Vec<ScalarType, 3>(1, 1, 1));

  std::vector<vtkm::Bounds> bounds;
  bounds.push_back(vtkm::Bounds(0, 10, 0, 10, 0, 10));
  bounds.push_back(vtkm::Bounds(-1, 1, -1, 1, -1, 1));
  bounds.push_back(vtkm::Bounds(0, 1, 0, 1, -1, 1));

  std::vector<vtkm::Id3> dims;
  dims.push_back(vtkm::Id3(5, 5, 5));
  dims.push_back(vtkm::Id3(10, 5, 5));
  dims.push_back(vtkm::Id3(10, 5, 5));

  for (auto& dim : dims)
  {
    for (auto& vec : vecs)
    {
      for (auto& bound : bounds)
      {
        std::vector<vtkm::cont::DataSet> dataSets;
        dataSets.push_back(CreateUniformDataSet<ScalarType>(bound, dim));
        dataSets.push_back(CreateRectilinearDataSet<ScalarType>(bound, dim));
        //Create an explicit dataset.
        //        auto expDS = CreateExplicitFromStructuredDataSet<ScalarType>(dataSets[0], false);
        //        dataSets.push_back(expDS);

        vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> vecField;
        CreateConstantVectorField(dim[0] * dim[1] * dim[2], vec, vecField);

        ScalarType stepSize = 0.01f;
        std::vector<vtkm::Vec<ScalarType, 3>> pointIns;
        std::vector<vtkm::Vec<ScalarType, 3>> stepResult;
        //Create a bunch of random points in the bounds.
        srand(314);
        for (int k = 0; k < 38; k++)
        {
          //Generate points 2 steps inside the bounding box.
          vtkm::Bounds interiorBounds = bound;
          interiorBounds.X.Min += 2 * stepSize;
          interiorBounds.Y.Min += 2 * stepSize;
          interiorBounds.Z.Min += 2 * stepSize;
          interiorBounds.X.Max -= 2 * stepSize;
          interiorBounds.Y.Max -= 2 * stepSize;
          interiorBounds.Z.Max -= 2 * stepSize;

          auto p = RandomPoint<ScalarType>(interiorBounds);
          pointIns.push_back(p);
          stepResult.push_back(p + vec * stepSize);
        }

        for (auto& ds : dataSets)
        {
          //          ds.PrintSummary(std::cout);
          //          vtkm::io::writer::VTKDataSetWriter writer1("ds.vtk");
          //          writer1.WriteDataSet(ds);

          GridEvalType gridEval(ds.GetCoordinateSystem(), ds.GetCellSet(), vecField);
          ValidateEvaluator(gridEval, pointIns, vec, "grid evaluator");

          RK4Type rk4(gridEval, stepSize);
          ValidateIntegrator(rk4, pointIns, stepResult, "constant vector RK4");
        }
      }
    }
  }
}

template <typename ScalarType>
void ValidateParticleAdvectionResult(const vtkm::worklet::ParticleAdvectionResult& res,
                                     vtkm::Id nSeeds,
                                     vtkm::Id maxSteps)
{
  VTKM_TEST_ASSERT(res.positions.GetNumberOfValues() == nSeeds,
                   "Number of output particles does not match input.");
  for (vtkm::Id i = 0; i < nSeeds; i++)
    VTKM_TEST_ASSERT(res.stepsTaken.GetPortalConstControl().Get(i) <= maxSteps,
                     "Too many steps taken in particle advection");
}

template <typename ScalarType>
void ValidateStreamlineResult(const vtkm::worklet::StreamlineResult& res,
                              vtkm::Id nSeeds,
                              vtkm::Id maxSteps)
{
  VTKM_TEST_ASSERT(res.polyLines.GetNumberOfCells() == nSeeds,
                   "Number of output streamlines does not match input.");

  for (vtkm::Id i = 0; i < nSeeds; i++)
    VTKM_TEST_ASSERT(res.stepsTaken.GetPortalConstControl().Get(i) <= maxSteps,
                     "Too many steps taken in streamline");
}

void TestParticleWorklets()
{
  using ScalarType = vtkm::worklet::particleadvection::ScalarType;
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  ScalarType stepSize = 0.01f;

  const vtkm::Id3 dims(5, 5, 5);
  vtkm::Id nElements = dims[0] * dims[1] * dims[2] * 3;

  std::vector<vtkm::Vec<vtkm::FloatDefault, 3>> field;
  for (vtkm::Id i = 0; i < nElements; i++)
  {
    ScalarType x = vecData[i];
    ScalarType y = vecData[++i];
    ScalarType z = vecData[++i];
    vtkm::Vec<ScalarType, 3> vec(x, y, z);
    field.push_back(vtkm::Normal(vec));
  }
  vtkm::cont::ArrayHandle<vtkm::Vec<ScalarType, 3>> fieldArray;
  fieldArray = vtkm::cont::make_ArrayHandle(field);

  std::vector<vtkm::Bounds> bounds;
  bounds.push_back(vtkm::Bounds(0, 10, 0, 10, 0, 10));
  bounds.push_back(vtkm::Bounds(-1, 1, -1, 1, -1, 1));
  bounds.push_back(vtkm::Bounds(0, 1, 0, 1, -1, 1));

  vtkm::Id maxSteps = 1000;
  for (auto& bound : bounds)
  {
    std::vector<vtkm::cont::DataSet> dataSets;
    dataSets.push_back(CreateUniformDataSet<ScalarType>(bound, dims));
    dataSets.push_back(CreateRectilinearDataSet<ScalarType>(bound, dims));
    //Create an explicit dataset.
    //    auto expDS = CreateExplicitFromStructuredDataSet<ScalarType>(dataSets[0], false);
    //    dataSets.push_back(expDS);

    //Generate three random points.
    std::vector<vtkm::Vec<ScalarType, 3>> pts;
    pts.push_back(RandomPoint<ScalarType>(bound));
    pts.push_back(RandomPoint<ScalarType>(bound));
    pts.push_back(RandomPoint<ScalarType>(bound));

    vtkm::Id nSeeds = static_cast<vtkm::Id>(pts.size());
    std::vector<vtkm::Id> stepsTaken = { 10, 20, 600 };

    for (auto& ds : dataSets)
    {
      GridEvalType eval(ds.GetCoordinateSystem(), ds.GetCellSet(), fieldArray);
      RK4Type rk4(eval, stepSize);

      //Do 4 tests on each dataset.
      //Particle advection worklet with and without steps taken.
      //Streamline worklet with and without steps taken.
      for (int i = 0; i < 4; i++)
      {
        auto seedsArray = vtkm::cont::make_ArrayHandle(pts, vtkm::CopyFlag::On);
        auto stepsTakenArray = vtkm::cont::make_ArrayHandle(stepsTaken, vtkm::CopyFlag::On);

        if (i < 2)
        {
          vtkm::worklet::ParticleAdvection pa;
          vtkm::worklet::ParticleAdvectionResult res;
          if (i == 0)
            res = pa.Run(rk4, seedsArray, maxSteps);
          else
            res = pa.Run(rk4, seedsArray, stepsTakenArray, maxSteps);
          ValidateParticleAdvectionResult<ScalarType>(res, nSeeds, maxSteps);
        }
        else
        {
          vtkm::worklet::Streamline s;
          vtkm::worklet::StreamlineResult res;
          if (i == 2)
            res = s.Run(rk4, seedsArray, maxSteps);
          else
            res = s.Run(rk4, seedsArray, stepsTakenArray, maxSteps);
          ValidateStreamlineResult<ScalarType>(res, nSeeds, maxSteps);
        }
      }
    }
  }
}

void TestParticleAdvection()
{
  TestEvaluators();
  TestParticleWorklets();
}

int UnitTestParticleAdvection(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleAdvection, argc, argv);
}
