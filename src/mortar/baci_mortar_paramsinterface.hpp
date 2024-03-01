/*----------------------------------------------------------------------*/
/*! \file
\brief Mortar parameter interface.

\level 2

*/
/*-----------------------------------------------------------------------*/

#ifndef BACI_MORTAR_PARAMSINTERFACE_HPP
#define BACI_MORTAR_PARAMSINTERFACE_HPP

#include "baci_config.hpp"

#include "baci_utils_any_data_container.hpp"

BACI_NAMESPACE_OPEN

namespace MORTAR
{
  //! Actions to be performed by mortar/contact framework
  enum ActionType : int
  {
    eval_none,        /*!< No evaluation type has been chosen */
    eval_force_stiff, /*!< Evaluation of the contact/meshtying right-hand-side and the
                         eval_force_stiff contact/meshtying jacobian. We call this method also, when
                         we are only interested in the jacobian, since the created overhead is
                         negligible. */
    eval_force,       /*!< Evaluation of the contact/meshtying right-hand-side only. Necessary and
                         meaningful for line search strategies for example. */
    eval_run_pre_evaluate,   /*!< Run at the very beginning of a call to
                                STR::ModelEvaluator::EvaluteForce/Stiff/ForceStiff */
    eval_run_post_evaluate,  /*!< Run in the end of a call to
                                STR::ModelEvaluator::EvaluteForce/Stiff/ForceStiff */
    eval_run_post_compute_x, /*!< recover internal quantities, e.g. Lagrange multipliers */
    eval_reset, /*!< reset internal quantities, e.g. displacement state and/or Lagrange multipliers
                 */
    eval_run_pre_compute_x, /*!< augment the solution direction at the very beginning of a ComputeX
                             */
    eval_run_post_iterate,  /*!< run in the end of a ::NOX::Solver::Step() call */
    eval_run_pre_solve,     /*!< run at the beginning of a ::NOX::Solver::solve() call */
    eval_contact_potential, /*!< Evaluate the contact potential */
    eval_wgap_gradient_error,   /*!< Evaluate the error of the weighted gap gradient */
    eval_static_constraint_rhs, /*!< Evaluate only the contributions to the constraint rhs. The
                                   active set is not updated during the evaluation. */
    eval_run_post_apply_jacobian_inverse,       /*!< run in the end of a
                                                   NOX::NLN::LinearSystem::applyJacobianInverse call */
    eval_correct_parameters,                    /*!< correct or adapt contact parameters */
    remove_condensed_contributions_from_str_rhs /*!< remove any condensed contact contributions from
                                                   the structural rhs */
  };

  /*! \brief Convert MORTAR::ActionType enum to string
   *
   * @param[in] act ActionType encoded as enum
   * @return String describing the action type
   */
  static inline std::string ActionType2String(const enum ActionType& act)
  {
    switch (act)
    {
      case eval_none:
        return "eval_none";
      case eval_force_stiff:
        return "eval_force_stiff";
      case eval_force:
        return "eval_force";
      case eval_run_pre_evaluate:
        return "eval_run_pre_evaluate";
      case eval_run_post_evaluate:
        return "eval_run_post_evaluate";
      case MORTAR::eval_reset:
        return "eval_reset";
      case MORTAR::eval_run_post_compute_x:
        return "eval_run_post_compute_x";
      case MORTAR::eval_run_pre_compute_x:
        return "eval_run_pre_compute_x";
      case MORTAR::eval_run_post_iterate:
        return "eval_run_post_iterate";
      case MORTAR::eval_contact_potential:
        return "eval_contact_potential";
      case MORTAR::eval_wgap_gradient_error:
        return "eval_wgap_gradient_error";
      case MORTAR::eval_static_constraint_rhs:
        return "eval_static_constraint_rhs";
      case MORTAR::eval_run_post_apply_jacobian_inverse:
        return "eval_run_post_apply_jacobian_inverse";
      case MORTAR::eval_correct_parameters:
        return "eval_correct_parameters";
      case remove_condensed_contributions_from_str_rhs:
        return "remove_condensed_contributions_from_str_rhs";
      case eval_run_pre_solve:
        return "eval_run_pre_solve";
      default:
        return "unknown [enum = " + std::to_string(act) + "]";
    }
    return "";
  };

  /*! \brief Mortar parameter interface
   *
   * Necessary for the communication between the structural time integration framework and the
   * mortar strategies.
   */
  class ParamsInterface : public CORE::GEN::AnyDataContainer
  {
   public:
    //! destructor
    virtual ~ParamsInterface() = default;

    //! Return the mortar/contact action type
    virtual enum ActionType GetActionType() const = 0;

    //! Get the nonlinear iteration number
    virtual int GetNlnIter() const = 0;

    //! Get the current time step counter \f$(n+1)\f$
    virtual int GetStepNp() const = 0;

    /*! \brief Get time step number from which the current simulation has been restarted
     *
     * Equal to 0 if no restart has been performed.
     */
    virtual int GetRestartStep() const = 0;

  };  // class ParamsInterface
}  // namespace MORTAR


BACI_NAMESPACE_CLOSE

#endif  // MORTAR_PARAMSINTERFACE_H