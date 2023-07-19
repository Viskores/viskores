namespace vtkm
{
namespace worklet
{
namespace flow
{

template class NoAnalysis<vtkm::Particle>;
template class NoAnalysis<vtkm::ChargedParticle>;
template class StreamlineAnalysis<vtkm::Particle>;
template class StreamlineAnalysis<vtkm::ChargedParticle>;

} // namespace particleadvection
} // namespace worklet
} // namespace vtkm
