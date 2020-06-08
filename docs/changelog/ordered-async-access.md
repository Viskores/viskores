# Order asynchronous `ArrayHandle` access

The recent feature of [tokens that scope access to
`ArrayHandle`s](scoping-tokens.md) allows multiple threads to use the same
`ArrayHandle`s without read/write hazards. The intent is twofold. First, it
allows two separate threads in the control environment to independently
schedule tasks. Second, it allows us to move toward scheduling worklets and
other algorithms asynchronously.

However, there was a flaw with the original implementation. Once requests
to an `ArrayHandle` get queued up, they are resolved in arbitrary order.
This might mean that things run in surprising and incorrect order.

## Problematic use case

To demonstrate the flaw in the original implementation, let us consider a
future scenario where when you invoke a worklet (on OpenMP or TBB), the
call to invoke returns immediately and the actual work is scheduled
asynchronously. Now let us say we have a sequence of 3 worklets we wish to
run: `Worklet1`, `Worklet2`, and `Worklet3`. One of `Worklet1`'s parameters
is a `FieldOut` that creates an intermediate `ArrayHandle` that we will
simply call `array`. `Worklet2` is given `array` as a `FieldInOut` to
modify its values. Finally, `Worklet3` is given `array` as a `FieldIn`. It
is clear that for the computation to be correct, the three worklets _must_
execute in the correct order of `Worklet1`, `Worklet2`, and `Worklet3`.

The problem is that if `Worklet2` and `Worklet3` are both scheduled before
`Worklet1` finishes, the order they are executed could be arbitrary. Let us
say that `Worklet1` is invoked, and the invoke call returns before the
execution of `Worklet1` finishes.

The calling code immediately invokes `Worklet2`. Because `array` is already
locked by `Worklet1`, `Worklet2` does not execute right away. Instead, it
waits on a condition variable of `array` until it is free. But even though
the scheduling of `Worklet2` is blocked, the invoke returns because we are
scheduling asynchronously.

Likewise, the calling code then immediately calls invoke for `Worklet3`.
`Worklet3` similarly waits on the condition variable of `array` until it is
free.

Let us assume the likely event that both `Worklet2` and `Worklet3` get
scheduled before `Worklet1` finishes. When `Worklet1` then later does
finish, it's token relinquishes the lock on `array`, which wakes up the
threads waiting for access to `array`. However, there is no imposed order on
in what order the waiting threads will acquire the lock and run. (At least,
I'm not aware of anything imposing an order.) Thus, it is quite possible
that `Worklet3` will wake up first. It will see that `array` is no longer
locked (because `Worklet1` has released it and `Worklet2` has not had a
chance to claim it).

Oops. Now `Worklet3` is operating on `array` before `Worklet2` has had a
chance to put the correct values in it. The results will be wrong.

## Queuing requests

What we want is to impose the restriction that locks to an `ArrayHandle`
get resolved in the order that they are requested. In the previous example,
we have 3 requests on an array that happen in a known order. We want
control given to them in the same order.

To implement this, we need to impose another restriction on the
`condition_variable` when waiting to read or write. We want the lock to go
to the thread that first started waiting. To do this, we added an
internal queue of `Token`s to the `ArrayHandle`.

In `ArrayHandle::WaitToRead` and `ArrayHandle::WaitToWrite`, it first adds
its `Token` to the back of the queue before waiting on the condition
variable. In the `CanRead` and `CanWrite` methods, it checks this queue to
see if the provided `Token` is at the front. If not, then the lock is
denied and the thread must continue to wait.

## Early enqueuing

Another issue that can happen in the previous example is that as threads
are spawned for the 3 different worklets, they may actually start running
in an unexpected order. So the thread running `Worklet3` might actually
start before the other 2 and place itself in the queue first.

The solution is to add a method to `ArrayHandle` called `Enqueue`. This
method takes a `Token` object and adds that `Token` to the queue. However,
regardless of where the `Token` ends up on the queue, the method
immediately returns. It does not attempt to lock the `ArrayHandle`.

So now we can ensure that `Worklet1` properly locks `array` with this
sequence of events. First, the main thread calls `array.Enqueue`. Then a
thread is spawned to call `PrepareForOutput`.

Even if control returns to the calling code and it calls invoke for
`Worklet2` before this spawned thread starts, `Worklet2` cannot start
first. When `PrepareForInput` is called on `array`, it is queued after the
`Token` for `Worklet1`, even if `Worklet1` has not started waiting on the
`array`.
