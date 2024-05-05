/*----------------------------------------------------------------------*/
/*! \file

\brief unit testing functionality for the brokenrational contact constitutivelaw

\level 2

*/
/*----------------------------------------------------------------------*/

#include <gtest/gtest.h>

#include "4C_contact_constitutivelaw_brokenrational_contactconstitutivelaw.hpp"
#include "4C_contact_node.hpp"

namespace
{
  using namespace FourC;

  class BrokenrationalConstitutiveLawTest : public ::testing::Test
  {
   public:
    BrokenrationalConstitutiveLawTest()
    {
      // initialize container for material parameters
      const Teuchos::RCP<CONTACT::CONSTITUTIVELAW::Container> container =
          Teuchos::rcp(new CONTACT::CONSTITUTIVELAW::Container(1,
              INPAR::CONTACT::ConstitutiveLawType::colaw_brokenrational,
              "Brokenrational Constitutivelaw"));

      // add parameters to container
      container->Add("A", -2.);
      container->Add("B", 4.);
      container->Add("C", -0.5);
      container->Add("Offset", 0.5);

      const Teuchos::RCP<CONTACT::CONSTITUTIVELAW::ConstitutiveLaw> brokenrationalcoconstlaw =
          CONTACT::CONSTITUTIVELAW::ConstitutiveLaw::Factory(container);
      coconstlaw_ = brokenrationalcoconstlaw;
    }

    Teuchos::RCP<CONTACT::CONSTITUTIVELAW::ConstitutiveLaw> coconstlaw_;

    Teuchos::RCP<CONTACT::Node> cnode;
  };

  //! test member function Evaluate
  TEST_F(BrokenrationalConstitutiveLawTest, TestEvaluate)
  {
    // gap < 0
    EXPECT_ANY_THROW(coconstlaw_->Evaluate(1.0, cnode.get()));
    // 0< gap < offset
    EXPECT_ANY_THROW(coconstlaw_->Evaluate(-0.25, cnode.get()));
    // offset < gap
    EXPECT_NEAR(coconstlaw_->Evaluate(-2.5, cnode.get()), -0.5, 1.e-15);
  }

  //! test member function EvaluateDeriv
  TEST_F(BrokenrationalConstitutiveLawTest, TestEvaluateDeriv)
  {
    EXPECT_NEAR(coconstlaw_->EvaluateDeriv(-2.5, cnode.get()), 0.5, 1.e-15);
    EXPECT_ANY_THROW(coconstlaw_->EvaluateDeriv(-0.25, cnode.get()));
  }
}  // namespace