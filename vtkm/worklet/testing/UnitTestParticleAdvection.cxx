//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <typeinfo>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DataSetBuilderRectilinear.h>
#include <vtkm/cont/DataSetBuilderUniform.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/testing/Testing.h>
#include <vtkm/worklet/ParticleAdvection.h>
#include <vtkm/worklet/particleadvection/GridEvaluators.h>
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>

namespace
{

vtkm::Float32 vecData[125 * 3] = {
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

template <typename FieldType>
void RandomPoint(const vtkm::Bounds& bounds, vtkm::Vec<FieldType, 3>& p)
{
  FieldType rx = static_cast<FieldType>(rand()) / static_cast<FieldType>(RAND_MAX);
  FieldType ry = static_cast<FieldType>(rand()) / static_cast<FieldType>(RAND_MAX);
  FieldType rz = static_cast<FieldType>(rand()) / static_cast<FieldType>(RAND_MAX);

  p[0] = static_cast<FieldType>(bounds.X.Min + rx * bounds.X.Length());
  p[1] = static_cast<FieldType>(bounds.Y.Min + ry * bounds.Y.Length());
  p[2] = static_cast<FieldType>(bounds.Z.Min + rz * bounds.Z.Length());
}

template <typename FieldType>
vtkm::cont::DataSet CreateUniformDataSet(const vtkm::Bounds& bounds, const vtkm::Id3& dims)
{
  vtkm::Vec<FieldType, 3> origin(static_cast<FieldType>(bounds.X.Min),
                                 static_cast<FieldType>(bounds.Y.Min),
                                 static_cast<FieldType>(bounds.Z.Min));
  vtkm::Vec<FieldType, 3> spacing(
    static_cast<FieldType>(bounds.X.Length()) / static_cast<FieldType>((dims[0] - 1)),
    static_cast<FieldType>(bounds.Y.Length()) / static_cast<FieldType>((dims[1] - 1)),
    static_cast<FieldType>(bounds.Z.Length()) / static_cast<FieldType>((dims[2] - 1)));

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;
  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims, origin, spacing);
  return ds;
}

template <typename FieldType>
vtkm::cont::DataSet CreateRectilinearDataSet(const vtkm::Bounds& bounds, const vtkm::Id3& dims)
{
  vtkm::cont::DataSetBuilderRectilinear dataSetBuilder;
  std::vector<FieldType> xvals, yvals, zvals;

  vtkm::Vec<FieldType, 3> spacing(
    static_cast<FieldType>(bounds.X.Length()) / static_cast<FieldType>((dims[0] - 1)),
    static_cast<FieldType>(bounds.Y.Length()) / static_cast<FieldType>((dims[1] - 1)),
    static_cast<FieldType>(bounds.Z.Length()) / static_cast<FieldType>((dims[2] - 1)));
  xvals.resize((size_t)dims[0]);
  xvals[0] = static_cast<FieldType>(bounds.X.Min);
  for (size_t i = 1; i < (size_t)dims[0]; i++)
    xvals[i] = xvals[i - 1] + spacing[0];

  yvals.resize((size_t)dims[1]);
  yvals[0] = static_cast<FieldType>(bounds.Y.Min);
  for (size_t i = 1; i < (size_t)dims[1]; i++)
    yvals[i] = yvals[i - 1] + spacing[1];

  zvals.resize((size_t)dims[2]);
  zvals[0] = static_cast<FieldType>(bounds.Z.Min);
  for (size_t i = 1; i < (size_t)dims[2]; i++)
    zvals[i] = zvals[i - 1] + spacing[2];

  vtkm::cont::DataSet ds = dataSetBuilder.Create(xvals, yvals, zvals);
  return ds;
}

template <typename FieldType>
void CreateConstantVectorField(vtkm::Id num,
                               const vtkm::Vec<FieldType, 3>& vec,
                               vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>& vecField)
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using DeviceAlgorithm = typename vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>;

  vtkm::cont::ArrayHandleConstant<vtkm::Vec<FieldType, 3>> vecConst;
  vecConst = vtkm::cont::make_ArrayHandleConstant(vec, num);
  DeviceAlgorithm::Copy(vecConst, vecField);
}

template <typename FieldType, typename Evaluator>
class TestEvaluatorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  TestEvaluatorWorklet(Evaluator e)
    : evaluator(e){};

  using ControlSignature = void(FieldIn<> inputPoint, FieldOut<> validity, FieldOut<> outputPoint);

  using ExecutionSignature = void(_1, _2, _3);

  VTKM_EXEC
  void operator()(vtkm::Vec<FieldType, 3>& pointIn,
                  bool& validity,
                  vtkm::Vec<FieldType, 3>& pointOut) const
  {
    validity = evaluator.Evaluate(pointIn, pointOut);
  }

private:
  Evaluator evaluator;
};

template <typename EvalType, typename FieldType>
void ValidateEvaluator(const EvalType& eval,
                       const std::vector<vtkm::Vec<FieldType, 3>>& pointIns,
                       const vtkm::Vec<FieldType, 3>& vec,
                       const std::string& msg)
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using EvalTester = TestEvaluatorWorklet<FieldType, EvalType>;
  using EvalTesterDispatcher = vtkm::worklet::DispatcherMapField<EvalTester>;
  EvalTester evalTester(eval);
  EvalTesterDispatcher evalTesterDispatcher(evalTester);
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> pointsHandle =
    vtkm::cont::make_ArrayHandle(pointIns);
  vtkm::Id numPoints = pointsHandle.GetNumberOfValues();
  pointsHandle.PrepareForInput(DeviceAdapter());
  vtkm::cont::ArrayHandle<bool> evalStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> evalResults;
  evalStatus.PrepareForOutput(numPoints, DeviceAdapter());
  evalResults.PrepareForOutput(numPoints, DeviceAdapter());
  evalTesterDispatcher.Invoke(pointsHandle, evalStatus, evalResults);
  auto statusPortal = evalStatus.GetPortalConstControl();
  auto resultsPortal = evalResults.GetPortalConstControl();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    bool status = statusPortal.Get(index);
    vtkm::Vec<FieldType, 3> result = resultsPortal.Get(index);
    VTKM_TEST_ASSERT(status, "Error in evaluator for " + msg);
    VTKM_TEST_ASSERT(result == vec, "Error in evaluator result for " + msg);
  }
  pointsHandle.ReleaseResources();
  evalStatus.ReleaseResources();
  evalResults.ReleaseResources();
}

template <typename FieldType, typename Integrator>
class TestIntegratorWorklet : public vtkm::worklet::WorkletMapField
{
public:
  TestIntegratorWorklet(Integrator i)
    : integrator(i){};

  using ControlSignature = void(FieldIn<> inputPoint, FieldOut<> validity, FieldOut<> outputPoint);

  using ExecutionSignature = void(_1, _2, _3);

  VTKM_EXEC
  void operator()(vtkm::Vec<FieldType, 3>& pointIn,
                  vtkm::worklet::particleadvection::ParticleStatus& status,
                  vtkm::Vec<FieldType, 3>& pointOut) const
  {
    FieldType time = 0;
    status = integrator.Step(pointIn, time, pointOut);
  }

private:
  Integrator integrator;
};


template <typename IntegratorType, typename FieldType>
void ValidateIntegrator(const IntegratorType& integrator,
                        const std::vector<vtkm::Vec<FieldType, 3>>& pointIns,
                        const std::vector<vtkm::Vec<FieldType, 3>>& expStepResults,
                        const std::string& msg)
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using IntegratorTester = TestIntegratorWorklet<FieldType, IntegratorType>;
  using IntegratorTesterDispatcher = vtkm::worklet::DispatcherMapField<IntegratorTester>;
  using Status = vtkm::worklet::particleadvection::ParticleStatus;
  IntegratorTester integratorTester(integrator);
  IntegratorTesterDispatcher integratorTesterDispatcher(integratorTester);
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> pointsHandle =
    vtkm::cont::make_ArrayHandle(pointIns);
  vtkm::Id numPoints = pointsHandle.GetNumberOfValues();
  pointsHandle.PrepareForInput(DeviceAdapter());
  vtkm::cont::ArrayHandle<Status> stepStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> stepResults;
  stepStatus.PrepareForOutput(numPoints, DeviceAdapter());
  stepResults.PrepareForOutput(numPoints, DeviceAdapter());
  integratorTesterDispatcher.Invoke(pointsHandle, stepStatus, stepResults);
  auto statusPortal = stepStatus.GetPortalConstControl();
  auto resultsPortal = stepResults.GetPortalConstControl();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);
    vtkm::Vec<FieldType, 3> result = resultsPortal.Get(index);
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
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using FieldType = vtkm::Float32;
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>;
  using FieldPortalConstType = FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;

  //Constant field evaluator and RK4 integrator.
  using CEvalType = vtkm::worklet::particleadvection::ConstantField<FieldType>;
  using RK4CType = vtkm::worklet::particleadvection::RK4Integrator<CEvalType, FieldType>;

  //Uniform grid evaluator and RK4 integrator.
  using UniformEvalType =
    vtkm::worklet::particleadvection::UniformGridEvaluate<FieldPortalConstType,
                                                          FieldType,
                                                          DeviceAdapter>;
  using RK4UniformType =
    vtkm::worklet::particleadvection::RK4Integrator<UniformEvalType, FieldType>;

  //Rectilinear grid evaluator and RK4 integrator.
  using RectilinearEvalType =
    vtkm::worklet::particleadvection::RectilinearGridEvaluate<FieldPortalConstType,
                                                              FieldType,
                                                              DeviceAdapter>;
  using RK4RectilinearType =
    vtkm::worklet::particleadvection::RK4Integrator<RectilinearEvalType, FieldType>;

  std::vector<vtkm::Vec<FieldType, 3>> vecs;
  vecs.push_back(vtkm::Vec<FieldType, 3>(1, 0, 0));
  vecs.push_back(vtkm::Vec<FieldType, 3>(0, 1, 0));
  vecs.push_back(vtkm::Vec<FieldType, 3>(0, 0, 1));
  vecs.push_back(vtkm::Vec<FieldType, 3>(1, 1, 0));
  vecs.push_back(vtkm::Vec<FieldType, 3>(0, 1, 1));
  vecs.push_back(vtkm::Vec<FieldType, 3>(1, 0, 1));
  vecs.push_back(vtkm::Vec<FieldType, 3>(1, 1, 1));

  std::vector<vtkm::Bounds> bounds;
  bounds.push_back(vtkm::Bounds(0, 10, 0, 10, 0, 10));
  bounds.push_back(vtkm::Bounds(-1, 1, -1, 1, -1, 1));
  bounds.push_back(vtkm::Bounds(0, 1, 0, 1, -1, 1));

  std::vector<vtkm::Id3> dims;
  dims.push_back(vtkm::Id3(5, 5, 5));
  dims.push_back(vtkm::Id3(10, 5, 5));
  dims.push_back(vtkm::Id3(10, 5, 5));

  std::vector<FieldType> steps;
  steps.push_back(0.01f);
  //  steps.push_back(0.05f);

  srand(314);
  for (std::size_t d = 0; d < dims.size(); d++)
  {
    vtkm::Id3 dim = dims[d];

    for (std::size_t i = 0; i < vecs.size(); i++)
    {
      vtkm::Vec<FieldType, 3> vec = vecs[i];

      for (std::size_t j = 0; j < bounds.size(); j++)
      {
        //Create uniform and rectilinear data.
        vtkm::cont::DataSet uniformData, rectData;
        vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> vecField;
        uniformData = CreateUniformDataSet<FieldType>(bounds[j], dim);
        rectData = CreateRectilinearDataSet<FieldType>(bounds[j], dim);
        CreateConstantVectorField(dim[0] * dim[1] * dim[2], vec, vecField);

        for (std::size_t s = 0; s < steps.size(); s++)
        {
          FieldType stepSize = steps[s];

          //Constant vector field evaluator.
          CEvalType constEval(bounds[j], vecs[i]);
          RK4CType constRK4(constEval, stepSize);


          //Uniform vector field evaluator
          UniformEvalType uniformEval(
            uniformData.GetCoordinateSystem(), uniformData.GetCellSet(0), vecField);
          RK4UniformType uniformRK4(uniformEval, stepSize);

          //Rectilinear grid evaluator.
          RectilinearEvalType rectEval(
            rectData.GetCoordinateSystem(), rectData.GetCellSet(0), vecField);
          RK4RectilinearType rectRK4(rectEval, stepSize);
          std::vector<vtkm::Vec<FieldType, 3>> pointIns;
          std::vector<vtkm::Vec<FieldType, 3>> stepResult;
          //Create a bunch of random points in the bounds.
          for (int k = 0; k < 38; k++)
          {
            //Generate points 2 steps inside the bounding box.
            vtkm::Bounds interiorBounds = bounds[j];
            interiorBounds.X.Min += 2 * stepSize;
            interiorBounds.Y.Min += 2 * stepSize;
            interiorBounds.Z.Min += 2 * stepSize;
            interiorBounds.X.Max -= 2 * stepSize;
            interiorBounds.Y.Max -= 2 * stepSize;
            interiorBounds.Z.Max -= 2 * stepSize;
            vtkm::Vec<FieldType, 3> p;
            RandomPoint<FieldType>(interiorBounds, p);
            pointIns.push_back(p);
            stepResult.push_back(p + vec * stepSize);
          }
          //Test the result for the evaluator
          ValidateEvaluator(constEval, pointIns, vec, "constant vector evaluator");
          ValidateEvaluator(uniformEval, pointIns, vec, "uniform evaluator");
          ValidateEvaluator(rectEval, pointIns, vec, "rectilinear evaluator");

          //Test taking one step.
          ValidateIntegrator(constRK4, pointIns, stepResult, "constant vector RK4");
          ValidateIntegrator(uniformRK4, pointIns, stepResult, "uniform RK4");
          ValidateIntegrator(rectRK4, pointIns, stepResult, "rectilinear evaluator");
        }
      }
    }
  }
}

void TestParticleWorklets()
{
  using DeviceAdapter = VTKM_DEFAULT_DEVICE_ADAPTER_TAG;
  using FieldType = vtkm::Float32;
  using FieldHandle = vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>>;
  using FieldPortalConstType = FieldHandle::template ExecutionTypes<DeviceAdapter>::PortalConst;

  FieldType stepSize = 0.01f;

  vtkm::cont::DataSetBuilderUniform dataSetBuilder;

  const vtkm::Id3 dims(5, 5, 5);
  vtkm::Id nElements = dims[0] * dims[1] * dims[2] * 3;

  std::vector<vtkm::Vec<vtkm::Float32, 3>> field;
  for (vtkm::Id i = 0; i < nElements; i++)
  {
    FieldType x = vecData[i];
    FieldType y = vecData[++i];
    FieldType z = vecData[++i];
    vtkm::Vec<FieldType, 3> vec(x, y, z);
    field.push_back(vtkm::Normal(vec));
  }
  vtkm::cont::DataSet ds = dataSetBuilder.Create(dims);

  using RGEvalType = vtkm::worklet::particleadvection::UniformGridEvaluate<FieldPortalConstType,
                                                                           FieldType,
                                                                           DeviceAdapter>;
  using RK4RGType = vtkm::worklet::particleadvection::RK4Integrator<RGEvalType, FieldType>;

  vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> fieldArray;
  fieldArray = vtkm::cont::make_ArrayHandle(field);

  RGEvalType eval(ds.GetCoordinateSystem(), ds.GetCellSet(0), fieldArray);
  RK4RGType rk4(eval, stepSize);

  for (int i = 0; i < 2; i++)
  {
    std::vector<vtkm::Vec<FieldType, 3>> pts;
    pts.push_back(vtkm::Vec<FieldType, 3>(1, 1, 1));
    pts.push_back(vtkm::Vec<FieldType, 3>(2, 2, 2));
    pts.push_back(vtkm::Vec<FieldType, 3>(3, 3, 3));

    vtkm::Id maxSteps = 1000;
    vtkm::Id nSeeds = static_cast<vtkm::Id>(pts.size());
    std::vector<vtkm::Id> stepsTaken = { 10, 20, 600 };

    if (i == 0)
    {
      for (int j = 0; j < 2; j++)
      {
        vtkm::worklet::ParticleAdvection particleAdvection;
        vtkm::worklet::ParticleAdvectionResult<FieldType> res;

        vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seeds;
        seeds.Allocate(nSeeds);
        for (vtkm::Id k = 0; k < nSeeds; k++)
          seeds.GetPortalControl().Set(k, pts[static_cast<size_t>(k)]);

        if (j == 0)
          res = particleAdvection.Run(rk4, seeds, maxSteps, DeviceAdapter());
        else if (j == 1)
        {
          vtkm::cont::ArrayHandle<vtkm::Id> stepsTakenArrayHandle =
            vtkm::cont::make_ArrayHandle(stepsTaken);
          res = particleAdvection.Run(rk4, seeds, stepsTakenArrayHandle, maxSteps, DeviceAdapter());
        }

        VTKM_TEST_ASSERT(res.positions.GetNumberOfValues() == seeds.GetNumberOfValues(),
                         "Number of output particles does not match input.");
        std::cout << "Test: " << i << " " << j << std::endl;
        vtkm::cont::printSummary_ArrayHandle(res.stepsTaken, std::cout);

        for (vtkm::Id k = 0; k < nSeeds; k++)
          VTKM_TEST_ASSERT(res.stepsTaken.GetPortalConstControl().Get(k) <= maxSteps,
                           "Too many steps taken in particle advection");
      }
    }
    else if (i == 1)
    {
      for (int j = 0; j < 2; j++)
      {
        vtkm::worklet::Streamline streamline;
        vtkm::worklet::StreamlineResult<FieldType> res;

        vtkm::cont::ArrayHandle<vtkm::Vec<FieldType, 3>> seeds;
        seeds.Allocate(nSeeds);
        for (vtkm::Id k = 0; k < nSeeds; k++)
          seeds.GetPortalControl().Set(k, pts[static_cast<size_t>(k)]);

        if (j == 0)
          res = streamline.Run(rk4, seeds, maxSteps, DeviceAdapter());
        else if (j == 1)
        {
          vtkm::cont::ArrayHandle<vtkm::Id> stepsTakenArrayHandle =
            vtkm::cont::make_ArrayHandle(stepsTaken);
          res = streamline.Run(rk4, seeds, stepsTakenArrayHandle, maxSteps, DeviceAdapter());
        }

        //Make sure we have the right number of streamlines.
        VTKM_TEST_ASSERT(res.polyLines.GetNumberOfCells() == seeds.GetNumberOfValues(),
                         "Number of output streamlines does not match input.");

        std::cout << "Test: " << i << " " << j << std::endl;
        vtkm::cont::printSummary_ArrayHandle(res.stepsTaken, std::cout);
        //Make sure we have the right number of samples in each streamline.
        nSeeds = static_cast<vtkm::Id>(pts.size());
        for (vtkm::Id k = 0; k < nSeeds; k++)
        {
          vtkm::Id numPoints = static_cast<vtkm::Id>(res.polyLines.GetNumberOfPointsInCell(k));
          vtkm::Id numSteps = res.stepsTaken.GetPortalConstControl().Get(k);
          if (j != 1)
          {
            VTKM_TEST_ASSERT(numPoints == numSteps, "Invalid number of points in streamline.");
          }
          VTKM_TEST_ASSERT(res.stepsTaken.GetPortalConstControl().Get(k) <= maxSteps,
                           "Too many steps taken in streamline");
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

int UnitTestParticleAdvection(int, char* [])
{
  return vtkm::cont::testing::Testing::Run(TestParticleAdvection);
}
