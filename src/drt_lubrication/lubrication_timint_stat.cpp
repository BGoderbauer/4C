/*--------------------------------------------------------------------------*/
/*!
\file lubrication_timint_stat.cpp

\brief solution algorithm for stationary problems

<pre>
Maintainer: Andy Wirtz
            wirtz@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089-289-15270
</pre>
*/
/*--------------------------------------------------------------------------*/

#include "../drt_scatra/scatra_timint_meshtying_strategy_base.H"

#include "../drt_io/io.H"

#include "../drt_meshfree_discret/drt_meshfree_discret.H"

#include "../drt_scatra_ele/scatra_ele_action.H"

#include <Teuchos_TimeMonitor.hpp>

#include "lubrication_timint_stat.H"

/*----------------------------------------------------------------------*
 |  Constructor (public)                                      gjb 08/08 |
 *----------------------------------------------------------------------*/
LUBRICATION::TimIntStationary::TimIntStationary(
  Teuchos::RCP<DRT::Discretization>      actdis,
  Teuchos::RCP<LINALG::Solver>           solver,
  Teuchos::RCP<Teuchos::ParameterList>   params,
  Teuchos::RCP<Teuchos::ParameterList>   extraparams,
  Teuchos::RCP<IO::DiscretizationWriter> output)
: TimIntImpl(actdis,solver,params,extraparams,output),
  fsprenp_(Teuchos::null)
{
  // DO NOT DEFINE ANY STATE VECTORS HERE (i.e., vectors based on row or column maps)
  // this is important since we have problems which require an extended ghosting
  // this has to be done before all state vectors are initialized
  return;
}


/*----------------------------------------------------------------------*
 |  initialize time integration                         rasthofer 09/13 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::Init()
{
  // initialize base class
  TimIntImpl::Init();

  // -------------------------------------------------------------------
  // get a vector layout from the discretization to construct matching
  // vectors and matrices
  //                 local <-> global dof numbering
  // -------------------------------------------------------------------
  const Epetra_Map* dofrowmap = discret_->DofRowMap();

  // fine-scale vector
  if (fssgd_ != INPAR::SCATRA::fssugrdiff_no)
    fsprenp_ = LINALG::CreateVector(*dofrowmap,true);
  if (turbmodel_ != INPAR::FLUID::no_model)
    dserror("Turbulence is not stationary problem!");

  // -------------------------------------------------------------------
  // set element parameters
  // -------------------------------------------------------------------
  // note: - this has to be done before element routines are called
  //       - order is important here: for safety checks in SetElementGeneralParameters(),
  //         we have to know the time-integration parameters
  SetElementTimeParameter();
  SetElementGeneralParameters();
  SetElementTurbulenceParameters();

  // setup krylov
  PrepareKrylovProjection();

  // safety check
  if (DRT::INPUT::IntegralValue<int>(*params_,"NATURAL_CONVECTION") == true)
    dserror("Natural convection for stationary time integration scheme is not implemented!");

  return;
}


/*----------------------------------------------------------------------*
| Destructor dtor (public)                                   gjb 08/08 |
*----------------------------------------------------------------------*/
LUBRICATION::TimIntStationary::~TimIntStationary()
{
  return;
}


/*----------------------------------------------------------------------*
 | set time parameter for element evaluation (usual call)    ehrl 11/13 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::SetElementTimeParameter(bool forcedincrementalsolver) const
{
  Teuchos::ParameterList eleparams;

  eleparams.set<int>("action",SCATRA::set_time_parameter);
  eleparams.set<bool>("using generalized-alpha time integration",false);
  eleparams.set<bool>("using stationary formulation",true);
  if(forcedincrementalsolver==false)
    eleparams.set<bool>("incremental solver",incremental_);
  else
    eleparams.set<bool>("incremental solver",true);

  eleparams.set<double>("time-step length",dta_);
  eleparams.set<double>("total time",time_);
  eleparams.set<double>("time factor",1.0);
  eleparams.set<double>("alpha_F",1.0);

  // call standard loop over elements
  discret_->Evaluate(eleparams,Teuchos::null,Teuchos::null,Teuchos::null,Teuchos::null,Teuchos::null);

  return;
}


/*----------------------------------------------------------------------*
 | set time for evaluation of Neumann boundary conditions      vg 12/08 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::SetTimeForNeumannEvaluation(
  Teuchos::ParameterList& params)
{
  params.set("total time",time_);
  return;
}


/*----------------------------------------------------------------------*
 | set part of the residual vector belonging to the old timestep        |
 |                                                            gjb 08/08 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::SetOldPartOfRighthandside()
{
  hist_->PutScalar(0.0);

  return;
}


/*----------------------------------------------------------------------*
 | add actual Neumann loads                                    vg 11/08 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::AddNeumannToResidual()
{
  residual_->Update(1.0,*neumann_loads_,1.0);
  return;
}


/*----------------------------------------------------------------------*
 | AVM3-based scale separation                                 vg 03/09 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::AVM3Separation()
{
  // time measurement: avm3
  TEUCHOS_FUNC_TIME_MONITOR("LUBRICATION:            + avm3");

  // AVM3 separation
  Sep_->Multiply(false,*prenp_,*fsprenp_);

  // set fine-scale vector
  discret_->SetState("fsprenp",fsprenp_);

  return;
}


/*--------------------------------------------------------------------------*
 | add global state vectors specific for time-integration scheme   vg 11/08 |
 *--------------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::AddTimeIntegrationSpecificVectors(bool forcedincrementalsolver)
{
  discret_->SetState("hist",hist_);
  discret_->SetState("prenp",prenp_);

  return;
}


/*----------------------------------------------------------------------*
 |                                                            gjb 09/08 |
 -----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::ReadRestart(int step)
{
  IO::DiscretizationReader reader(discret_,step);
  time_ = reader.ReadDouble("time");
  step_ = reader.ReadInt("step");

  if (myrank_==0)
    std::cout<<"Reading Lubrication restart data (time="<<time_<<" ; step="<<step_<<")"<<std::endl;

  // read state vectors that are needed for restart
  reader.ReadVector(prenp_, "prenp");

  // for elch problems with moving boundary
  // if(isale_)
  //  reader.ReadVector(trueresidual_, "trueresidual");

  return;
}


/*----------------------------------------------------------------------*
 | update of solution at end of time step                     gjb 12/10 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::Update(const int num)
{
  // for the stationary scheme there is nothing to do except this:

  // compute flux vector field for later output BEFORE time shift of results
  // is performed below !!
  if (writeflux_!=INPAR::SCATRA::flux_no)
  {
    if (DoOutput() or DoBoundaryFluxStatistics())
      flux_ = CalcFlux(true, num);
  }

  // compute values at nodes from nodal values for non-interpolatory basis functions
  if (preatmeshfreenodes_!=Teuchos::null)
    Teuchos::rcp_dynamic_cast<DRT::MESHFREE::MeshfreeDiscretization>(discret_)->ComputeValuesAtNodes(prenp_, preatmeshfreenodes_);

  return;
};

/*----------------------------------------------------------------------*
 | write additional data required for restart                 gjb 10/09 |
 *----------------------------------------------------------------------*/
void LUBRICATION::TimIntStationary::OutputRestart()
{
  // This feature enables starting a time-dependent simulation from
  // a non-trivial steady-state solution that was calculated before.
  output_->WriteVector("pren", prenp_);  // for OST and BDF2
  output_->WriteVector("prenm", prenp_); // for BDF2
  output_->WriteVector("predtn", zeros_); // for OST

  // for elch problems with moving boundary
  //if (isale_)
  //  output_->WriteVector("trueresidual", trueresidual_);

  return;
}