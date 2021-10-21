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
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/filter/GhostCellClassify.h>
#include <vtkm/io/VTKDataSetReader.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/EulerIntegrator.h>
#include <vtkm/worklet/particleadvection/Field.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Particles.h>
#include <vtkm/worklet/particleadvection/RK4Integrator.h>
#include <vtkm/worklet/particleadvection/Stepper.h>
#include <vtkm/worklet/testing/GenerateTestDataSets.h>

#include <random>

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

void GenerateRandomParticles(std::vector<vtkm::Particle>& points,
                             const std::size_t N,
                             const vtkm::Bounds& bounds,
                             const std::size_t seed = 314)
{
  std::random_device device;
  std::default_random_engine generator(static_cast<vtkm::UInt32>(seed));
  vtkm::FloatDefault zero(0), one(1);
  std::uniform_real_distribution<vtkm::FloatDefault> distribution(zero, one);

  points.resize(0);
  for (std::size_t i = 0; i < N; i++)
  {
    vtkm::FloatDefault rx = distribution(generator);
    vtkm::FloatDefault ry = distribution(generator);
    vtkm::FloatDefault rz = distribution(generator);

    vtkm::Vec3f p;
    p[0] = static_cast<vtkm::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
    p[1] = static_cast<vtkm::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
    p[2] = static_cast<vtkm::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
    points.push_back(vtkm::Particle(p, static_cast<vtkm::Id>(i)));
  }
}

void CreateConstantVectorField(vtkm::Id num,
                               const vtkm::Vec3f& vec,
                               vtkm::cont::ArrayHandle<vtkm::Vec3f>& vecField)
{
  vtkm::cont::ArrayHandleConstant<vtkm::Vec3f> vecConst;
  vecConst = vtkm::cont::make_ArrayHandleConstant(vec, num);
  vtkm::cont::ArrayCopy(vecConst, vecField);
}

class TestEvaluatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputPoint,
                                ExecObject evaluator,
                                FieldOut validity,
                                FieldOut outputPoint);

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename EvaluatorType>
  VTKM_EXEC void operator()(vtkm::Particle& pointIn,
                            const EvaluatorType& evaluator,
                            vtkm::worklet::particleadvection::GridEvaluatorStatus& status,
                            vtkm::Vec3f& pointOut) const
  {
    vtkm::VecVariable<vtkm::Vec3f, 2> values;
    status = evaluator.Evaluate(pointIn.Pos, pointIn.Time, values);
    pointOut = values[0];
  }
};

template <typename EvalType>
void ValidateEvaluator(const EvalType& eval,
                       const std::vector<vtkm::Particle>& pointIns,
                       const vtkm::Vec3f& vec,
                       const std::string& msg)
{
  using EvalTester = TestEvaluatorWorklet;
  using EvalTesterDispatcher = vtkm::worklet::DispatcherMapField<EvalTester>;
  using Status = vtkm::worklet::particleadvection::GridEvaluatorStatus;
  EvalTester evalTester;
  EvalTesterDispatcher evalTesterDispatcher(evalTester);
  vtkm::cont::ArrayHandle<vtkm::Particle> pointsHandle =
    vtkm::cont::make_ArrayHandle(pointIns, vtkm::CopyFlag::Off);
  vtkm::Id numPoints = pointsHandle.GetNumberOfValues();
  vtkm::cont::ArrayHandle<Status> evalStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> evalResults;
  evalTesterDispatcher.Invoke(pointsHandle, eval, evalStatus, evalResults);
  auto statusPortal = evalStatus.ReadPortal();
  auto resultsPortal = evalResults.ReadPortal();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);
    vtkm::Vec3f result = resultsPortal.Get(index);
    VTKM_TEST_ASSERT(status.CheckOk(), "Error in evaluator for " + msg);
    VTKM_TEST_ASSERT(result == vec, "Error in evaluator result for " + msg);
  }
}

class TestIntegratorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn inputPoint,
                                ExecObject integrator,
                                FieldOut validity,
                                FieldOut outputPoint);

  using ExecutionSignature = void(_1, _2, _3, _4);

  template <typename Particle, typename IntegratorType>
  VTKM_EXEC void operator()(Particle& pointIn,
                            const IntegratorType integrator,
                            vtkm::worklet::particleadvection::IntegratorStatus& status,
                            vtkm::Vec3f& pointOut) const
  {
    vtkm::FloatDefault time = 0;
    status = integrator.Step(pointIn, time, pointOut);
    if (status.CheckSpatialBounds())
      status = integrator.SmallStep(pointIn, time, pointOut);
  }
};


template <typename IntegratorType>
void ValidateIntegrator(const IntegratorType& integrator,
                        const std::vector<vtkm::Particle>& pointIns,
                        const std::vector<vtkm::Vec3f>& expStepResults,
                        const std::string& msg)
{
  using IntegratorTester = TestIntegratorWorklet;
  using IntegratorTesterDispatcher = vtkm::worklet::DispatcherMapField<IntegratorTester>;
  using Status = vtkm::worklet::particleadvection::IntegratorStatus;
  IntegratorTesterDispatcher integratorTesterDispatcher;
  auto pointsHandle = vtkm::cont::make_ArrayHandle(pointIns, vtkm::CopyFlag::Off);
  vtkm::Id numPoints = pointsHandle.GetNumberOfValues();
  vtkm::cont::ArrayHandle<Status> stepStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> stepResults;
  integratorTesterDispatcher.Invoke(pointsHandle, integrator, stepStatus, stepResults);
  auto statusPortal = stepStatus.ReadPortal();
  auto pointsPortal = pointsHandle.ReadPortal();
  auto resultsPortal = stepResults.ReadPortal();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);
    vtkm::Vec3f result = resultsPortal.Get(index);
    VTKM_TEST_ASSERT(status.CheckOk(), "Error in evaluator for " + msg);
    if (status.CheckSpatialBounds())
      VTKM_TEST_ASSERT(result == pointsPortal.Get(index).Pos,
                       "Error in evaluator result for [OUTSIDE SPATIAL]" + msg);
    else
      VTKM_TEST_ASSERT(result == expStepResults[static_cast<size_t>(index)],
                       "Error in evaluator result for " + msg);
  }
}

template <typename IntegratorType>
void ValidateIntegratorForBoundary(const vtkm::Bounds& bounds,
                                   const IntegratorType& integrator,
                                   const std::vector<vtkm::Particle>& pointIns,
                                   const std::string& msg)
{
  using IntegratorTester = TestIntegratorWorklet;
  using IntegratorTesterDispatcher = vtkm::worklet::DispatcherMapField<IntegratorTester>;
  using Status = vtkm::worklet::particleadvection::IntegratorStatus;

  IntegratorTesterDispatcher integratorTesterDispatcher;
  auto pointsHandle = vtkm::cont::make_ArrayHandle(pointIns, vtkm::CopyFlag::Off);
  vtkm::Id numPoints = pointsHandle.GetNumberOfValues();
  vtkm::cont::ArrayHandle<Status> stepStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> stepResults;
  integratorTesterDispatcher.Invoke(pointsHandle, integrator, stepStatus, stepResults);
  auto statusPortal = stepStatus.ReadPortal();
  auto resultsPortal = stepResults.ReadPortal();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);

    VTKM_TEST_ASSERT(status.CheckOk(), "Error in evaluator for " + msg);
    VTKM_TEST_ASSERT(status.CheckSpatialBounds(), "Error in evaluator for " + msg);
    //Result should be push just outside of the bounds.
    vtkm::Vec3f result = resultsPortal.Get(index);
    VTKM_TEST_ASSERT(!bounds.Contains(result),
                     "Integrator did not step out of boundary for " + msg);
  }
}

void TestEvaluators()
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::particleadvection::Stepper<RK4Type, GridEvalType>;

  std::vector<vtkm::Vec3f> vecs;
  vtkm::FloatDefault vals[3] = { -1., 0., 1. };
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      for (int k = 0; k < 3; k++)
        if (!(i == 1 && j == 1 && k == 1)) //don't add a [0,0,0] vec.
          vecs.push_back(vtkm::Vec3f(vals[i], vals[j], vals[k]));

  std::vector<vtkm::Bounds> bounds;
  bounds.push_back(vtkm::Bounds(0, 10, 0, 10, 0, 10));
  bounds.push_back(vtkm::Bounds(-1, 1, -1, 1, -1, 1));
  bounds.push_back(vtkm::Bounds(0, 1, 0, 1, -1, 1));
  bounds.push_back(vtkm::Bounds(0, 1000, 0, 1, -1, 1000));
  bounds.push_back(vtkm::Bounds(0, 1000, -100, 0, -1, 1000));

  std::vector<vtkm::Id3> dims;
  dims.push_back(vtkm::Id3(5, 5, 5));
  dims.push_back(vtkm::Id3(10, 5, 5));

  for (auto& dim : dims)
  {
    for (auto& vec : vecs)
    {
      for (auto& bound : bounds)
      {
        auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bound, dim, false);

        vtkm::cont::ArrayHandle<vtkm::Vec3f> vecField;
        CreateConstantVectorField(dim[0] * dim[1] * dim[2], vec, vecField);
        FieldType velocities(vecField);

        //vtkm::FloatDefault stepSize = 0.01f;
        vtkm::FloatDefault stepSize = 0.1f;
        std::vector<vtkm::Particle> pointIns;
        std::vector<vtkm::Vec3f> stepResult;

        //Generate points 2 steps inside the bounding box.
        vtkm::Bounds interiorBounds = bound;
        interiorBounds.X.Min += 2 * stepSize;
        interiorBounds.Y.Min += 2 * stepSize;
        interiorBounds.Z.Min += 2 * stepSize;
        interiorBounds.X.Max -= 2 * stepSize;
        interiorBounds.Y.Max -= 2 * stepSize;
        interiorBounds.Z.Max -= 2 * stepSize;

        GenerateRandomParticles(pointIns, 38, interiorBounds);
        for (auto& p : pointIns)
          stepResult.push_back(p.Pos + vec * stepSize);

        vtkm::Range xRange, yRange, zRange;

        if (vec[0] > 0)
          xRange = vtkm::Range(bound.X.Max - stepSize / 2., bound.X.Max);
        else
          xRange = vtkm::Range(bound.X.Min, bound.X.Min + stepSize / 2.);
        if (vec[1] > 0)
          yRange = vtkm::Range(bound.Y.Max - stepSize / 2., bound.Y.Max);
        else
          yRange = vtkm::Range(bound.Y.Min, bound.Y.Min + stepSize / 2.);
        if (vec[2] > 0)
          zRange = vtkm::Range(bound.Z.Max - stepSize / 2., bound.Z.Max);
        else
          zRange = vtkm::Range(bound.Z.Min, bound.Z.Min + stepSize / 2.);

        vtkm::Bounds forBoundary(xRange, yRange, zRange);

        // Generate a bunch of boundary points towards the face of the direction
        // of the velocity field
        // All velocities are in the +ve direction.

        std::vector<vtkm::Particle> boundaryPoints;
        GenerateRandomParticles(boundaryPoints, 10, forBoundary, 919);

        for (auto& ds : dataSets)
        {
          GridEvalType gridEval(ds.GetCoordinateSystem(), ds.GetCellSet(), velocities);
          ValidateEvaluator(gridEval, pointIns, vec, "grid evaluator");

          Stepper rk4(gridEval, stepSize);
          ValidateIntegrator(rk4, pointIns, stepResult, "constant vector RK4");
          ValidateIntegratorForBoundary(bound, rk4, boundaryPoints, "constant vector RK4");
        }
      }
    }
  }
}

void TestGhostCellEvaluators()
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::particleadvection::Stepper<RK4Type, GridEvalType>;

  constexpr vtkm::Id nX = 6;
  constexpr vtkm::Id nY = 6;
  constexpr vtkm::Id nZ = 6;

  vtkm::Bounds bounds(0,
                      static_cast<vtkm::FloatDefault>(nX),
                      0,
                      static_cast<vtkm::FloatDefault>(nY),
                      0,
                      static_cast<vtkm::FloatDefault>(nZ));
  vtkm::Id3 dims(nX + 1, nY + 1, nZ + 1);

  auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, true);
  for (auto& ds : dataSets)
  {
    vtkm::cont::ArrayHandle<vtkm::Vec3f> vecField;
    vtkm::Vec3f vec(1, 0, 0);
    CreateConstantVectorField(dims[0] * dims[1] * dims[2], vec, vecField);
    //    ds.AddPointField("vec", vecField);
    FieldType velocities(vecField);

    GridEvalType gridEval(ds, velocities);

    vtkm::FloatDefault stepSize = static_cast<vtkm::FloatDefault>(0.1);
    Stepper rk4(gridEval, stepSize);

    vtkm::worklet::ParticleAdvection pa;
    std::vector<vtkm::Particle> seeds;
    //Points in a ghost cell.
    seeds.push_back(vtkm::Particle(vtkm::Vec3f(.5, .5, .5), 0));
    seeds.push_back(vtkm::Particle(vtkm::Vec3f(.5, 3, 3), 1));
    seeds.push_back(vtkm::Particle(vtkm::Vec3f(5.5, 5.5, 5.5), 2));

    //Point inside
    seeds.push_back(vtkm::Particle(vtkm::Vec3f(3, 3, 3), 3));

    auto seedArray = vtkm::cont::make_ArrayHandle(seeds, vtkm::CopyFlag::Off);
    auto res = pa.Run(rk4, seedArray, 10000);

    auto posPortal = res.Particles.ReadPortal();
    vtkm::Id numSeeds = seedArray.GetNumberOfValues();
    for (vtkm::Id i = 0; i < numSeeds; i++)
    {
      const auto& p = posPortal.Get(i);
      VTKM_TEST_ASSERT(p.Status.CheckSpatialBounds(), "Particle did not leave the dataset.");
      VTKM_TEST_ASSERT(p.Status.CheckInGhostCell(), "Particle did not end up in ghost cell.");

      //Particles that start in a ghost cell should take no steps.
      if (p.ID == 0 || p.ID == 1 || p.ID == 2)
        VTKM_TEST_ASSERT(p.NumSteps == 0, "Particle in ghost cell should *not* take any steps");
      else if (p.ID == 3)
        VTKM_TEST_ASSERT(p.NumSteps == 21, "Wrong number of steps for particle with ghost cells");
    }
  }
}

void ValidateParticleAdvectionResult(
  const vtkm::worklet::ParticleAdvectionResult<vtkm::Particle>& res,
  vtkm::Id nSeeds,
  vtkm::Id maxSteps)
{
  VTKM_TEST_ASSERT(res.Particles.GetNumberOfValues() == nSeeds,
                   "Number of output particles does not match input.");
  auto portal = res.Particles.ReadPortal();
  for (vtkm::Id i = 0; i < nSeeds; i++)
  {
    auto stepsTaken = portal.Get(i).NumSteps;
    auto status = portal.Get(i).Status;
    VTKM_TEST_ASSERT(stepsTaken <= maxSteps, "Too many steps taken in particle advection");
    if (stepsTaken == maxSteps)
      VTKM_TEST_ASSERT(status.CheckTerminate(), "Particle expected to be terminated");
    else
      VTKM_TEST_ASSERT(status.CheckSpatialBounds() || status.CheckTemporalBounds(),
                       "Particle expected to be outside spatial/temporal bounds");
  }
}

void ValidateStreamlineResult(const vtkm::worklet::StreamlineResult<vtkm::Particle>& res,
                              vtkm::Id nSeeds,
                              vtkm::Id maxSteps)
{
  VTKM_TEST_ASSERT(res.PolyLines.GetNumberOfCells() == nSeeds,
                   "Number of output streamlines does not match input.");
  auto portal = res.Particles.ReadPortal();
  for (vtkm::Id i = 0; i < nSeeds; i++)
  {
    VTKM_TEST_ASSERT(portal.Get(i).NumSteps <= maxSteps, "Too many steps taken in streamline");
    VTKM_TEST_ASSERT(portal.Get(i).Status.CheckOk(), "Bad status in streamline");
  }
  VTKM_TEST_ASSERT(res.Particles.GetNumberOfValues() == nSeeds,
                   "Number of output particles does not match input.");
}

void TestIntegrators()
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;

  const vtkm::Id3 dims(5, 5, 5);
  const vtkm::Bounds bounds(0., 1., 0., 1., .0, .1);
  auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, false);

  const vtkm::Id nSeeds = 3;
  const vtkm::Id maxSteps = 10;
  const vtkm::FloatDefault stepSize = 0.01f;

  vtkm::Id nElements = dims[0] * dims[1] * dims[2];
  std::vector<vtkm::Vec3f> fieldData;
  for (vtkm::Id i = 0; i < nElements; i++)
    fieldData.push_back(vtkm::Vec3f(0., 0., 1.));
  FieldHandle fieldValues = vtkm::cont::make_ArrayHandle(fieldData, vtkm::CopyFlag::Off);
  FieldType velocities(fieldValues);

  for (auto& ds : dataSets)
  {
    GridEvalType eval(ds, velocities);

    //Generate three random points.
    std::vector<vtkm::Particle> points;
    GenerateRandomParticles(points, 3, bounds);

    vtkm::worklet::ParticleAdvection pa;
    vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> res;
    {
      auto seeds = vtkm::cont::make_ArrayHandle(points, vtkm::CopyFlag::On);
      using IntegratorType = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
      using Stepper = vtkm::worklet::particleadvection::Stepper<IntegratorType, GridEvalType>;
      Stepper rk4(eval, stepSize);
      res = pa.Run(rk4, seeds, maxSteps);
      ValidateParticleAdvectionResult(res, nSeeds, maxSteps);
    }
    {
      auto seeds = vtkm::cont::make_ArrayHandle(points, vtkm::CopyFlag::On);
      using IntegratorType = vtkm::worklet::particleadvection::EulerIntegrator<GridEvalType>;
      using Stepper = vtkm::worklet::particleadvection::Stepper<IntegratorType, GridEvalType>;
      Stepper euler(eval, stepSize);
      res = pa.Run(euler, seeds, maxSteps);
      ValidateParticleAdvectionResult(res, nSeeds, maxSteps);
    }
  }
}

void TestParticleWorkletsWithDataSetTypes()
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::particleadvection::Stepper<RK4Type, GridEvalType>;
  vtkm::FloatDefault stepSize = 0.01f;

  const vtkm::Id3 dims(5, 5, 5);
  vtkm::Id nElements = dims[0] * dims[1] * dims[2] * 3;

  std::vector<vtkm::Vec3f> field;
  for (vtkm::Id i = 0; i < nElements; i++)
  {
    vtkm::FloatDefault x = vecData[i];
    vtkm::FloatDefault y = vecData[++i];
    vtkm::FloatDefault z = vecData[++i];
    vtkm::Vec3f vec(x, y, z);
    field.push_back(vtkm::Normal(vec));
  }
  vtkm::cont::ArrayHandle<vtkm::Vec3f> fieldArray;
  fieldArray = vtkm::cont::make_ArrayHandle(field, vtkm::CopyFlag::Off);
  FieldType velocities(fieldArray);

  std::vector<vtkm::Bounds> bounds;
  bounds.push_back(vtkm::Bounds(0, 10, 0, 10, 0, 10));
  bounds.push_back(vtkm::Bounds(-1, 1, -1, 1, -1, 1));
  bounds.push_back(vtkm::Bounds(0, 1, 0, 1, -1, 1));

  vtkm::Id maxSteps = 1000;
  for (auto& bound : bounds)
  {
    auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bound, dims, false);

    //Generate three random points.
    std::vector<vtkm::Particle> pts;
    GenerateRandomParticles(pts, 3, bound, 111);
    std::vector<vtkm::Particle> pts2 = pts;

    vtkm::Id nSeeds = static_cast<vtkm::Id>(pts.size());
    std::vector<vtkm::Id> stepsTaken = { 10, 20, 600 };
    for (std::size_t i = 0; i < stepsTaken.size(); i++)
      pts2[i].NumSteps = stepsTaken[i];

    for (auto& ds : dataSets)
    {
      GridEvalType eval(ds.GetCoordinateSystem(), ds.GetCellSet(), velocities);
      Stepper rk4(eval, stepSize);

      //Do 4 tests on each dataset.
      //Particle advection worklet with and without steps taken.
      //Streamline worklet with and without steps taken.
      for (int i = 0; i < 4; i++)
      {
        if (i < 2)
        {
          vtkm::worklet::ParticleAdvection pa;
          vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> res;
          if (i == 0)
          {
            auto seeds = vtkm::cont::make_ArrayHandle(pts, vtkm::CopyFlag::On);
            res = pa.Run(rk4, seeds, maxSteps);
          }
          else
          {
            auto seeds = vtkm::cont::make_ArrayHandle(pts2, vtkm::CopyFlag::On);
            res = pa.Run(rk4, seeds, maxSteps);
          }
          ValidateParticleAdvectionResult(res, nSeeds, maxSteps);
        }
        else
        {
          vtkm::worklet::Streamline s;
          vtkm::worklet::StreamlineResult<vtkm::Particle> res;
          if (i == 2)
          {
            auto seeds = vtkm::cont::make_ArrayHandle(pts, vtkm::CopyFlag::On);
            res = s.Run(rk4, seeds, maxSteps);
          }
          else
          {
            auto seeds = vtkm::cont::make_ArrayHandle(pts2, vtkm::CopyFlag::On);
            res = s.Run(rk4, seeds, maxSteps);
          }
          ValidateStreamlineResult(res, nSeeds, maxSteps);
        }
      }
    }
  }
}

void TestParticleStatus()
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;

  vtkm::Bounds bounds(0, 1, 0, 1, 0, 1);
  const vtkm::Id3 dims(5, 5, 5);
  vtkm::Id nElements = dims[0] * dims[1] * dims[2];

  FieldHandle fieldArray;
  CreateConstantVectorField(nElements, vtkm::Vec3f(1, 0, 0), fieldArray);

  auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, false);
  for (auto& ds : dataSets)
  {
    using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
    using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
    using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
    using Stepper = vtkm::worklet::particleadvection::Stepper<RK4Type, GridEvalType>;

    vtkm::Id maxSteps = 1000;
    vtkm::FloatDefault stepSize = 0.01f;

    FieldType velocities(fieldArray);

    GridEvalType eval(ds, velocities);
    Stepper rk4(eval, stepSize);

    vtkm::worklet::ParticleAdvection pa;
    std::vector<vtkm::Particle> pts;
    pts.push_back(vtkm::Particle(vtkm::Vec3f(.5, .5, .5), 0));
    pts.push_back(vtkm::Particle(vtkm::Vec3f(-1, -1, -1), 1));
    auto seedsArray = vtkm::cont::make_ArrayHandle(pts, vtkm::CopyFlag::On);
    pa.Run(rk4, seedsArray, maxSteps);
    auto portal = seedsArray.ReadPortal();

    bool tookStep0 = portal.Get(0).Status.CheckTookAnySteps();
    bool tookStep1 = portal.Get(1).Status.CheckTookAnySteps();
    VTKM_TEST_ASSERT(tookStep0 == true, "Particle failed to take any steps");
    VTKM_TEST_ASSERT(tookStep1 == false, "Particle took a step when it should not have.");
  }
}

void TestWorkletsBasic()
{
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::particleadvection::Stepper<RK4Type, GridEvalType>;
  vtkm::FloatDefault stepSize = 0.01f;

  const vtkm::Id3 dims(5, 5, 5);
  vtkm::Id nElements = dims[0] * dims[1] * dims[2] * 3;

  std::vector<vtkm::Vec3f> field;
  vtkm::Vec3f vecDir(1, 0, 0);
  for (vtkm::Id i = 0; i < nElements; i++)
    field.push_back(vtkm::Normal(vecDir));

  vtkm::cont::ArrayHandle<vtkm::Vec3f> fieldArray;
  fieldArray = vtkm::cont::make_ArrayHandle(field, vtkm::CopyFlag::Off);
  FieldType velocities(fieldArray);

  vtkm::Bounds bounds(0, 1, 0, 1, 0, 1);

  auto dataSets = vtkm::worklet::testing::CreateAllDataSets(bounds, dims, false);
  for (auto& ds : dataSets)
  {
    GridEvalType eval(ds, velocities);
    Stepper rk4(eval, stepSize);

    vtkm::Id maxSteps = 83;
    std::vector<std::string> workletTypes = { "particleAdvection", "streamline" };
    vtkm::FloatDefault endT = stepSize * static_cast<vtkm::FloatDefault>(maxSteps);

    for (auto w : workletTypes)
    {
      std::vector<vtkm::Particle> particles;
      std::vector<vtkm::Vec3f> pts, samplePts, endPts;
      vtkm::FloatDefault X = static_cast<vtkm::FloatDefault>(.1);
      vtkm::FloatDefault Y = static_cast<vtkm::FloatDefault>(.1);
      vtkm::FloatDefault Z = static_cast<vtkm::FloatDefault>(.1);

      for (int i = 0; i < 8; i++)
      {
        pts.push_back(vtkm::Vec3f(X, Y, Z));
        Y += static_cast<vtkm::FloatDefault>(.1);
      }

      vtkm::Id id = 0;
      for (std::size_t i = 0; i < pts.size(); i++, id++)
      {
        vtkm::Vec3f p = pts[i];
        particles.push_back(vtkm::Particle(p, id));
        samplePts.push_back(p);
        for (vtkm::Id j = 0; j < maxSteps; j++)
        {
          p = p + vecDir * stepSize;
          samplePts.push_back(p);
        }
        endPts.push_back(p);
      }

      auto seedsArray = vtkm::cont::make_ArrayHandle(particles, vtkm::CopyFlag::On);

      if (w == "particleAdvection")
      {
        vtkm::worklet::ParticleAdvection pa;
        vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> res;

        res = pa.Run(rk4, seedsArray, maxSteps);

        vtkm::Id numRequiredPoints = static_cast<vtkm::Id>(endPts.size());
        VTKM_TEST_ASSERT(res.Particles.GetNumberOfValues() == numRequiredPoints,
                         "Wrong number of points in particle advection result.");
        auto portal = res.Particles.ReadPortal();
        for (vtkm::Id i = 0; i < res.Particles.GetNumberOfValues(); i++)
        {
          VTKM_TEST_ASSERT(portal.Get(i).Pos == endPts[static_cast<std::size_t>(i)],
                           "Particle advection point is wrong");
          VTKM_TEST_ASSERT(portal.Get(i).NumSteps == maxSteps,
                           "Particle advection NumSteps is wrong");
          VTKM_TEST_ASSERT(vtkm::Abs(portal.Get(i).Time - endT) < stepSize / 100,
                           "Particle advection Time is wrong");
          VTKM_TEST_ASSERT(portal.Get(i).Status.CheckOk(), "Particle advection Status is wrong");
          VTKM_TEST_ASSERT(portal.Get(i).Status.CheckTerminate(),
                           "Particle advection particle did not terminate");
        }
      }
      else if (w == "streamline")
      {
        vtkm::worklet::Streamline s;
        vtkm::worklet::StreamlineResult<vtkm::Particle> res;

        res = s.Run(rk4, seedsArray, maxSteps);

        vtkm::Id numRequiredPoints = static_cast<vtkm::Id>(samplePts.size());
        VTKM_TEST_ASSERT(res.Positions.GetNumberOfValues() == numRequiredPoints,
                         "Wrong number of points in streamline result.");

        //Make sure all the points match.
        auto parPortal = res.Particles.ReadPortal();
        for (vtkm::Id i = 0; i < res.Particles.GetNumberOfValues(); i++)
        {
          VTKM_TEST_ASSERT(parPortal.Get(i).Pos == endPts[static_cast<std::size_t>(i)],
                           "Streamline end point is wrong");
          VTKM_TEST_ASSERT(parPortal.Get(i).NumSteps == maxSteps, "Streamline NumSteps is wrong");
          VTKM_TEST_ASSERT(vtkm::Abs(parPortal.Get(i).Time - endT) < stepSize / 100,
                           "Streamline Time is wrong");
          VTKM_TEST_ASSERT(parPortal.Get(i).Status.CheckOk(), "Streamline Status is wrong");
          VTKM_TEST_ASSERT(parPortal.Get(i).Status.CheckTerminate(),
                           "Streamline particle did not terminate");
        }

        auto posPortal = res.Positions.ReadPortal();
        for (vtkm::Id i = 0; i < res.Positions.GetNumberOfValues(); i++)
          VTKM_TEST_ASSERT(posPortal.Get(i) == samplePts[static_cast<std::size_t>(i)],
                           "Streamline points do not match");

        vtkm::Id numCells = res.PolyLines.GetNumberOfCells();
        VTKM_TEST_ASSERT(numCells == static_cast<vtkm::Id>(pts.size()),
                         "Wrong number of polylines in streamline");
        for (vtkm::Id i = 0; i < numCells; i++)
        {
          VTKM_TEST_ASSERT(res.PolyLines.GetCellShape(i) == vtkm::CELL_SHAPE_POLY_LINE,
                           "Wrong cell type in streamline.");
          VTKM_TEST_ASSERT(res.PolyLines.GetNumberOfPointsInCell(i) ==
                             static_cast<vtkm::Id>(maxSteps + 1),
                           "Wrong number of points in streamline cell");
        }
      }
    }
  }
}

template <class ResultType>
void ValidateResult(const ResultType& res,
                    vtkm::Id maxSteps,
                    const std::vector<vtkm::Vec3f>& endPts)
{
  const vtkm::FloatDefault eps = static_cast<vtkm::FloatDefault>(1e-3);
  vtkm::Id numPts = static_cast<vtkm::Id>(endPts.size());

  VTKM_TEST_ASSERT(res.Particles.GetNumberOfValues() == numPts,
                   "Wrong number of points in particle advection result.");

  auto portal = res.Particles.ReadPortal();
  for (vtkm::Id i = 0; i < 3; i++)
  {
    vtkm::Vec3f p = portal.Get(i).Pos;
    vtkm::Vec3f e = endPts[static_cast<std::size_t>(i)];

    VTKM_TEST_ASSERT(vtkm::Magnitude(p - e) <= eps, "Particle advection point is wrong");
    VTKM_TEST_ASSERT(portal.Get(i).NumSteps == maxSteps, "Particle advection NumSteps is wrong");
    VTKM_TEST_ASSERT(portal.Get(i).Status.CheckOk(), "Particle advection Status is wrong");
    VTKM_TEST_ASSERT(portal.Get(i).Status.CheckTerminate(),
                     "Particle advection particle did not terminate");
  }
}


void TestParticleAdvectionFile(const std::string& fname,
                               const std::vector<vtkm::Vec3f>& pts,
                               vtkm::FloatDefault stepSize,
                               vtkm::Id maxSteps,
                               const std::vector<vtkm::Vec3f>& endPts)
{

  VTKM_LOG_S(vtkm::cont::LogLevel::Info, "Testing particle advection on file " << fname);
  vtkm::io::VTKDataSetReader reader(fname);
  vtkm::cont::DataSet ds;
  try
  {
    ds = reader.ReadDataSet();
  }
  catch (vtkm::io::ErrorIO& e)
  {
    std::string message("Error reading: ");
    message += fname;
    message += ", ";
    message += e.GetMessage();

    VTKM_TEST_FAIL(message.c_str());
  }

  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::particleadvection::VelocityField<FieldHandle>;
  using GridEvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldType>;
  using RK4Type = vtkm::worklet::particleadvection::RK4Integrator<GridEvalType>;
  using Stepper = vtkm::worklet::particleadvection::Stepper<RK4Type, GridEvalType>;

  VTKM_TEST_ASSERT(ds.HasField("vec"), "Data set missing a field named 'vec'");
  vtkm::cont::Field& field = ds.GetField("vec");
  auto fieldData = field.GetData();

  FieldHandle fieldArray;

  // Get fieldData (from file) into an ArrayHandle of type vtkm::Vec3f
  // If types match, do a simple cast.
  // If not, need to copy it into the appropriate type.
  if (fieldData.IsType<FieldHandle>())
    fieldArray = fieldData.AsArrayHandle<FieldHandle>();
  else
    vtkm::cont::ArrayCopy(fieldData, fieldArray);

  FieldType velocities(fieldArray);
  GridEvalType eval(ds.GetCoordinateSystem(), ds.GetCellSet(), velocities);
  Stepper rk4(eval, stepSize);

  for (int i = 0; i < 2; i++)
  {
    std::vector<vtkm::Particle> seeds;
    for (size_t j = 0; j < pts.size(); j++)
      seeds.push_back(vtkm::Particle(pts[j], static_cast<vtkm::Id>(j)));
    auto seedArray = vtkm::cont::make_ArrayHandle(seeds, vtkm::CopyFlag::Off);

    if (i == 0)
    {
      vtkm::worklet::ParticleAdvection pa;
      vtkm::worklet::ParticleAdvectionResult<vtkm::Particle> res;

      res = pa.Run(rk4, seedArray, maxSteps);
      ValidateResult(res, maxSteps, endPts);
    }
    else if (i == 1)
    {
      vtkm::worklet::Streamline s;
      vtkm::worklet::StreamlineResult<vtkm::Particle> res;

      res = s.Run(rk4, seedArray, maxSteps);
      ValidateResult(res, maxSteps, endPts);
    }
  }
}

void TestParticleAdvection()
{
  TestIntegrators();
  TestEvaluators();
  TestGhostCellEvaluators();

  TestParticleStatus();
  TestWorkletsBasic();
  TestParticleWorkletsWithDataSetTypes();

  //Fusion test.
  std::vector<vtkm::Vec3f> fusionPts, fusionEndPts;
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.6f, 0.6f));
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.8f, 0.6f));
  fusionPts.push_back(vtkm::Vec3f(0.8f, 0.8f, 0.3f));
  //End point values were generated in VisIt.
  fusionEndPts.push_back(vtkm::Vec3f(0.5335789918f, 0.87112802267f, 0.6723330020f));
  fusionEndPts.push_back(vtkm::Vec3f(0.5601879954f, 0.91389900446f, 0.43989110522f));
  fusionEndPts.push_back(vtkm::Vec3f(0.7004770041f, 0.63193398714f, 0.64524400234f));
  vtkm::FloatDefault fusionStep = 0.005f;
  std::string fusionFile = vtkm::cont::testing::Testing::DataPath("rectilinear/fusion.vtk");
  TestParticleAdvectionFile(fusionFile, fusionPts, fusionStep, 1000, fusionEndPts);

  //Fishtank test.
  std::vector<vtkm::Vec3f> fishPts, fishEndPts;
  fishPts.push_back(vtkm::Vec3f(0.75f, 0.5f, 0.01f));
  fishPts.push_back(vtkm::Vec3f(0.4f, 0.2f, 0.7f));
  fishPts.push_back(vtkm::Vec3f(0.5f, 0.3f, 0.8f));
  //End point values were generated in VisIt.
  fishEndPts.push_back(vtkm::Vec3f(0.7734669447f, 0.4870159328f, 0.8979591727f));
  fishEndPts.push_back(vtkm::Vec3f(0.7257543206f, 0.1277695596f, 0.7468645573f));
  fishEndPts.push_back(vtkm::Vec3f(0.8347796798f, 0.1276152730f, 0.4985143244f));

  vtkm::FloatDefault fishStep = 0.001f;
  std::string fishFile = vtkm::cont::testing::Testing::DataPath("rectilinear/fishtank.vtk");
  TestParticleAdvectionFile(fishFile, fishPts, fishStep, 100, fishEndPts);
}

int UnitTestParticleAdvection(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestParticleAdvection, argc, argv);
}
