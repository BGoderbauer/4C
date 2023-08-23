/*---------------------------------------------------------------------------*/
/*! \file
\brief rigid particle contact handler for smoothed particle hydrodynamics (SPH) interactions
\level 3
*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 | headers                                                                   |
 *---------------------------------------------------------------------------*/
#include "baci_particle_interaction_sph_rigid_particle_contact.H"

#include "baci_discretization_fem_general_utils_fem_shapefunctions.H"
#include "baci_io_visualization_manager.H"
#include "baci_lib_element.H"
#include "baci_lib_utils.H"
#include "baci_particle_engine_container.H"
#include "baci_particle_engine_interface.H"
#include "baci_particle_interaction_runtime_writer.H"
#include "baci_particle_interaction_sph_neighbor_pairs.H"
#include "baci_particle_interaction_utils.H"
#include "baci_particle_wall_datastate.H"
#include "baci_particle_wall_interface.H"
#include "baci_utils_exceptions.H"

#include <Teuchos_TimeMonitor.hpp>

/*---------------------------------------------------------------------------*
 | definitions                                                               |
 *---------------------------------------------------------------------------*/
PARTICLEINTERACTION::SPHRigidParticleContactBase::SPHRigidParticleContactBase(
    const Teuchos::ParameterList& params)
    : params_sph_(params),
      writeparticlewallinteraction_(
          DRT::INPUT::IntegralValue<int>(params_sph_, "WRITE_PARTICLE_WALL_INTERACTION"))
{
  // empty constructor
}

void PARTICLEINTERACTION::SPHRigidParticleContactBase::Init()
{
  // init with potential boundary particle types
  boundarytypes_ = {PARTICLEENGINE::BoundaryPhase, PARTICLEENGINE::RigidPhase};
}

void PARTICLEINTERACTION::SPHRigidParticleContactBase::Setup(
    const std::shared_ptr<PARTICLEENGINE::ParticleEngineInterface> particleengineinterface,
    const std::shared_ptr<PARTICLEWALL::WallHandlerInterface> particlewallinterface,
    const std::shared_ptr<PARTICLEINTERACTION::InteractionWriter> particleinteractionwriter,
    const std::shared_ptr<PARTICLEINTERACTION::SPHNeighborPairs> neighborpairs)
{
  // set interface to particle engine
  particleengineinterface_ = particleengineinterface;

  // set particle container bundle
  particlecontainerbundle_ = particleengineinterface_->GetParticleContainerBundle();

  // set interface to particle wall hander
  particlewallinterface_ = particlewallinterface;

  // set particle interaction writer
  particleinteractionwriter_ = particleinteractionwriter;

  // setup particle interaction writer
  SetupParticleInteractionWriter();

  // set neighbor pair handler
  neighborpairs_ = neighborpairs;

  // update with actual boundary particle types
  const auto boundarytypes = boundarytypes_;
  for (const auto& type_i : boundarytypes)
    if (not particlecontainerbundle_->GetParticleTypes().count(type_i))
      boundarytypes_.erase(type_i);

  // safety check
  if (not boundarytypes_.count(PARTICLEENGINE::RigidPhase))
    dserror("no rigid particles defined but a rigid particle contact formulation is set!");
}

void PARTICLEINTERACTION::SPHRigidParticleContactBase::SetupParticleInteractionWriter()
{
  // register specific runtime vtp writer
  if (writeparticlewallinteraction_)
    particleinteractionwriter_->RegisterSpecificRuntimeVtuWriter("rigidparticle-wall-contact");
}

PARTICLEINTERACTION::SPHRigidParticleContactElastic::SPHRigidParticleContactElastic(
    const Teuchos::ParameterList& params)
    : PARTICLEINTERACTION::SPHRigidParticleContactBase(params),
      stiff_(params_sph_.get<double>("RIGIDPARTICLECONTACTSTIFF")),
      damp_(params_sph_.get<double>("RIGIDPARTICLECONTACTDAMP"))
{
  // empty constructor
}

void PARTICLEINTERACTION::SPHRigidParticleContactElastic::Setup(
    const std::shared_ptr<PARTICLEENGINE::ParticleEngineInterface> particleengineinterface,
    const std::shared_ptr<PARTICLEWALL::WallHandlerInterface> particlewallinterface,
    const std::shared_ptr<PARTICLEINTERACTION::InteractionWriter> particleinteractionwriter,
    const std::shared_ptr<PARTICLEINTERACTION::SPHNeighborPairs> neighborpairs)
{
  // call base class setup
  SPHRigidParticleContactBase::Setup(
      particleengineinterface, particlewallinterface, particleinteractionwriter, neighborpairs);

  // safety check
  if (not(stiff_ > 0.0)) dserror("rigid particle contact stiffness not positive!");
  if (damp_ < 0.0) dserror("rigid particle contact damping parameter not positive or zero!");
}

void PARTICLEINTERACTION::SPHRigidParticleContactElastic::AddForceContribution()
{
  TEUCHOS_FUNC_TIME_MONITOR(
      "PARTICLEINTERACTION::SPHRigidParticleContactElastic::AddForceContribution");

  // elastic contact (particle contribution)
  ElasticContactParticleContribution();

  // elastic contact (particle-wall contribution)
  if (particlewallinterface_) ElasticContactParticleWallContribution();
}

void PARTICLEINTERACTION::SPHRigidParticleContactElastic::ElasticContactParticleContribution()
{
  TEUCHOS_FUNC_TIME_MONITOR(
      "PARTICLEINTERACTION::SPHRigidParticleContactElastic::ElasticContactParticleContribution");

  // get initial particle spacing
  const double initialparticlespacing = params_sph_.get<double>("INITIALPARTICLESPACING");

  // get relevant particle pair indices
  std::vector<int> relindices;
  neighborpairs_->GetRelevantParticlePairIndicesForEqualCombination(boundarytypes_, relindices);

  // iterate over relevant particle pairs
  for (const int particlepairindex : relindices)
  {
    const SPHParticlePair& particlepair =
        neighborpairs_->GetRefToParticlePairData()[particlepairindex];

    // evaluate contact condition
    if (not(particlepair.absdist_ < initialparticlespacing)) continue;

    // access values of local index tuples of particle i and j
    PARTICLEENGINE::TypeEnum type_i;
    PARTICLEENGINE::StatusEnum status_i;
    int particle_i;
    std::tie(type_i, status_i, particle_i) = particlepair.tuple_i_;

    PARTICLEENGINE::TypeEnum type_j;
    PARTICLEENGINE::StatusEnum status_j;
    int particle_j;
    std::tie(type_j, status_j, particle_j) = particlepair.tuple_j_;

    // get corresponding particle containers
    PARTICLEENGINE::ParticleContainer* container_i =
        particlecontainerbundle_->GetSpecificContainer(type_i, status_i);

    PARTICLEENGINE::ParticleContainer* container_j =
        particlecontainerbundle_->GetSpecificContainer(type_j, status_j);

    // get pointer to particle states
    const double* vel_i = container_i->GetPtrToState(PARTICLEENGINE::Velocity, particle_i);
    double* force_i = container_i->CondGetPtrToState(PARTICLEENGINE::Force, particle_i);

    // get pointer to particle states
    const double* vel_j = container_j->GetPtrToState(PARTICLEENGINE::Velocity, particle_j);

    double* force_j = nullptr;
    if (status_j == PARTICLEENGINE::Owned)
      force_j = container_j->CondGetPtrToState(PARTICLEENGINE::Force, particle_j);

    // compute normal gap and rate of normal gap
    const double gap = particlepair.absdist_ - initialparticlespacing;
    const double gapdot =
        UTILS::VecDot(vel_i, particlepair.e_ij_) - UTILS::VecDot(vel_j, particlepair.e_ij_);

    // magnitude of rigid particle contact force
    const double fac = std::min(0.0, (stiff_ * gap + damp_ * gapdot));

    // add contributions
    if (force_i) UTILS::VecAddScale(force_i, -fac, particlepair.e_ij_);
    if (force_j) UTILS::VecAddScale(force_j, fac, particlepair.e_ij_);
  }
}

void PARTICLEINTERACTION::SPHRigidParticleContactElastic::ElasticContactParticleWallContribution()
{
  TEUCHOS_FUNC_TIME_MONITOR(
      "PARTICLEINTERACTION::SPHRigidParticleContactElastic::"
      "ElasticContactParticleWallContribution");

  // get initial particle spacing
  const double initialparticlespacing = params_sph_.get<double>("INITIALPARTICLESPACING");

  // get wall data state container
  std::shared_ptr<PARTICLEWALL::WallDataState> walldatastate =
      particlewallinterface_->GetWallDataState();

  // get reference to particle-wall pair data
  const SPHParticleWallPairData& particlewallpairdata =
      neighborpairs_->GetRefToParticleWallPairData();

  // get number of particle-wall pairs
  const int numparticlewallpairs = particlewallpairdata.size();

  // write interaction output
  const bool writeinteractionoutput =
      particleinteractionwriter_->GetCurrentWriteResultFlag() and writeparticlewallinteraction_;

  // init storage for interaction output
  std::vector<double> attackpoints;
  std::vector<double> contactforces;
  std::vector<double> normaldirection;

  // prepare storage for interaction output
  if (writeinteractionoutput)
  {
    attackpoints.reserve(3 * numparticlewallpairs);
    contactforces.reserve(3 * numparticlewallpairs);
    normaldirection.reserve(3 * numparticlewallpairs);
  }

  // get relevant particle wall pair indices for specific particle types
  std::vector<int> relindices;
  neighborpairs_->GetRelevantParticleWallPairIndices({PARTICLEENGINE::RigidPhase}, relindices);

  // iterate over relevant particle-wall pairs
  for (const int particlewallpairindex : relindices)
  {
    const SPHParticleWallPair& particlewallpair = particlewallpairdata[particlewallpairindex];

    // evaluate contact condition
    if (not(particlewallpair.absdist_ < 0.5 * initialparticlespacing)) continue;

    // access values of local index tuple of particle i
    PARTICLEENGINE::TypeEnum type_i;
    PARTICLEENGINE::StatusEnum status_i;
    int particle_i;
    std::tie(type_i, status_i, particle_i) = particlewallpair.tuple_i_;

    // get corresponding particle container
    PARTICLEENGINE::ParticleContainer* container_i =
        particlecontainerbundle_->GetSpecificContainer(type_i, status_i);

    // get pointer to particle states
    const double* pos_i = container_i->GetPtrToState(PARTICLEENGINE::Position, particle_i);
    const double* vel_i = container_i->GetPtrToState(PARTICLEENGINE::Velocity, particle_i);
    double* force_i = container_i->CondGetPtrToState(PARTICLEENGINE::Force, particle_i);

    // get pointer to column wall element
    DRT::Element* ele = particlewallpair.ele_;

    // number of nodes of wall element
    const int numnodes = ele->NumNode();

    // shape functions and location vector of wall element
    CORE::LINALG::SerialDenseVector funct(numnodes);
    std::vector<int> lmele;

    if (walldatastate->GetVelCol() != Teuchos::null or
        walldatastate->GetForceCol() != Teuchos::null)
    {
      // evaluate shape functions of element at wall contact point
      CORE::DRT::UTILS::shape_function_2D(
          funct, particlewallpair.elecoords_[0], particlewallpair.elecoords_[1], ele->Shape());

      // get location vector of wall element
      lmele.reserve(numnodes * 3);
      std::vector<int> lmowner;
      std::vector<int> lmstride;
      ele->LocationVector(
          *particlewallinterface_->GetWallDiscretization(), lmele, lmowner, lmstride);
    }

    // velocity of wall contact point j
    double vel_j[3] = {0.0, 0.0, 0.0};

    if (walldatastate->GetVelCol() != Teuchos::null)
    {
      // get nodal velocities
      std::vector<double> nodal_vel(numnodes * 3);
      DRT::UTILS::ExtractMyValues(*walldatastate->GetVelCol(), nodal_vel, lmele);

      // determine velocity of wall contact point j
      for (int node = 0; node < numnodes; ++node)
        for (int dim = 0; dim < 3; ++dim) vel_j[dim] += funct[node] * nodal_vel[node * 3 + dim];
    }

    // compute normal gap and rate of normal gap
    const double gap = particlewallpair.absdist_ - 0.5 * initialparticlespacing;
    const double gapdot =
        UTILS::VecDot(vel_i, particlewallpair.e_ij_) - UTILS::VecDot(vel_j, particlewallpair.e_ij_);

    // magnitude of rigid particle contact force
    const double fac = std::min(0.0, (stiff_ * gap + damp_ * gapdot));

    // add contributions
    if (force_i) UTILS::VecAddScale(force_i, -fac, particlewallpair.e_ij_);

    // calculation of wall contact force
    double wallcontactforce[3] = {0.0, 0.0, 0.0};
    if (writeinteractionoutput or walldatastate->GetForceCol() != Teuchos::null)
      UTILS::VecSetScale(wallcontactforce, fac, particlewallpair.e_ij_);

    // write interaction output
    if (writeinteractionoutput)
    {
      // compute vector from wall contact point j to particle i
      double r_ij[3];
      UTILS::VecSetScale(r_ij, particlewallpair.absdist_, particlewallpair.e_ij_);

      // calculate wall contact point
      double wallcontactpoint[3];
      UTILS::VecSet(wallcontactpoint, pos_i);
      UTILS::VecSub(wallcontactpoint, r_ij);

      // set wall attack point and states
      for (int dim = 0; dim < 3; ++dim) attackpoints.push_back(wallcontactpoint[dim]);
      for (int dim = 0; dim < 3; ++dim) contactforces.push_back(wallcontactforce[dim]);
      for (int dim = 0; dim < 3; ++dim) normaldirection.push_back(particlewallpair.e_ij_[dim]);
    }

    // assemble contact force acting on wall element
    if (walldatastate->GetForceCol() != Teuchos::null)
    {
      // determine nodal forces
      std::vector<double> nodal_force(numnodes * 3);
      for (int node = 0; node < numnodes; ++node)
        for (int dim = 0; dim < 3; ++dim)
          nodal_force[node * 3 + dim] = funct[node] * wallcontactforce[dim];

      // assemble nodal forces
      const int err = walldatastate->GetForceCol()->SumIntoGlobalValues(
          numnodes * 3, nodal_force.data(), lmele.data());
      if (err < 0) dserror("sum into Epetra_Vector failed!");
    }
  }

  if (writeinteractionoutput)
  {
    // get specific runtime vtp writer
    IO::VisualizationManager* visualization_manager =
        particleinteractionwriter_->GetSpecificRuntimeVtuWriter("rigidparticle-wall-contact");
    auto& visualization_data = visualization_manager->GetVisualizationData();

    // set wall attack points
    visualization_data.GetPointCoordinates() = attackpoints;

    // append states
    visualization_data.SetPointDataVector<double>("contact force", contactforces, 3);
    visualization_data.SetPointDataVector<double>("normal direction", normaldirection, 3);
  }
}