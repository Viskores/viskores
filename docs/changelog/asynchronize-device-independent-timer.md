# Introduce asynchronous and device independent timer

The timer class now is asynchronous and device independent. it's using an
similiar API as vtkOpenGLRenderTimer with Start(), Stop(), Reset(), Ready(),
and GetElapsedTime() function. For convenience and backward compability, Each
Start() function call will call Reset() internally. GetElapsedTime() function
can be used multiple times to time sequential operations and Stop() function
can be helpful when you want to get the elapsed time latter.

Bascially it can be used in two modes:

* Create a Timer without any device info.
  * It would enable the timer for all enabled devices on the machine. Users can get a
specific elapsed time by passing a device id into the GetElapsedTime function.
If no device is provided, it would pick the maximum of all timer results - the
logic behind this decision is that if cuda is disabled, openmp, serial and tbb
roughly give the same results; if cuda is enabled it's safe to return the
maximum elapsed time since users are more interested in the device execution
time rather than the kernal launch time. The Ready function can be handy here
to query the status of the timer.

``` Construct a generic timer
// Assume CUDA is enabled on the machine
vtkm::cont::Timer timer;
timer.Start();
// Run the algorithm

auto timeHost = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagSerial());
// To avoid the expensive device synchronization, we query is ready here.
if (timer.IsReady())
{
  auto timeDevice = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagCuda());
}
// Force the synchronization. Ideally device execution time would be returned
which takes longer time than ther kernal call
auto timeGeneral = timer.GetElapsedTime();
```

* Create a Timer with a specific device.
  * It works as the old timer that times for a specific device id.
``` Construct a device specific timer
// Assume TBB is enabled on the machine
vtkm::cont::Timer timer{vtkm::cont::DeviceAdaptertagTBB()};
timer.Start(); // t0
// Run the algorithm

// Timer would just return 0 and warn the user in the logger that an invalid
// device is used to query elapsed time
auto timeInvalid = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagSerial());
if timer.IsReady()
{
  // Either will work and mark t1, return t1-t0
  auto time1TBB = timer.GetElapsedTime(vtkm::cont::DeviceAdapterTagTBB());
  auto time1General = timer.GetElapsedTime();
}

// Do something
auto time2 = timer.GetElapsedTime(); // t2 will be marked and t2-t0 will be returned

// Do something
timer.Stop() // t3 marked

// Do something then summarize latter
auto timeFinal = timer.GetElapsedTime(); // t3-t0
```
