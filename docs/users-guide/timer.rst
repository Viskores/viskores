==============================
Timers
==============================

.. index:: timer

It is often the case that you need to measure the time it takes for an operation to happen.
This could be for performing measurements for algorithm study or it could be to dynamically adjust scheduling.

Performing timing in a multi-threaded environment can be tricky because operations happen asynchronously.
To ensure that accurate timings can be made, |Viskores| provides a :class:`viskores::cont::Timer` class to provide an accurate measurement of operations that happen on devices that |Viskores| can use.
By default, :class:`viskores::cont::Timer` will time operations on all possible devices.

The timer is started by calling the :func:`viskores::cont::Timer::Start` method.
The timer can subsequently be stopped by calling :func:`viskores::cont::Timer::Stop`.
The time elapsed between calls to :func:`viskores::cont::Timer::Start` and :func:`viskores::cont::Timer::Stop` (or the current time if :func:`viskores::cont::Timer::Stop` was not called) can be retrieved with a call to the :func:`viskores::cont::Timer::GetElapsedTime` method.
Subsequently calling :func:`viskores::cont::Timer::Start` again will restart the timer.

.. load-example:: Timer
   :file: GuideExampleTimer.cxx
   :caption: Using :class:`viskores::cont::Timer`.

.. commonerrors::
   Some device require data to be copied between the host CPU and the device.
   In this case you might want to measure the time to copy data back to the host.
   This can be done by "touching" the data on the host by getting a control portal.

The |Viskores| :class:`viskores::cont::Timer` does its best to capture the time it takes for all parallel operations run between calls to :func:`viskores::cont::Timer::Start` and :func:`viskores::cont::Timer::Stop` to complete.
It does so by synchronizing to concurrent execution on devices that might be in use.

.. commonerrors::
   Because :class:`viskores::cont::Timer` synchronizes with devices (essentially waiting for the device to finish executing), that can have an effect on how your program runs.
   Be aware that using a :class:`viskores::cont::Timer` can itself change the performance of your code.
   In particular, starting and stopping the timer many times to measure the parts of a sequence of operations can potentially make the whole operation run slower.

By default, :class:`viskores::cont::Timer` will synchronize with all active devices.
However, if you want to measure the time for a specific device, then you can pass the device adapter tag or id to :class:`viskores::cont::Timer`'s constructor.
You can also change the device being used by passing a device adapter tag or id to the :func:`viskores::cont::Timer::Reset` method.
A device can also be specified through an optional argument to the :func:`viskores::cont::Timer::GetElapsedTime` method.

.. doxygenclass:: viskores::cont::Timer
   :members:
