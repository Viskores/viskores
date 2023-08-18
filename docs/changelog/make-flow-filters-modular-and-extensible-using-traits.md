## Make flow filters modular and extensible using traits

Many flow filters have common underpinnings in term of the components they use.
E.g., the choice and handling for solvers, analysis, termination, vector field, etc.
However, having these components baked hard in the infrastructure makes extensibility chanllenging,
which leads to developers implementing bespoke solutions.
This change establishes an infrastructure for easy specification and development of flow filter.

To that end, two new abstractions are introduced along with thier basic implementations : `Analysis` and `Termination`
* `Analysis` defines how each step of the particle needs to be analyzed
* `Termination` defines the termination criteria for every particle

The two, in addition to the existing abstractions for `Particle` and `Field` can be used to specify
novel flow filters. This is accomplished by defining a new trait for the new filter using implementations
for these abstractions.

E.g., for specifying the streamline filter for a general case the following trait can be used

```cpp
template <>
struct FlowTraits<Streamline>
{
  using ParticleType    = vtkm::Particle;
  using TerminationType = vtkm::worklet::flow::NormalTermination;
  using AnalysisType    = vtkm::worklet::flow::StreamlineAnalysis<ParticleType>;
  using ArrayType       = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType       = vtkm::worklet::flow::VelocityField<ArrayType>;
};
```
Similarly, to produce a flow map, the following trait can be used

```cpp
template <>
struct FlowTraits<ParticleAdvection>
{
  using ParticleType = vtkm::Particle;
  using TerminationType = vtkm::worklet::flow::NormalTermination;
  using AnalysisType = vtkm::worklet::flow::NoAnalysis<ParticleType>;
  using ArrayType = vtkm::cont::ArrayHandle<vtkm::Vec3f>;
  using FieldType = vtkm::worklet::flow::VelocityField<ArrayType>;
};
```
These traits are enough for the infrastrucutre to use the correct code paths to produce the desired
result.

Along with changing the existing filters to use this new way of specification of components, 
a new filter `WarpXStreamline` has been added to enable streamline analysis for charged particles for
the WarpX simulation.
