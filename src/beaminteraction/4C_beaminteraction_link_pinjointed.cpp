// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_beaminteraction_link_pinjointed.hpp"

#include "4C_beaminteraction_link.hpp"
#include "4C_beaminteraction_link_beam3_reissner_line2_pinjointed.hpp"
#include "4C_beaminteraction_link_truss.hpp"
#include "4C_comm_pack_helpers.hpp"
#include "4C_fem_general_largerotations.hpp"
#include "4C_inpar_beaminteraction.hpp"
#include "4C_linalg_serialdensematrix.hpp"
#include "4C_linalg_serialdensevector.hpp"
#include "4C_utils_exceptions.hpp"

#include <memory>

FOUR_C_NAMESPACE_OPEN


BEAMINTERACTION::BeamLinkPinJointedType BEAMINTERACTION::BeamLinkPinJointedType::instance_;


/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
BEAMINTERACTION::BeamLinkPinJointed::BeamLinkPinJointed() : BeamLink() {}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
BEAMINTERACTION::BeamLinkPinJointed::BeamLinkPinJointed(
    const BEAMINTERACTION::BeamLinkPinJointed& old)
    : BEAMINTERACTION::BeamLink(old)
{
  return;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void BEAMINTERACTION::BeamLinkPinJointed::init(int id,
    const std::vector<std::pair<int, int>>& eleids,
    const std::vector<Core::LinAlg::Matrix<3, 1>>& initpos,
    const std::vector<Core::LinAlg::Matrix<3, 3>>& inittriad,
    Inpar::BEAMINTERACTION::CrosslinkerType linkertype, double timelinkwasset)
{
  issetup_ = false;

  BeamLink::init(id, eleids, initpos, inittriad, linkertype, timelinkwasset);

  issetup_ = true;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void BEAMINTERACTION::BeamLinkPinJointed::setup(const int matnum)
{
  check_init();

  // the flag issetup_ will be set in the derived method!
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void BEAMINTERACTION::BeamLinkPinJointed::pack(Core::Communication::PackBuffer& data) const
{
  // pack type of this instance of ParObject
  int type = unique_par_object_id();
  add_to_pack(data, type);
  // add base class Element
  BeamLink::pack(data);

  return;
}

/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
void BEAMINTERACTION::BeamLinkPinJointed::unpack(Core::Communication::UnpackBuffer& buffer)
{
  Core::Communication::extract_and_assert_id(buffer, unique_par_object_id());

  // extract base class Element
  BeamLink::unpack(buffer);



  return;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void BEAMINTERACTION::BeamLinkPinJointed::reset_state(
    std::vector<Core::LinAlg::Matrix<3, 1>>& bspotpos,
    std::vector<Core::LinAlg::Matrix<3, 3>>& bspottriad)
{
  check_init_setup();

  BeamLink::reset_state(bspotpos, bspottriad);
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
std::shared_ptr<BEAMINTERACTION::BeamLinkPinJointed> BEAMINTERACTION::BeamLinkPinJointed::create(
    Inpar::BEAMINTERACTION::JointType type)
{
  if (type == Inpar::BEAMINTERACTION::beam3r_line2_pin)
    return std::make_shared<BEAMINTERACTION::BeamLinkBeam3rLine2PinJointed>();
  else if (type == Inpar::BEAMINTERACTION::truss)
    return std::make_shared<BEAMINTERACTION::BeamLinkTruss>();
  else
    FOUR_C_THROW(
        "instantiation of new BeamLinkPinJointed object failed due to "
        "unknown type of linker");

  return nullptr;
}

/*----------------------------------------------------------------------------*
 *----------------------------------------------------------------------------*/
void BEAMINTERACTION::BeamLinkPinJointed::print(std::ostream& out) const
{
  check_init();

  BeamLink::print(out);

  out << "\n";
}

FOUR_C_NAMESPACE_CLOSE
