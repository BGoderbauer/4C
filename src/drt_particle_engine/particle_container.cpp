/*---------------------------------------------------------------------------*/
/*!
\file particle_container.cpp

\brief smart particle container class

\level 3

\maintainer  Sebastian Fuchs
             fuchs@lnm.mw.tum.de
             http://www.lnm.mw.tum.de
             089 - 289 -15262

*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | headers                                                    sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
#include "particle_container.H"

#include "../drt_lib/drt_dserror.H"

#include <iostream>

/*---------------------------------------------------------------------------*
 | constructor                                                sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
PARTICLEENGINE::ParticleContainer::ParticleContainer() : containersize_(0), particlestored_(0)
{
  // empty constructor
}

/*---------------------------------------------------------------------------*
 | init particle container                                    sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::Init()
{
  // nothing to do
}

/*---------------------------------------------------------------------------*
 | setup particle container                                   sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::Setup(
    int numContainerSize, const std::set<StateEnum>& stateEnumSet)
{
  // set size of particle container (at least one)
  containersize_ = (numContainerSize > 0) ? numContainerSize : 1;

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;

  // iterate over set of given particle state enums
  for (auto& stateEnum : stateEnumSet)
  {
    // allocate memory for current state in particle container
    stateDim = EnumToStateDim(stateEnum);
    state = std::make_shared<std::vector<double>>(containersize_ * stateDim);
    states_.insert(std::make_pair(stateEnum, state));
  }
}

/*---------------------------------------------------------------------------*
 | increase the container size                                sfuchs 04/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::IncreaseContainerSize()
{
  // size of container is doubled
  containersize_ *= 2;

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    state->resize(containersize_ * stateDim);
  }
}

/*---------------------------------------------------------------------------*
 | decrease the container size                                sfuchs 04/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::DecreaseContainerSize()
{
  // size of container is halved
  int newsize = static_cast<int>(0.5 * containersize_);

  // set size of particle container (at least one)
  containersize_ = (newsize > 0) ? newsize : 1;

  // safety check
  if (particlestored_ > containersize_)
    dserror(
        "decreasing size of container not possible: particles stored %d > new container size %d!",
        particlestored_, containersize_);

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    state->resize(containersize_ * stateDim);
  }
}

/*---------------------------------------------------------------------------*
 | clear particle container                                   sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::ClearContainer() { particlestored_ = 0; }

/*---------------------------------------------------------------------------*
 | add particle to particle container and get index           sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
int PARTICLEENGINE::ParticleContainer::AddParticle(const ParticleStates& particle)
{
  // increase size of container
  if (particlestored_ == containersize_) IncreaseContainerSize();

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    auto particleStateIt = particle.find(it.first);
    // state not handed over
    if (particleStateIt == particle.end())
    {
      // initialize to zero
      for (int dim = 0; dim < stateDim; ++dim) (*state)[particlestored_ * stateDim + dim] = 0.0;
    }
    // state handed over
    else
    {
      const std::vector<double>& particleState = particleStateIt->second;

      // check dimensions
      if (static_cast<int>(particleState.size()) != stateDim)
        dserror("Cannot add particle: dimensions of state '%s' do not match!",
            PARTICLEENGINE::EnumToStateName(it.first).c_str());

      // store state in container
      for (int dim = 0; dim < stateDim; ++dim)
        (*state)[particlestored_ * stateDim + dim] = particleState[dim];
    }
  }

  // return index of added particle and increase counter of stored particles
  return particlestored_++;
}

/*---------------------------------------------------------------------------*
 | replace particle in particle container at given index      sfuchs 05/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::ReplaceParticle(
    int index, const ParticleStates& particle) const
{
  if (index < 0 or index > (particlestored_ - 1))
    dserror("can not replace particle as index out of bounds!");

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    auto particleStateIt = particle.find(it.first);
    // state not handed over
    if (particleStateIt == particle.end())
    {
      // leave state untouched
    }
    // state handed over
    else
    {
      const std::vector<double>& particleState = particleStateIt->second;

      // check dimensions
      if (static_cast<int>(particleState.size()) != stateDim)
        dserror("Cannot replace particle: dimensions of state '%s' do not match!",
            PARTICLEENGINE::EnumToStateName(it.first).c_str());

      // replace state in container
      for (int dim = 0; dim < stateDim; ++dim)
        (*state)[index * stateDim + dim] = particleState[dim];
    }
  }
}

/*---------------------------------------------------------------------------*
 | get particle at index from particle container              sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
PARTICLEENGINE::ParticleStates PARTICLEENGINE::ParticleContainer::GetParticle(int index) const
{
  if (index < 0 or index > (particlestored_ - 1))
    dserror("can not return particle as index out of bounds!");

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;
  ParticleStates particle;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    std::vector<double> particleState(stateDim);

    // store current state in vector
    for (int dim = 0; dim < stateDim; ++dim) particleState[dim] = (*state)[index * stateDim + dim];

    particle.insert(std::make_pair(it.first, particleState));
  }

  return particle;
}

/*---------------------------------------------------------------------------*
 | remove particle from particle container                    sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::RemoveParticle(int index)
{
  if (index < 0 or index > (particlestored_ - 1))
    dserror("can not remove particle as index out of bounds!");

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    for (int dim = 0; dim < stateDim; ++dim)
    {
      // avoid fragmentation of memory by swapping the particle to be removed with last particle in
      // container
      std::swap((*state)[index * stateDim + dim], (*state)[(particlestored_ - 1) * stateDim + dim]);
    }
  }

  // decrease counter of stored particles
  --particlestored_;

  // decrease size of container
  if (particlestored_ < 0.45 * containersize_) DecreaseContainerSize();
}

/*---------------------------------------------------------------------------*
 | remove and get particle from particle container            sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
PARTICLEENGINE::ParticleStates PARTICLEENGINE::ParticleContainer::RemoveGetParticle(int index)
{
  if (index < 0 or index > (particlestored_ - 1))
    dserror("can not remove particle as index out of bounds!");

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;
  ParticleStates particle;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    std::vector<double> particleState(stateDim);

    for (int dim = 0; dim < stateDim; ++dim)
    {
      // store current state in vector
      particleState[dim] = (*state)[index * stateDim + dim];

      // avoid fragmentation of memory by swapping the particle to be removed with last particle in
      // container
      std::swap((*state)[index * stateDim + dim], (*state)[(particlestored_ - 1) * stateDim + dim]);
    }

    particle.insert(std::make_pair(it.first, particleState));
  }

  // decrease counter of stored particles
  --particlestored_;

  // decrease size of container
  if (particlestored_ < 0.45 * containersize_) DecreaseContainerSize();

  return particle;
}

/*---------------------------------------------------------------------------*
 | swap particles at index a and b in particle container      sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::SwapParticle(int index_a, int index_b) const
{
  if (index_a < 0 or index_a > (particlestored_ - 1) or index_b < 0 or
      index_b > (particlestored_ - 1))
    dserror("can not swap particles as index out of bounds!");

  int stateDim = 0;
  std::shared_ptr<std::vector<double>> state;

  // iterate over states stored in container
  for (auto& it : states_)
  {
    stateDim = EnumToStateDim(it.first);
    state = it.second;

    for (int dim = 0; dim < stateDim; ++dim)
      std::swap((*state)[index_a * stateDim + dim], (*state)[index_b * stateDim + dim]);
  }
}

/*---------------------------------------------------------------------------*
 | return pointer to state of a particle at index             sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
double* PARTICLEENGINE::ParticleContainer::GetPtrToParticleState(
    StateEnum stateEnum, int index) const
{
  if (index < 0 or index > (particlestored_ - 1))
    dserror("can not return pointer to state of particle as index out of bounds!");

  int stateDim = EnumToStateDim(stateEnum);

  auto it = states_.find(stateEnum);
  if (it == states_.end())
    dserror("particle state '%s' not found!", PARTICLEENGINE::EnumToStateName(stateEnum).c_str());
  auto state = it->second;

  return &((*state)[index * stateDim]);
}

/*---------------------------------------------------------------------------*
 | get state of a particle at index                           sfuchs 05/2018 |
 *---------------------------------------------------------------------------*/
std::vector<double> PARTICLEENGINE::ParticleContainer::GetParticleState(
    StateEnum stateEnum, int index) const
{
  if (index < 0 or index > (particlestored_ - 1))
    dserror("can not return state of particle as index out of bounds!");

  int stateDim = EnumToStateDim(stateEnum);

  auto it = states_.find(stateEnum);
  if (it == states_.end())
    dserror("particle state '%s' not found!", PARTICLEENGINE::EnumToStateName(stateEnum).c_str());
  auto state = it->second;

  std::vector<double> particleState(stateDim);
  for (int dim = 0; dim < stateDim; ++dim) particleState[dim] = (*state)[index * stateDim + dim];

  return particleState;
}

/*---------------------------------------------------------------------------*
 | scale state of particles                                   sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::ScaleState(double fac, StateEnum stateEnum) const
{
  int stateDim = EnumToStateDim(stateEnum);

  auto it = states_.find(stateEnum);
  if (it == states_.end())
    dserror("particle state '%s' not found!", PARTICLEENGINE::EnumToStateName(stateEnum).c_str());
  auto state = it->second;

  for (int i = 0; i < (particlestored_ * stateDim); ++i) (*state)[i] *= fac;
}

/*---------------------------------------------------------------------------*
 | scale and add states to update state of particles          sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::UpdateState(
    double facA, StateEnum stateEnumA, double facB, StateEnum stateEnumB) const
{
  int stateDim = EnumToStateDim(stateEnumA);
  if (stateDim != EnumToStateDim(stateEnumB)) dserror("dimensions of states do not match!");

  auto it = states_.find(stateEnumA);
  if (it == states_.end())
    dserror("particle state '%s' not found!", PARTICLEENGINE::EnumToStateName(stateEnumA).c_str());
  auto stateA = it->second;

  it = states_.find(stateEnumB);
  if (it == states_.end())
    dserror("particle state '%s' not found!", PARTICLEENGINE::EnumToStateName(stateEnumB).c_str());
  auto stateB = it->second;

  for (int i = 0; i < (particlestored_ * stateDim); ++i)
    (*stateA)[i] = facA * (*stateA)[i] + facB * (*stateB)[i];
}

/*---------------------------------------------------------------------------*
 | set state to particles                                     sfuchs 04/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::SetState(std::vector<double> val, StateEnum stateEnum) const
{
  int stateDim = EnumToStateDim(stateEnum);

  if (stateDim != static_cast<int>(val.size())) dserror("dimensions of states do not match!");

  auto it = states_.find(stateEnum);
  if (it == states_.end())
    dserror("particle state '%s' not found!", PARTICLEENGINE::EnumToStateName(stateEnum).c_str());
  auto state = it->second;

  for (int i = 0; i < particlestored_; ++i)
    for (int dim = 0; dim < stateDim; ++dim) (*state)[i * stateDim + dim] = val[dim];
}

/*---------------------------------------------------------------------------*
 | clear state of particles                                   sfuchs 05/2018 |
 *---------------------------------------------------------------------------*/
void PARTICLEENGINE::ParticleContainer::ClearState(StateEnum stateEnum) const
{
  int stateDim = EnumToStateDim(stateEnum);

  auto it = states_.find(stateEnum);
  if (it == states_.end())
    dserror("particle state '%s' not found!", PARTICLEENGINE::EnumToStateName(stateEnum).c_str());
  auto state = it->second;

  for (int i = 0; i < (particlestored_ * stateDim); ++i) (*state)[i] = 0.0;
}

/*---------------------------------------------------------------------------*
 | get stored particle states                                 sfuchs 03/2018 |
 *---------------------------------------------------------------------------*/
const std::set<PARTICLEENGINE::StateEnum> PARTICLEENGINE::ParticleContainer::GetParticleStates()
    const
{
  std::set<StateEnum> particlestates;

  // iterate over states stored in container
  for (auto& it : states_) particlestates.insert(it.first);

  return particlestates;
}