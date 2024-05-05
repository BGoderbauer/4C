/*----------------------------------------------------------------------*/
/*! \file

\brief scalar transport field base algorithm

\level 1


*/
/*----------------------------------------------------------------------*/

#include "4C_adapter_scatra_base_algorithm.hpp"

#include "4C_global_data.hpp"
#include "4C_inpar_ssi.hpp"
#include "4C_inpar_ssti.hpp"
#include "4C_inpar_sti.hpp"
#include "4C_io.hpp"
#include "4C_io_control.hpp"
#include "4C_levelset_timint_ost.hpp"
#include "4C_levelset_timint_stat.hpp"
#include "4C_linear_solver_method.hpp"
#include "4C_linear_solver_method_linalg.hpp"
#include "4C_scatra_resulttest_hdg.hpp"
#include "4C_scatra_timint_bdf2.hpp"
#include "4C_scatra_timint_cardiac_monodomain_scheme.hpp"
#include "4C_scatra_timint_cardiac_monodomain_scheme_hdg.hpp"
#include "4C_scatra_timint_elch_scheme.hpp"
#include "4C_scatra_timint_genalpha.hpp"
#include "4C_scatra_timint_loma_genalpha.hpp"
#include "4C_scatra_timint_ost.hpp"
#include "4C_scatra_timint_poromulti.hpp"
#include "4C_scatra_timint_stat.hpp"
#include "4C_scatra_timint_stat_hdg.hpp"

#include <Teuchos_StandardParameterEntryValidators.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
ADAPTER::ScaTraBaseAlgorithm::ScaTraBaseAlgorithm(const Teuchos::ParameterList& prbdyn,
    const Teuchos::ParameterList& scatradyn, const Teuchos::ParameterList& solverparams,
    const std::string& disname, const bool isale)
    : scatra_(Teuchos::null), issetup_(false), isinit_(false)
{
  // setup scalar transport algorithm (overriding some dynamic parameters
  // with values specified in given problem-dependent ParameterList prbdyn)

  // -------------------------------------------------------------------
  // what's the current problem type?
  // -------------------------------------------------------------------
  auto probtype = GLOBAL::Problem::Instance()->GetProblemType();

  // -------------------------------------------------------------------
  // access the discretization
  // -------------------------------------------------------------------
  auto discret = GLOBAL::Problem::Instance()->GetDis(disname);

  // -------------------------------------------------------------------
  // set degrees of freedom in the discretization
  // -------------------------------------------------------------------
  if (!discret->Filled() or !discret->HaveDofs()) discret->FillComplete();

  // -------------------------------------------------------------------
  // context for output and restart
  // -------------------------------------------------------------------
  auto output = discret->Writer();
  if (discret->NumGlobalElements() == 0)
    FOUR_C_THROW("No elements in discretization %s", discret->Name().c_str());
  output->WriteMesh(0, 0.0);

  // -------------------------------------------------------------------
  // create a solver
  // -------------------------------------------------------------------
  // TODO: TAW use of solverparams???
  // change input parameter to solver number instead of parameter list?
  // -> no default paramter possible any more
  auto solver = Teuchos::rcp(new CORE::LINALG::Solver(solverparams, discret->Comm()));

  // -------------------------------------------------------------------
  // set parameters in list required for all schemes
  // -------------------------------------------------------------------
  // make a copy (inside an Teuchos::rcp) containing also all sublists
  auto scatratimeparams = Teuchos::rcp(new Teuchos::ParameterList(scatradyn));
  if (scatratimeparams == Teuchos::null)
    FOUR_C_THROW("Instantiation of Teuchos::ParameterList failed!");

  // -------------------------------------------------------------------
  // overrule certain parameters for coupled problems
  // -------------------------------------------------------------------
  // the default time step size
  scatratimeparams->set<double>("TIMESTEP", prbdyn.get<double>("TIMESTEP"));
  // maximum simulation time
  scatratimeparams->set<double>("MAXTIME", prbdyn.get<double>("MAXTIME"));
  // maximum number of timesteps
  scatratimeparams->set<int>("NUMSTEP", prbdyn.get<int>("NUMSTEP"));
  // restart
  scatratimeparams->set<int>("RESTARTEVRY", prbdyn.get<int>("RESTARTEVRY"));
  // solution output
  scatratimeparams->set<int>("RESULTSEVRY", prbdyn.get<int>("RESULTSEVRY"));

  // -------------------------------------------------------------------
  // overrule flags for solid-based scalar transport!
  // (assumed disname = "scatra2" for solid-based scalar transport)
  // -------------------------------------------------------------------
  if (probtype == GLOBAL::ProblemType::ac_fsi or probtype == GLOBAL::ProblemType::biofilm_fsi or
      probtype == GLOBAL::ProblemType::gas_fsi or probtype == GLOBAL::ProblemType::fps3i or
      probtype == GLOBAL::ProblemType::thermo_fsi)
  {
    // scatra1 (=fluid scalar) get's inputs from SCALAR TRANSPORT DYNAMIC/STABILIZATION, hence
    // nothing is to do here
    //    if (disname== "scatra1") //get's inputs from SCALAR TRANSPORT DYNAMIC/STABILIZATION

    if (disname == "scatra2")  // structure_scatra discretisation
    {
      // scatra2 (=structure scalar) get's inputs from FS3I DYNAMIC/STRUCTURE SCALAR STABILIZATION,
      // hence we have to replace it
      scatratimeparams->sublist("STABILIZATION") = prbdyn.sublist("STRUCTURE SCALAR STABILIZATION");
      scatratimeparams->set<std::string>(
          "CONVFORM", prbdyn.get<std::string>("STRUCTSCAL_CONVFORM"));

      // scatra2 get's in initial functions from FS3I DYNAMICS
      switch (CORE::UTILS::IntegralValue<INPAR::SCATRA::InitialField>(
          prbdyn, "STRUCTSCAL_INITIALFIELD"))
      {
        case INPAR::SCATRA::initfield_zero_field:
          scatratimeparams->set<std::string>("INITIALFIELD",
              "zero_field");  // we want zero initial conditions for the structure scalar
          scatratimeparams->set<int>("INITFUNCNO", -1);
          break;
        case INPAR::SCATRA::initfield_field_by_function:
          scatratimeparams->set<std::string>(
              "INITIALFIELD", "field_by_function");  // we want the same initial conditions for
                                                     // structure scalar as for the fluid scalar
          scatratimeparams->set<int>("INITFUNCNO", prbdyn.get<int>("STRUCTSCAL_INITFUNCNO"));
          break;
        default:
          FOUR_C_THROW("Your STRUCTSCAL_INITIALFIELD type is not supported!");
          break;
      }

      // structure scatra does not require any Neumann inflow boundary conditions
      scatratimeparams->set<std::string>("NEUMANNINFLOW", "no");
    }
    else if (disname == "scatra1")  // fluid_scatra discretisation
    {
      // fluid scatra does not require any convective heat transfer boundary conditions
      scatratimeparams->set<std::string>("CONV_HEAT_TRANS", "no");
    }
  }

  // -------------------------------------------------------------------
  // list for extra parameters
  // (put here everything that is not available in scatradyn or its sublists)
  // -------------------------------------------------------------------
  auto extraparams = Teuchos::rcp(new Teuchos::ParameterList());

  // ----------------Eulerian or ALE formulation of transport equation(s)
  extraparams->set<bool>("isale", isale);

  // ------------------------------------get also fluid turbulence sublist
  const auto& fdyn = GLOBAL::Problem::Instance()->FluidDynamicParams();
  extraparams->sublist("TURBULENCE MODEL") = fdyn.sublist("TURBULENCE MODEL");
  extraparams->sublist("SUBGRID VISCOSITY") = fdyn.sublist("SUBGRID VISCOSITY");
  extraparams->sublist("MULTIFRACTAL SUBGRID SCALES") = fdyn.sublist("MULTIFRACTAL SUBGRID SCALES");
  extraparams->sublist("TURBULENT INFLOW") = fdyn.sublist("TURBULENT INFLOW");

  // ------------------------------------get electromagnetic parameters
  extraparams->set<bool>("ELECTROMAGNETICDIFFUSION",
      CORE::UTILS::IntegralValue<int>(scatradyn, "ELECTROMAGNETICDIFFUSION"));
  extraparams->set<int>("EMDSOURCE", scatradyn.get<int>("EMDSOURCE"));

  // -------------------------------------------------------------------
  // algorithm construction depending on problem type and
  // time-integration (or stationary) scheme
  // -------------------------------------------------------------------
  auto timintscheme =
      CORE::UTILS::IntegralValue<INPAR::SCATRA::TimeIntegrationScheme>(scatradyn, "TIMEINTEGR");

  // low Mach number flow
  if (probtype == GLOBAL::ProblemType::loma or probtype == GLOBAL::ProblemType::thermo_fsi)
  {
    auto lomaparams =
        Teuchos::rcp(new Teuchos::ParameterList(GLOBAL::Problem::Instance()->LOMAControlParams()));
    switch (timintscheme)
    {
      case INPAR::SCATRA::timeint_gen_alpha:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::TimIntLomaGenAlpha(
            discret, solver, lomaparams, scatratimeparams, extraparams, output));
        break;
      }
      default:
        FOUR_C_THROW("Unknown time integration scheme for loMa!");
        break;
    }
  }

  // electrochemistry
  else if (probtype == GLOBAL::ProblemType::elch or
           ((probtype == GLOBAL::ProblemType::ssi and
                Teuchos::getIntegralValue<INPAR::SSI::ScaTraTimIntType>(
                    GLOBAL::Problem::Instance()->SSIControlParams(), "SCATRATIMINTTYPE") ==
                    INPAR::SSI::ScaTraTimIntType::elch) or
               (disname == "scatra" and
                   ((probtype == GLOBAL::ProblemType::ssti and
                        Teuchos::getIntegralValue<INPAR::SSTI::ScaTraTimIntType>(
                            GLOBAL::Problem::Instance()->SSTIControlParams(), "SCATRATIMINTTYPE") ==
                            INPAR::SSTI::ScaTraTimIntType::elch) or
                       (probtype == GLOBAL::ProblemType::sti and
                           Teuchos::getIntegralValue<INPAR::STI::ScaTraTimIntType>(
                               GLOBAL::Problem::Instance()->STIDynamicParams(),
                               "SCATRATIMINTTYPE") == INPAR::STI::ScaTraTimIntType::elch)))))
  {
    auto elchparams =
        Teuchos::rcp(new Teuchos::ParameterList(GLOBAL::Problem::Instance()->ELCHControlParams()));

    switch (timintscheme)
    {
      case INPAR::SCATRA::timeint_one_step_theta:
      {
        if (CORE::UTILS::IntegralValue<bool>(
                elchparams->sublist("SCL"), "ADD_MICRO_MACRO_COUPLING"))
        {
          if (disname == "scatra")
          {
            scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntElchSCLOST(
                discret, solver, elchparams, scatratimeparams, extraparams, output));
          }
          else if (disname == "scatra_micro")
          {
            scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntElchOST(
                discret, solver, elchparams, scatratimeparams, extraparams, output));
          }
          else
            FOUR_C_THROW("not identified");
        }
        else
        {
          scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntElchOST(
              discret, solver, elchparams, scatratimeparams, extraparams, output));
        }

        break;
      }
      case INPAR::SCATRA::timeint_bdf2:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntElchBDF2(
            discret, solver, elchparams, scatratimeparams, extraparams, output));
        break;
      }
      case INPAR::SCATRA::timeint_gen_alpha:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntElchGenAlpha(
            discret, solver, elchparams, scatratimeparams, extraparams, output));
        break;
      }
      case INPAR::SCATRA::timeint_stationary:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntElchStationary(
            discret, solver, elchparams, scatratimeparams, extraparams, output));
        break;
      }
      default:
        FOUR_C_THROW("Unknown time integration scheme for electrochemistry!");
        break;
    }
  }

  // levelset
  else if (probtype == GLOBAL::ProblemType::level_set or
           probtype == GLOBAL::ProblemType::fluid_xfem_ls)
  {
    Teuchos::RCP<Teuchos::ParameterList> lsparams = Teuchos::null;
    switch (probtype)
    {
      case GLOBAL::ProblemType::level_set:
        lsparams = Teuchos::rcp(new Teuchos::ParameterList(prbdyn));
        break;
      default:
      {
        if (lsparams.is_null())
          lsparams = Teuchos::rcp(
              new Teuchos::ParameterList(GLOBAL::Problem::Instance()->LevelSetControl()));
        // overrule certain parameters for coupled problems
        // this has already been ensured for scatratimeparams, but has also been ensured for the
        // level-set parameter in a hybrid approach time step size
        lsparams->set<double>("TIMESTEP", prbdyn.get<double>("TIMESTEP"));
        // maximum simulation time
        lsparams->set<double>("MAXTIME", prbdyn.get<double>("MAXTIME"));
        // maximum number of timesteps
        lsparams->set<int>("NUMSTEP", prbdyn.get<int>("NUMSTEP"));
        // restart
        lsparams->set<int>("RESTARTEVRY", prbdyn.get<int>("RESTARTEVRY"));
        // solution output
        lsparams->set<int>("RESULTSEVRY", prbdyn.get<int>("RESULTSEVRY"));

        break;
      }
    }

    switch (timintscheme)
    {
      case INPAR::SCATRA::timeint_one_step_theta:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::LevelSetTimIntOneStepTheta(
            discret, solver, lsparams, scatratimeparams, extraparams, output));
        break;
      }
      case INPAR::SCATRA::timeint_stationary:
      {
        // create instance of time integration class (call the constructor)
        switch (probtype)
        {
          case GLOBAL::ProblemType::level_set:
          {
            FOUR_C_THROW(
                "Stationary time integration scheme only supported for a selection of coupled "
                "level-set problems!");
            exit(EXIT_FAILURE);
          }
          default:
          {
            scatra_ = Teuchos::rcp(new SCATRA::LevelSetTimIntStationary(
                discret, solver, lsparams, scatratimeparams, extraparams, output));
            break;
          }
        }
        break;
      }
      case INPAR::SCATRA::timeint_gen_alpha:
      {
        switch (probtype)
        {
          default:
            FOUR_C_THROW("Unknown time-integration scheme for level-set problem");
            exit(EXIT_FAILURE);
        }

        break;
      }
      default:
        FOUR_C_THROW("Unknown time-integration scheme for level-set problem");
        break;
    }  // switch(timintscheme)
  }

  // cardiac monodomain
  else if (probtype == GLOBAL::ProblemType::cardiac_monodomain or
           (probtype == GLOBAL::ProblemType::ssi and
               Teuchos::getIntegralValue<INPAR::SSI::ScaTraTimIntType>(
                   GLOBAL::Problem::Instance()->SSIControlParams(), "SCATRATIMINTTYPE") ==
                   INPAR::SSI::ScaTraTimIntType::cardiac_monodomain))
  {
    auto cmonoparams =
        rcp(new Teuchos::ParameterList(GLOBAL::Problem::Instance()->EPControlParams()));

    // HDG implements all time stepping schemes within gen-alpha
    if (GLOBAL::Problem::Instance()->SpatialApproximationType() == CORE::FE::ShapeFunctionType::hdg)
    {
      scatra_ = Teuchos::rcp(new SCATRA::TimIntCardiacMonodomainHDG(
          discret, solver, cmonoparams, scatratimeparams, extraparams, output));
    }
    else
    {
      switch (timintscheme)
      {
        case INPAR::SCATRA::timeint_gen_alpha:
        {
          // create instance of time integration class (call the constructor)
          scatra_ = Teuchos::rcp(new SCATRA::TimIntCardiacMonodomainGenAlpha(
              discret, solver, cmonoparams, scatratimeparams, extraparams, output));
          break;
        }
        case INPAR::SCATRA::timeint_one_step_theta:
        {
          // create instance of time integration class (call the constructor)
          scatra_ = Teuchos::rcp(new SCATRA::TimIntCardiacMonodomainOST(
              discret, solver, cmonoparams, scatratimeparams, extraparams, output));
          break;
        }
        case INPAR::SCATRA::timeint_bdf2:
        {
          // create instance of time integration class (call the constructor)
          scatra_ = Teuchos::rcp(new SCATRA::TimIntCardiacMonodomainBDF2(
              discret, solver, cmonoparams, scatratimeparams, extraparams, output));
          break;
        }
        default:
          FOUR_C_THROW("Unknown time integration scheme for cardiac monodomain problem!");
          break;
      }  // switch(timintscheme)
    }
  }
  else if (probtype == GLOBAL::ProblemType::poromultiphasescatra)
  {
    switch (timintscheme)
    {
      case INPAR::SCATRA::timeint_gen_alpha:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntPoroMultiGenAlpha(
            discret, solver, Teuchos::null, scatratimeparams, extraparams, output));
        break;
      }
      case INPAR::SCATRA::timeint_one_step_theta:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntPoroMultiOST(
            discret, solver, Teuchos::null, scatratimeparams, extraparams, output));
        break;
      }
      case INPAR::SCATRA::timeint_bdf2:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntPoroMultiBDF2(
            discret, solver, Teuchos::null, scatratimeparams, extraparams, output));
        break;
      }
      case INPAR::SCATRA::timeint_stationary:
      {
        // create instance of time integration class (call the constructor)
        scatra_ = Teuchos::rcp(new SCATRA::ScaTraTimIntPoroMultiStationary(
            discret, solver, Teuchos::null, scatratimeparams, extraparams, output));
        break;
      }
      default:
        FOUR_C_THROW("Unknown time integration scheme for porous medium multiphase problem!");
        break;
    }  // switch(timintscheme)
  }
  // everything else
  else
  {
    // HDG implements all time stepping schemes within gen-alpha
    if (GLOBAL::Problem::Instance()->SpatialApproximationType() == CORE::FE::ShapeFunctionType::hdg)
    {
      switch (timintscheme)
      {
        case INPAR::SCATRA::timeint_one_step_theta:
        case INPAR::SCATRA::timeint_bdf2:
        case INPAR::SCATRA::timeint_gen_alpha:
        {
          scatra_ = Teuchos::rcp(
              new SCATRA::TimIntHDG(discret, solver, scatratimeparams, extraparams, output));
          break;
        }
        case INPAR::SCATRA::timeint_stationary:
        {
          scatra_ = Teuchos::rcp(new SCATRA::TimIntStationaryHDG(
              discret, solver, scatratimeparams, extraparams, output));
          break;
        }
        default:
        {
          FOUR_C_THROW("Unknown time-integration scheme for HDG scalar transport problem");
          break;
        }
      }
    }
    else
    {
      switch (timintscheme)
      {
        case INPAR::SCATRA::timeint_stationary:
        {
          // create instance of time integration class (call the constructor)
          scatra_ = Teuchos::rcp(
              new SCATRA::TimIntStationary(discret, solver, scatratimeparams, extraparams, output));
          break;
        }
        case INPAR::SCATRA::timeint_one_step_theta:
        {
          // create instance of time integration class (call the constructor)
          scatra_ = Teuchos::rcp(new SCATRA::TimIntOneStepTheta(
              discret, solver, scatratimeparams, extraparams, output));
          break;
        }
        case INPAR::SCATRA::timeint_bdf2:
        {
          // create instance of time integration class (call the constructor)
          scatra_ = Teuchos::rcp(
              new SCATRA::TimIntBDF2(discret, solver, scatratimeparams, extraparams, output));
          break;
        }
        case INPAR::SCATRA::timeint_gen_alpha:
        {
          // create instance of time integration class (call the constructor)
          scatra_ = Teuchos::rcp(
              new SCATRA::TimIntGenAlpha(discret, solver, scatratimeparams, extraparams, output));
          break;
        }
        default:
          FOUR_C_THROW("Unknown time-integration scheme for scalar transport problem");
          break;
      }  // switch(timintscheme)
    }
  }
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::ScaTraBaseAlgorithm::Init()
{
  SetIsSetup(false);

  // initialize scatra time integrator
  scatra_->Init();

  SetIsInit(true);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::ScaTraBaseAlgorithm::Setup()
{
  CheckIsInit();

  // setup the time integrator
  scatra_->Setup();

  // get the parameter list
  auto scatradyn = scatra_->ScatraParameterList();
  // get the discretization
  auto discret = scatra_->Discretization();

  // -------------------------------------------------------------------
  // what's the current problem type?
  // -------------------------------------------------------------------
  auto probtype = GLOBAL::Problem::Instance()->GetProblemType();

  // prepare fixing the null space for electrochemistry and sti
  if (probtype == GLOBAL::ProblemType::elch or
      (probtype == GLOBAL::ProblemType::sti and discret->Name() == "scatra" and
          Teuchos::getIntegralValue<INPAR::STI::ScaTraTimIntType>(
              GLOBAL::Problem::Instance()->STIDynamicParams(), "SCATRATIMINTTYPE") ==
              INPAR::STI::ScaTraTimIntType::elch))
  {
    auto elchparams =
        Teuchos::rcp(new Teuchos::ParameterList(GLOBAL::Problem::Instance()->ELCHControlParams()));

    // create a 2nd solver for block-preconditioning if chosen from input
    if (CORE::UTILS::IntegralValue<int>(*elchparams, "BLOCKPRECOND"))
    {
      const auto& solver = scatra_->Solver();

      const int linsolvernumber = scatradyn->get<int>("LINEAR_SOLVER");
      const auto prec = Teuchos::getIntegralValue<CORE::LINEAR_SOLVER::PreconditionerType>(
          GLOBAL::Problem::Instance()->SolverParams(linsolvernumber), "AZPREC");
      if (prec !=
          CORE::LINEAR_SOLVER::PreconditionerType::cheap_simple)  // TODO adapt error message
      {
        FOUR_C_THROW(
            "If SIMPLER flag is set to YES you can only use CheapSIMPLE as preconditioner in your "
            "fluid solver. Choose CheapSIMPLE in the SOLVER %i block in your dat file.",
            linsolvernumber);
      }

      solver->Params().sublist("CheapSIMPLE Parameters").set("Prec Type", "CheapSIMPLE");
      solver->Params().set(
          "ELCH", true);  // internal CheapSIMPLE modus for ML null space computation

      // add Inverse1 block for velocity dofs
      // tell Inverse1 block about NodalBlockInformation
      // In contrary to contact/meshtying problems this is necessary here, since we originally have
      // built the null space for the whole problem (velocity and pressure dofs). However, if we
      // split the matrix into velocity and pressure block, we have to adapt the null space
      // information for the subblocks. Therefore we need the nodal block information in the first
      // subblock for the velocities. The pressure null space is trivial to be built using a
      // constant vector
      auto& inv1 = solver->Params().sublist("CheapSIMPLE Parameters").sublist("Inverse1");
      inv1.sublist("NodalBlockInformation") = solver->Params().sublist("NodalBlockInformation");
    }
  }

  SetIsSetup(true);
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<DRT::ResultTest> ADAPTER::ScaTraBaseAlgorithm::CreateScaTraFieldTest()
{
  return scatra_->CreateScaTraFieldTest();
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::ScaTraBaseAlgorithm::CheckIsSetup() const
{
  if (not IsSetup()) FOUR_C_THROW("Setup() was not called.");
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
void ADAPTER::ScaTraBaseAlgorithm::CheckIsInit() const
{
  if (not IsInit()) FOUR_C_THROW("Init(...) was not called.");
}

FOUR_C_NAMESPACE_CLOSE