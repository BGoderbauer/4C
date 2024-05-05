/*-----------------------------------------------------------*/
/*! \file
\brief Parallel redistribution for CONTACT::AUG


\level 3
*/
/*-----------------------------------------------------------*/

#include "4C_contact_aug_parallel_distribution_controller.hpp"

#include "4C_contact_aug_strategy.hpp"
#include "4C_contact_paramsinterface.hpp"
#include "4C_io_pstream.hpp"

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void CONTACT::AUG::ParallelDistributionController::setup(CONTACT::ParamsInterface& cparams)
{
  // time measurement (on each processor)
  global_timer_.setComm(&strat_.Comm());
  global_timer_.reset();
  cparams.SetTimer(&global_timer_, 0);

  acttype_ = cparams.GetActionType();

  // rebuild the slave element evaluation time vector only if the redistribution
  // has been changed
  if (sele_eval_times_.is_null() or not sele_eval_times_->Map().SameAs(data_.GSeleColMap()))
  {
    sele_eval_times_ = Teuchos::null;
    sele_eval_times_ = Teuchos::rcp(new Epetra_Vector(data_.GSeleColMap()));
  }
  // otherwise set its content to zero
  else
    sele_eval_times_->PutScalar(0.0);

  cparams.Set(sele_eval_times_.get(), 0);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void CONTACT::AUG::ParallelDistributionController::check(CONTACT::ParamsInterface& cparams)
{
  cparams.ClearEntry(CORE::GEN::AnyDataContainer::DataType::time_monitor, 0);
  cparams.ClearEntry(CORE::GEN::AnyDataContainer::DataType::any, 0);

  global_timer_.write(IO::cout.os(IO::verbose));

  if (data_.GSeleEvalTimesPtr().is_null() or
      not sele_eval_times_->Map().SameAs(data_.GSeleEvalTimesPtr()->Map()))
  {
    data_.GSeleEvalTimesPtr() = Teuchos::rcp(new Epetra_Vector(*sele_eval_times_));
  }

  // average over the last force/stiff and pure force evaluation
  switch (acttype_)
  {
    case MORTAR::eval_force_stiff:
    {
      // delete too old information
      data_.UnbalanceElementFactors().clear();
      data_.UnbalanceTimeFactors().clear();

      // (re)set the slave element evaluation times
      data_.GSeleEvalTimesPtr()->Scale(1.0, *sele_eval_times_);
      break;
    }
    case MORTAR::eval_force:
      data_.GSeleEvalTimesPtr()->Update(1.0, *sele_eval_times_, 1.0);
      strat_.SpreadGlobalSeleEvalTimesToInterfaces();
      break;
    // in all other cases: return
    default:
      return;
  }

  strat_.CheckParallelDistribution(global_timer_);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
bool CONTACT::AUG::ParallelDistributionController::redistribute(
    const Teuchos::RCP<const Epetra_Vector>& dis, Teuchos::RCP<const Epetra_Vector> vel,
    const int nlniter)
{
  if (nlniter % interval_ == 0)
  {
    const bool is_redis = strat_.RedistributeContact(dis, vel);
    if (is_redis) sele_eval_times_ = Teuchos::null;

    return is_redis;
  }

  return false;
}

FOUR_C_NAMESPACE_CLOSE