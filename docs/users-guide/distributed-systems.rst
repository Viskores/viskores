===================
Distributed Systems
===================

.. index::
   single: distributed

------------
Introduction
------------

As an HPC visualization toolkit, |Viskores| is designed to be executed in distributed systems.
This is a key requirement for resource-expensive applications where a computational job can be partitioned into different tasks, which are then assigned to different processes located on potentially different nodes.
Those nodes, as a composite, will normally form a cluster of computers or a supercomputer.
These computing tasks can communicate and coordinate with each other through the use of a *middleware* library, which in the case of |Viskores| is the DIY library.
Furthermore, both to launch jobs and ultimately to communicate between tasks, |Viskores| delegates to MPI (Message Passing
Interface).

Despite the fact that only some |Viskores| filters can run out of the box as a distributed job, many other |Viskores| components can run as a distributed job by manually partitioning the computational problem data and assigning one of those partitions to each task.

---
DIY
---

.. index::
   single: DIY

DIY is a block-parallel library for implementing scalable algorithms that can execute both *in-core* and *out-of-core*.
The same program can be executed with one or more threads per MPI process, seamlessly combining distributed-memory message passing with shared-memory thread parallelism.
The abstraction enabling these capabilities is block parallelism: blocks and their message queues are mapped onto processing elements, consisting of either MPI processes or threads, and are migrated between memory and storage by the DIY runtime.
Complex communication patterns, including neighbor exchange, merge reduction, swap reduction, and *all-to-all* exchange, are possible in DIY both *in-core* and *out-of-core*.

A full description of using DIY to perform distributed visualization is beyond the scope of this guide.
For a full description, refer to the documentation provided by DIY.
The basic procedure of any DIY algorithm is to first define a set of blocks, assign them to the ranks of an MPI job, and define neighborhood relationships between them.
The following example demonstrates defining a set
of blocks, one per MPI rank.

.. load-example:: DIYSetupComm
   :file: GuideExampleDIY.cxx
   :caption: Communication setup of an example DIY application.

.. didyouknow::
   When using DIY objects from inside |Viskores|, use the objects in the mangled
   ``viskoresdiy`` namespace rather than the ``diy`` namespace. |Viskores| uses
   this mangled namespace to prevent conflicts if it is used with another
   library or executable that uses a different version of DIY.

Any distributed algorithm needs to establish a "communicator" that defines the group of processes that are communicating.
Viskores uses the :class:`viskores::cont::EnvironmentTracker` class to manage a global communicator used within Viskores.
:exlineref:`ex:DIYSetupComm:SetCommunicator` demonstrates establishing the communicator.

.. doxygenclass:: viskores::cont::EnvironmentTracker
   :members:

Communication in DIY is managed by the ``viskoresdiy::Master`` object.
References to the defined blocks are added to the ``viskoresdiy::Master`` object.
You can then run an operation on each of these blocks using the ``foreach`` method, which is given a function to execute on each block.
This function is provided with a proxy that enables communicating data with other nodes using a variety of communication patterns.
These communications do not happen right away but rather are queued for later exchange.
This exchange is done by calling the free function :func:`viskores::cont::DIYMasterExchange`.
The following example, which builds on the previous one, finds the median value of an array on each rank and then finds the maximum median value.
The blocks and communication used by these examples are outlined in :numref:`fig:DIYApp`.

.. doxygenfunction:: viskores::cont::DIYMasterExchange

.. load-example:: DIYForeach
   :file: GuideExampleDIY.cxx
   :caption: Example DIY application that finds the maximum of the medians of different ``ArrayHandle`` objects.

.. figure:: ../../data/users-guide/images/DIYApp.png
   :width: 100%
   :name: fig:DIYApp

   Communication topology of the example DIY application shown in :numref:`ex:DIYSetupComm` and :numref:`ex:DIYForeach`.

.. commonerrors::
   Normally, the DIY exchange process is done by calling the ``viskoresdiy::Master::exchange`` method.
   However, when using DIY with |Viskores|, the exchange should instead be done by calling :func:`viskores::cont::DIYMasterExchange`.
   This function allows |Viskores| to enable the state needed to interface between DIY and |Viskores| data without
   otherwise affecting exchanges that happen outside of |Viskores|.

--------------------
Object Serialization
--------------------

When data are transferred among ranks, the format needs to be packed in a way the message layer understands.
Objects that might have complex structure typically need to be converted to one or more buffers through serialization.

DIY provides out-of-the-box serialization of common C++ standard-library types such as ``std::vector`` and ``std::string``.
|Viskores| also provides serialization for common |Viskores| data types such as :class:`viskores::cont::ArrayHandle` and :class:`viskores::cont::DataSet`.
This list is not exhaustive, since |Viskores| also provides DIY serialization for many other data types.
For custom data types, the user can specify how to serialize and deserialize the desired type by defining an additional template specialization for ``struct viskoresdiy::Serialization``.
An example of this
can be found in :numref:`ex:DIYSerialization`.

.. load-example:: DIYSerialization
   :file: GuideExampleDIYSerialization.cxx
   :caption: Example DIY application that demonstrates how to serialize custom data types in DIY.

-------------
GPU-aware MPI
-------------

Modern HPC GPUs allow direct *GPU-to-GPU* communication.
This provides GPUs with an efficient mechanism to directly send data stored in their device memory to the target GPU's device memory.
This is a significant departure from the traditional approach, where the NIC is solely accessible from the CPU, constraining applications to a costly GPU communication pattern consisting of first copying the desired data from device memory to host memory, transferring it over the network, and then copying the received data from host memory to device memory.

Both major GPU parallel platforms, ROCm and CUDA, provide APIs that support direct *GPU-to-GPU* communication.
Nevertheless, to avoid vendor lock-in, |Viskores| does not directly use these APIs.
Instead, |Viskores| delegates to MPI, which implements a unified and standardized API for *GPU-to-GPU* communication.
Consequently, |Viskores| provides users with the capability to use direct *GPU-to-GPU* communication during MPI-based distributed executions of |Viskores| applications.

|Viskores| can autonomously determine whether *GPU-to-GPU* communication is possible.
Consequently, it does not provide a specific API to control this type of communication.
The communication type is selected on each call to the free function :func:`viskores::cont::DIYMasterExchange`, demonstrated in :numref:`ex:DIYForeach`.
The function internally decorates ``viskoresdiy::Master::exchange`` so that it can perform *GPU-to-GPU* communication when the situation allows it.
Currently, this is only possible with AMD GPUs that support this feature, such as the AMD MI250X used by both OLCF Crusher and OLCF Frontier.

This *GPU-aware* MPI feature can be enabled with the flag ``VISKORES_ENABLE_GPU_MPI=ON``.
Enabling this feature on target supercomputers often requires additional setup that depends on the particular system.
Refer to the target system's documentation for further information.
