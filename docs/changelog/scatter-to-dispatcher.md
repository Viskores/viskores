# Scatter class moved to dispatcher

Scatter classes are special objects that are associated with a worklet to
adjust the standard 1:1 mapping of input to output in the worklet execution
to some other mapping with multiple outputs to a single input or skipping
over input values. A classic use case is the Marching Cubes algorithm where
cube cases will have different numbers of output. A scatter object allows
you to specify for each output polygon which source cube it comes from.

Scatter objects have been in VTK-m for some time now (since before the 1.0
release). The way they used to work is that the worklet completely managed
the scatter object. It would declare the `ScatterType`, keep a copy as part
of its state, and provide a `GetScatter` method so that the dispatcher
could use it for scheduling.

The problem with this approach is that it put control-environment-specific
state into the worklet. The scatter object would be pushed into the
execution environment (like a CUDA device) like the rest of the worklet
where it could not be used. It also meant that worklets that defined their
own scatter had to declare a bunch more code to manage the scatter.

This behavior has been changed so that the dispatcher object manages the
scatter object. The worklet still declares the type of scatter by declaring
a `ScatterType` (defaulting to `ScatterUniform` for 1:1 mapping),
but its responsibility ends there. When the dispatcher is constructed, it
must be given a scatter object that matches the `ScatterType` of the
associated worklet. (If `ScatterType` has a default constructor, then one
can be created automatically.) A worklet may declare a static `MakeScatter`
method for convenience, but this is not necessary.

As an example, a worklet may declare a custom scatter like this.

``` cpp
  class Generate : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<Vec3> inPoints,
                                  FieldOut<Vec3> outPoints);
    typedef void ExecutionSignature(_1, _2);
    using InputDomain = _1;

    using ScatterType = vtkm::worklet::ScatterCounting;

    template<typename CountArrayType, typename DeviceAdapterTag>
    VTKM_CONT
    static ScatterType MakeScatter(const CountArrayType &countArray,
                                   DeviceAdapterTag)
    {
      VTKM_IS_ARRAY_HANDLE(CountArrayType);
      return ScatterType(countArray, DeviceAdapterTag());
    }
```

Note that the `ScatterCounting` needs to be created with the appropriate
indexing arrays to make the scatter behave as the worklet expects, so the
worklet provides a helpful `MakeScatter` method to make it more clear how
to construct the scatter.

This worklet can be invoked as follows.

``` cpp
    auto generateScatter =
        ClipPoints::Generate::MakeScatter(countArray, DeviceAdapterTag());
    vtkm::worklet::DispatcherMapField<ClipPoints::Generate, DeviceAdapterTag>
        dispatcherGenerate(generateScatter);
    dispatcherGenerate.Invoke(pointArray, clippedPointsArray);
```

Because the `ScatterCounting` class does not have a default constructor,
you would get a compiler error if you failed to provide one to the
dispatcher's constructor. The compiler error will probably not be too
helpful the the user, but there is a detailed comment in the dispatcher's
code where the compiler error will occur describing what the issue is.
