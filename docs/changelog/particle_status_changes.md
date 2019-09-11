# Updating particle status for advection

There are now special statuses for Particle, Integrator, and Evaluator.

The particle advection modules only supported statuses for particles and made it
difficult to handle advanced integtator statuses.
Now each of the three important modules return their own statuses

Particles have `vtkm::worklet::particleadvection::ParticleStatus`,
Integrators have `vtkm::worklet::particleadvection::IntegratorStatus`, and
Evaluators have `vtkm::worklet::particleadvection::EvaluatorStatus`.

Further, names of the statuses in `vtkm::worklet::particleadvection::ParticleStatus`
have changed

`ParticleStatus::STATUS_OK` is now `ParticleStatus::SUCCESS`, and there is another
status `ParticleStatus::TOOK_ANY_STEPS` which is active if the particle has taken
at least one step with the current data.

There are few more changes that allow particle advection in 2D structured grids.
