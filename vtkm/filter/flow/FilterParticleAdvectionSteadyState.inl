#include<vtkm/filter/flow/ParticleAdvection.h>
#include<vtkm/filter/flow/Streamline.h>
#include<vtkm/filter/flow/WarpXStreamline.h>

namespace vtkm
{
namespace filter
{
namespace flow
{

template class FilterParticleAdvectionSteadyState<vtkm::filter::flow::ParticleAdvection>;
template class FilterParticleAdvectionSteadyState<vtkm::filter::flow::Streamline>;
template class FilterParticleAdvectionSteadyState<vtkm::filter::flow::WarpXStreamline>;

} // namespace flow
} // namespace filter
} // namespace vtkm
