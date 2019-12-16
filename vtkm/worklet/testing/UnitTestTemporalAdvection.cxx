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
#include <vtkm/worklet/particleadvection/Integrators.h>
#include <vtkm/worklet/particleadvection/Particles.h>
#include <vtkm/worklet/particleadvection/TemporalGridEvaluators.h>

#include <vtkm/io/writer/VTKDataSetWriter.h>

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
    status = evaluator.Evaluate(pointIn.Pos, 0.5f, pointOut);
  }
};

template <typename EvalType>
void ValidateEvaluator(const EvalType& eval,
                       const vtkm::cont::ArrayHandle<vtkm::Particle>& pointIns,
                       const vtkm::cont::ArrayHandle<vtkm::Vec3f>& validity,
                       const std::string& msg)
{
  using EvalTester = TestEvaluatorWorklet;
  using EvalTesterDispatcher = vtkm::worklet::DispatcherMapField<EvalTester>;
  using Status = vtkm::worklet::particleadvection::GridEvaluatorStatus;

  EvalTester evalTester;
  EvalTesterDispatcher evalTesterDispatcher(evalTester);
  vtkm::Id numPoints = pointIns.GetNumberOfValues();
  vtkm::cont::ArrayHandle<Status> evalStatus;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> evalResults;
  evalTesterDispatcher.Invoke(pointIns, eval, evalStatus, evalResults);
  auto statusPortal = evalStatus.GetPortalConstControl();
  auto resultsPortal = evalResults.GetPortalConstControl();
  auto validityPortal = validity.GetPortalConstControl();
  for (vtkm::Id index = 0; index < numPoints; index++)
  {
    Status status = statusPortal.Get(index);
    vtkm::Vec3f result = resultsPortal.Get(index);
    vtkm::Vec3f expected = validityPortal.Get(index);
    VTKM_TEST_ASSERT(status.CheckOk(), "Error in evaluator for " + msg);
    VTKM_TEST_ASSERT(result == expected, "Error in evaluator result for " + msg);
  }
  evalStatus.ReleaseResources();
  evalResults.ReleaseResources();
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

vtkm::Vec3f RandomPt(const vtkm::Bounds& bounds)
{
  vtkm::FloatDefault rx =
    static_cast<vtkm::FloatDefault>(rand()) / static_cast<vtkm::FloatDefault>(RAND_MAX);
  vtkm::FloatDefault ry =
    static_cast<vtkm::FloatDefault>(rand()) / static_cast<vtkm::FloatDefault>(RAND_MAX);
  vtkm::FloatDefault rz =
    static_cast<vtkm::FloatDefault>(rand()) / static_cast<vtkm::FloatDefault>(RAND_MAX);

  vtkm::Vec3f p;
  p[0] = static_cast<vtkm::FloatDefault>(bounds.X.Min + rx * bounds.X.Length());
  p[1] = static_cast<vtkm::FloatDefault>(bounds.Y.Min + ry * bounds.Y.Length());
  p[2] = static_cast<vtkm::FloatDefault>(bounds.Z.Min + rz * bounds.Z.Length());
  return p;
}

void GeneratePoints(const vtkm::Id numOfEntries,
                    const vtkm::Bounds& bounds,
                    vtkm::cont::ArrayHandle<vtkm::Particle>& pointIns)
{
  pointIns.Allocate(numOfEntries);
  auto writePortal = pointIns.GetPortalControl();
  for (vtkm::Id index = 0; index < numOfEntries; index++)
  {
    vtkm::Particle particle(RandomPt(bounds), index);
    writePortal.Set(index, particle);
  }
}

void GenerateValidity(const vtkm::Id numOfEntries,
                      vtkm::cont::ArrayHandle<vtkm::Vec3f>& validity,
                      const vtkm::Vec3f& vecOne,
                      const vtkm::Vec3f& vecTwo)
{
  validity.Allocate(numOfEntries);
  auto writePortal = validity.GetPortalControl();
  for (vtkm::Id index = 0; index < numOfEntries; index++)
  {
    vtkm::Vec3f value = 0.5f * vecOne + (1.0f - 0.5f) * vecTwo;
    writePortal.Set(index, value);
  }
}

void TestTemporalEvaluators()
{
  using ScalarType = vtkm::FloatDefault;
  using PointType = vtkm::Vec<ScalarType, 3>;
  using FieldHandle = vtkm::cont::ArrayHandle<PointType>;
  using EvalType = vtkm::worklet::particleadvection::GridEvaluator<FieldHandle>;
  using TemporalEvalType = vtkm::worklet::particleadvection::TemporalGridEvaluator<FieldHandle>;

  // Create Datasets
  vtkm::Id3 dims(5, 5, 5);
  vtkm::Bounds bounds(0, 10, 0, 10, 0, 10);
  vtkm::cont::DataSet sliceOne = CreateUniformDataSet<ScalarType>(bounds, dims);
  vtkm::cont::DataSet sliceTwo = CreateUniformDataSet<ScalarType>(bounds, dims);

  // Create Vector Field
  PointType X(1, 0, 0);
  PointType Z(0, 0, 1);
  vtkm::cont::ArrayHandle<PointType> alongX, alongZ;
  CreateConstantVectorField(125, X, alongX);
  CreateConstantVectorField(125, Z, alongZ);

  // Send them to test
  EvalType evalOne(sliceOne.GetCoordinateSystem(), sliceOne.GetCellSet(), alongX);
  EvalType evalTwo(sliceTwo.GetCoordinateSystem(), sliceTwo.GetCellSet(), alongZ);

  // Test data : populate with meaningful values
  vtkm::Id numValues = 10;
  vtkm::cont::ArrayHandle<vtkm::Particle> pointIns;
  vtkm::cont::ArrayHandle<vtkm::Vec3f> validity;
  GeneratePoints(numValues, bounds, pointIns);
  GenerateValidity(numValues, validity, X, Z);

  vtkm::FloatDefault timeOne(0.0f), timeTwo(1.0f);
  TemporalEvalType gridEval(evalOne, timeOne, evalTwo, timeTwo);
  ValidateEvaluator(gridEval, pointIns, validity, "grid evaluator");
}

void TestTemporalAdvection()
{
  TestTemporalEvaluators();
}

int UnitTestTemporalAdvection(int argc, char* argv[])
{
  return vtkm::cont::testing::Testing::Run(TestTemporalAdvection, argc, argv);
}
