/*-----------------------------------------------------------*/
/*! \file

\brief wrapper class for a user derived NOX PrePostOperator



\level 3

*/
/*-----------------------------------------------------------*/

#include "baci_solver_nonlin_nox_group_prepostoperator.H"

#include <Teuchos_ParameterList.hpp>

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
NOX::NLN::GROUP::PrePostOperator::PrePostOperator() : havePrePostOperator_(false)
{
  // Disallowed constructor
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
NOX::NLN::GROUP::PrePostOperator::PrePostOperator(const PrePostOperator& ppo)
    : havePrePostOperator_(false)
{
  // Disallowed copy constructor
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
NOX::NLN::GROUP::PrePostOperator& NOX::NLN::GROUP::PrePostOperator::operator=(
    const PrePostOperator& ppo)
{
  // disallowed assignment operator
  return *this;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
NOX::NLN::GROUP::PrePostOperator::PrePostOperator(Teuchos::ParameterList& groupOptionsSubList)
    : havePrePostOperator_(false), prePostOperatorMapPtr_(Teuchos::null)
{
  reset(groupOptionsSubList);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void NOX::NLN::GROUP::PrePostOperator::reset(Teuchos::ParameterList& groupOptionsSubList)
{
  havePrePostOperator_ = false;

  /* Check if a pre/post operator for the group is provided
   * by the user. */
  if (groupOptionsSubList.INVALID_TEMPLATE_QUALIFIER isType<Teuchos::RCP<Map>>(
          "User Defined Pre/Post Operator"))
  {
    prePostOperatorMapPtr_ = groupOptionsSubList.INVALID_TEMPLATE_QUALIFIER get<Teuchos::RCP<Map>>(
        "User Defined Pre/Post Operator");
    havePrePostOperator_ = true;
  }
}

// non-member function
/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
NOX::NLN::GROUP::PrePostOperator::Map& NOX::NLN::GROUP::PrePostOp::GetMap(
    Teuchos::ParameterList& p_grp_opt)
{
  Teuchos::RCP<NOX::NLN::GROUP::PrePostOperator::Map>& mapptr =
      p_grp_opt.get<Teuchos::RCP<NOX::NLN::GROUP::PrePostOperator::Map>>(
          "User Defined Pre/Post Operator",
          Teuchos::rcp(new NOX::NLN::GROUP::PrePostOperator::Map()));

  return *mapptr;
}