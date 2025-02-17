// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_so3_plast_ssn_sosh8.hpp"

#include "4C_global_data.hpp"
#include "4C_io_linedefinition.hpp"
#include "4C_linalg_fixedsizematrix_solver.hpp"
#include "4C_linalg_serialdensematrix.hpp"
#include "4C_linalg_serialdensevector.hpp"
#include "4C_mat_plasticelasthyper.hpp"
#include "4C_so3_plast_ssn.hpp"
#include "4C_so3_utils.hpp"
#include "4C_structure_new_elements_paramsinterface.hpp"
#include "4C_utils_parameter_list.hpp"
#include "4C_utils_shared_ptr_from_ref.hpp"

#include <Teuchos_SerialDenseSolver.hpp>

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*
 | build an instance of plast type                         seitz 05/14 |
 *----------------------------------------------------------------------*/
Discret::Elements::SoSh8PlastType Discret::Elements::SoSh8PlastType::instance_;
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<Core::FE::CellType::hex8>::nsd_,
                    Discret::Elements::So3Plast<Core::FE::CellType::hex8>::nsd_>>
    Discret::Elements::SoSh8Plast::jac_refe_;
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<Core::FE::CellType::hex8>::nsd_,
                    Discret::Elements::So3Plast<Core::FE::CellType::hex8>::nsd_>>
    Discret::Elements::SoSh8Plast::jac_curr_;
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::SoSh8Plast::num_ans *
                                         Discret::Elements::SoSh8Plast::num_sp,
                    Discret::Elements::SoSh8Plast::numdofperelement_>>
    Discret::Elements::SoSh8Plast::B_ans_loc_;
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::SoSh8Plast::numstr_,
                    Discret::Elements::SoSh8Plast::numstr_>>
    Discret::Elements::SoSh8Plast::TinvT_;



Discret::Elements::SoSh8PlastType& Discret::Elements::SoSh8PlastType::instance()
{
  return instance_;
}

/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 05/14 |
| is called in ElementRegisterType                                     |
*----------------------------------------------------------------------*/
Core::Communication::ParObject* Discret::Elements::SoSh8PlastType::create(
    Core::Communication::UnpackBuffer& buffer)
{
  auto* object = new Discret::Elements::SoSh8Plast(-1, -1);
  object->unpack(buffer);
  return object;
}

/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 05/14 |
| is called from ParObjectFactory                                      |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoSh8PlastType::create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == get_element_type_string())
  {
    std::shared_ptr<Core::Elements::Element> ele =
        std::make_shared<Discret::Elements::SoSh8Plast>(id, owner);
    return ele;
  }
  return nullptr;
}

/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 05/14 |
| virtual method of ElementType                                        |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoSh8PlastType::create(
    const int id, const int owner)
{
  std::shared_ptr<Core::Elements::Element> ele =
      std::make_shared<Discret::Elements::SoSh8Plast>(id, owner);
  return ele;
}

/*----------------------------------------------------------------------*
| initialise the element (public)                          seitz 05/14 |
*----------------------------------------------------------------------*/
int Discret::Elements::SoSh8PlastType::initialize(Core::FE::Discretization& dis)
{
  // sosh8_gmshplotdis(dis);

  int num_morphed_so_hex8_easmild = 0;
  int num_morphed_so_hex8_easnone = 0;

  // Loop through all elements
  for (int i = 0; i < dis.num_my_col_elements(); ++i)
  {
    // get the actual element
    if (dis.l_col_element(i)->element_type() != *this) continue;
    auto* actele = dynamic_cast<Discret::Elements::SoSh8Plast*>(dis.l_col_element(i));
    if (!actele) FOUR_C_THROW("cast to So_sh8* failed");

    if (!actele->nodes_rearranged_)
    {
      switch (actele->thickdir_)
      {
        // check for automatic definition of thickness direction
        case Discret::Elements::SoSh8Plast::autoj:
        {
          actele->thickdir_ = actele->findthickdir();
          break;
        }
        // check for enforced definition of thickness direction
        case Discret::Elements::SoSh8Plast::globx:
        {
          Core::LinAlg::Matrix<NUMDIM_SOH8, 1> thickdirglo(true);
          thickdirglo(0) = 1.0;
          actele->thickdir_ = actele->enfthickdir(thickdirglo);
          break;
        }
        case Discret::Elements::SoSh8Plast::globy:
        {
          Core::LinAlg::Matrix<NUMDIM_SOH8, 1> thickdirglo(true);
          thickdirglo(1) = 1.0;
          actele->thickdir_ = actele->enfthickdir(thickdirglo);
          break;
        }
        case Discret::Elements::SoSh8Plast::globz:
        {
          Core::LinAlg::Matrix<NUMDIM_SOH8, 1> thickdirglo(true);
          thickdirglo(2) = 1.0;
          actele->thickdir_ = actele->enfthickdir(thickdirglo);
          break;
        }
        default:
          break;
      }

      int new_nodeids[NUMNOD_SOH8];

      switch (actele->thickdir_)
      {
        case Discret::Elements::SoSh8Plast::globx:
        case Discret::Elements::SoSh8Plast::globy:
        case Discret::Elements::SoSh8Plast::globz:
        {
          FOUR_C_THROW("This should have been replaced by auto(r|s|t)");
          break;
        }
        case Discret::Elements::SoSh8Plast::autor:
        case Discret::Elements::SoSh8Plast::enfor:
        {
          // resorting of nodes,
          // such that previous local r-dir is local t-dir afterwards
          new_nodeids[0] = actele->node_ids()[7];
          new_nodeids[1] = actele->node_ids()[4];
          new_nodeids[2] = actele->node_ids()[0];
          new_nodeids[3] = actele->node_ids()[3];
          new_nodeids[4] = actele->node_ids()[6];
          new_nodeids[5] = actele->node_ids()[5];
          new_nodeids[6] = actele->node_ids()[1];
          new_nodeids[7] = actele->node_ids()[2];
          actele->set_node_ids(NUMNOD_SOH8, new_nodeids);
          actele->nodes_rearranged_ = true;
          break;
        }
        case Discret::Elements::SoSh8Plast::autos:
        case Discret::Elements::SoSh8Plast::enfos:
        {
          // resorting of nodes,
          // such that previous local s-dir is local t-dir afterwards
          new_nodeids[0] = actele->node_ids()[4];
          new_nodeids[1] = actele->node_ids()[5];
          new_nodeids[2] = actele->node_ids()[1];
          new_nodeids[3] = actele->node_ids()[0];
          new_nodeids[4] = actele->node_ids()[7];
          new_nodeids[5] = actele->node_ids()[6];
          new_nodeids[6] = actele->node_ids()[2];
          new_nodeids[7] = actele->node_ids()[3];
          actele->set_node_ids(NUMNOD_SOH8, new_nodeids);
          actele->nodes_rearranged_ = true;
          break;
        }
        case Discret::Elements::SoSh8Plast::autot:
        case Discret::Elements::SoSh8Plast::enfot:
        {
          // no resorting necessary
          for (int node = 0; node < 8; ++node)
          {
            new_nodeids[node] = actele->node_ids()[node];
          }
          actele->set_node_ids(NUMNOD_SOH8, new_nodeids);
          actele->nodes_rearranged_ = true;
          break;
        }
        case Discret::Elements::SoSh8Plast::undefined:
        {
          if (actele->eastype_ == Discret::Elements::soh8p_eassosh8)
          {
            // here comes plan B: morph So_sh8 to So_hex8
            actele->re_init_eas(Discret::Elements::soh8p_easmild);
            actele->anstype_ = SoSh8Plast::ansnone_p;
            actele->init_jacobian_mapping();
            num_morphed_so_hex8_easmild++;
          }
          else if (actele->eastype_ == Discret::Elements::soh8p_easnone)
          {
            // here comes plan B: morph So_sh8 to So_hex8
            actele->re_init_eas(Discret::Elements::soh8p_easnone);
            actele->anstype_ = SoSh8Plast::ansnone_p;
            actele->init_jacobian_mapping();
            num_morphed_so_hex8_easnone++;
          }
          else if (actele->eastype_ == Discret::Elements::soh8p_easmild)
          {
            // this might happen in post filter (for morped sosh8->soh8)
            actele->re_init_eas(Discret::Elements::soh8p_easmild);
            actele->anstype_ = SoSh8Plast::ansnone_p;
            actele->init_jacobian_mapping();
          }
          else if (actele->eastype_ == Discret::Elements::soh8p_easnone)
          {
            // this might happen in post filter (for morped sosh8->soh8)
            actele->anstype_ = SoSh8Plast::ansnone_p;
            actele->init_jacobian_mapping();
          }
          else
            FOUR_C_THROW("Undefined EAS type");
          break;
        }
        case Discret::Elements::SoSh8Plast::none:
          break;
        default:
          FOUR_C_THROW("no thickness direction for So_sh8");
          break;
      }
    }
  }

  if (num_morphed_so_hex8_easmild > 0)
  {
    std::cout << std::endl
              << num_morphed_so_hex8_easmild
              << " Sosh8Plast-Elements have no clear 'thin' direction and have morphed to "
                 "So_hex8Plast with eas_mild"
              << std::endl;
  }
  if (num_morphed_so_hex8_easnone > 0)
  {
    std::cout << std::endl
              << num_morphed_so_hex8_easnone
              << " Sosh8Plast-Elements have no clear 'thin' direction and have morphed to "
                 "So_hex8Plast with eas_none"
              << std::endl;
  }

  // fill complete again to reconstruct element-node pointers,
  // but without element init, etc.
  dis.fill_complete(false, false, false);

  // loop again to init Jacobian for Sosh8's
  for (int i = 0; i < dis.num_my_col_elements(); ++i)
  {
    if (dis.l_col_element(i)->element_type() != *this) continue;
    auto* actele = dynamic_cast<Discret::Elements::SoSh8Plast*>(dis.l_col_element(i));
    if (!actele) FOUR_C_THROW("cast to So_sh8* failed");
    actele->init_jacobian_mapping();
  }

  return 0;
}

/*---------------------------------------------------------------------*
|                                                          seitz 05/14 |
*----------------------------------------------------------------------*/
void Discret::Elements::SoSh8PlastType::setup_element_definition(
    std::map<std::string, std::map<std::string, Input::LineDefinition>>& definitions)
{
  std::map<std::string, Input::LineDefinition>& defs = definitions[get_element_type_string()];

  defs["HEX8"] = Input::LineDefinition::Builder()
                     .add_int_vector("HEX8", 8)
                     .add_named_int("MAT")
                     .add_named_string("KINEM")
                     .add_named_string("EAS")
                     .add_named_string("ANS")
                     .add_named_string("THICKDIR")
                     .add_optional_named_double_vector("FIBER1", 3)
                     .add_optional_named_double_vector("FIBER2", 3)
                     .add_optional_named_double_vector("FIBER3", 3)
                     .build();

}  // setup_element_definition()

/*----------------------------------------------------------------------*
 | ctor (public)                                            seitz 05/14 |
 *----------------------------------------------------------------------*/
Discret::Elements::SoSh8Plast::SoSh8Plast(int id, int owner)
    : SoBase(id, owner), Discret::Elements::So3Plast<Core::FE::CellType::hex8>(id, owner)
{
  thickdir_ = globx;
  nodes_rearranged_ = false;
  thickvec_.resize(3, 0.);

  std::shared_ptr<const Teuchos::ParameterList> params =
      Global::Problem::instance()->get_parameter_list();
  if (params != nullptr)
  {
    Discret::Elements::Utils::throw_error_fd_material_tangent(
        Global::Problem::instance()->structural_dynamic_params(), get_element_type_string());
  }

  return;
}

/*----------------------------------------------------------------------*
 | copy-ctor (public)                                       seitz 05/14 |
 *----------------------------------------------------------------------*/
Discret::Elements::SoSh8Plast::SoSh8Plast(const Discret::Elements::SoSh8Plast& old)
    : SoBase(old), Discret::Elements::So3Plast<Core::FE::CellType::hex8>(old)
{
  return;
}

/*----------------------------------------------------------------------*
 | deep copy this instance of Solid3 and return pointer to              |
 | it (public)                                              seitz 05/14 |
 *----------------------------------------------------------------------*/
Core::Elements::Element* Discret::Elements::SoSh8Plast::clone() const
{
  auto* newelement = new Discret::Elements::SoSh8Plast(*this);
  return newelement;
}

/*----------------------------------------------------------------------*
 | pack data (public)                                       seitz 05/14 |
 *----------------------------------------------------------------------*/
void Discret::Elements::SoSh8Plast::pack(Core::Communication::PackBuffer& data) const
{
  // pack type of this instance of ParObject
  int type = unique_par_object_id();
  add_to_pack(data, type);
  // add base class So3Plast Element
  Discret::Elements::So3Plast<Core::FE::CellType::hex8>::pack(data);
  // thickdir
  add_to_pack(data, thickdir_);
  add_to_pack(data, thickvec_);
  add_to_pack(data, anstype_);
  add_to_pack(data, nodes_rearranged_);

  return;
}

/*----------------------------------------------------------------------*
 | unpack data (public)                                     seitz 05/14 |
 *----------------------------------------------------------------------*/
void Discret::Elements::SoSh8Plast::unpack(Core::Communication::UnpackBuffer& buffer)
{
  Core::Communication::extract_and_assert_id(buffer, unique_par_object_id());

  // extract base class So_hex8 Element
  Discret::Elements::So3Plast<Core::FE::CellType::hex8>::unpack(buffer);
  // thickdir
  extract_from_pack(buffer, thickdir_);
  extract_from_pack(buffer, thickvec_);
  extract_from_pack(buffer, anstype_);
  extract_from_pack(buffer, nodes_rearranged_);
}

void Discret::Elements::SoSh8Plast::print(std::ostream& os) const
{
  os << "So_sh8Plast ";
  Element::print(os);
  std::cout << std::endl;
}

/*----------------------------------------------------------------------*
 | read this element, get the material (public)             seitz 05/14 |
 *----------------------------------------------------------------------*/
bool Discret::Elements::SoSh8Plast::read_element(const std::string& eletype,
    const std::string& distype, const Core::IO::InputParameterContainer& container)
{
  std::string buffer = container.get<std::string>("KINEM");

  // geometrically linear
  if (buffer == "linear")
  {
    FOUR_C_THROW("no linear kinematics");
  }
  // geometrically non-linear with Total Lagrangean approach
  else if (buffer == "nonlinear")
  {
    kintype_ = Inpar::Solid::KinemType::nonlinearTotLag;
    // everything ok
  }
  else
    FOUR_C_THROW("Reading of SO3_PLAST element failed! KINEM unknown");

  Core::FE::GaussIntegration ip(Core::FE::CellType::hex8, 3);
  numgpt_ = ip.num_points();
  xsi_.resize(numgpt_);
  wgt_.resize(numgpt_);
  for (int gp = 0; gp < numgpt_; ++gp)
  {
    wgt_[gp] = ip.weight(gp);
    const double* gpcoord = ip.point(gp);
    for (int idim = 0; idim < nsd_; idim++) xsi_[gp](idim) = gpcoord[idim];
  }

  // no fbar
  fbar_ = false;

  // read number of material model
  int material_id = container.get<int>("MAT");

  set_material(0, Mat::factory(material_id));

  std::shared_ptr<Mat::So3Material> so3mat = solid_material();
  so3mat->setup(numgpt_, container);
  so3mat->valid_kinematics(Inpar::Solid::KinemType::nonlinearTotLag);
  if (have_plastic_spin())
    plspintype_ = plspin;
  else
    plspintype_ = zerospin;

  // EAS
  buffer = container.get<std::string>("EAS");

  if (buffer == "none")
    eastype_ = soh8p_easnone;
  else if (buffer == "sosh8")
    eastype_ = soh8p_eassosh8;
  else
    FOUR_C_THROW("unknown EAS type for so3_plast");

  // initialize EAS data
  eas_init();

  // read ANS technology flag
  buffer = container.get<std::string>("ANS");
  if (buffer == "sosh8")
  {
    anstype_ = anssosh8_p;
  }
  // no ANS technology
  else if (buffer == "none")
  {
    anstype_ = ansnone_p;
  }
  else
    FOUR_C_THROW("Reading of SO_SH8 ANS technology failed");

  buffer = container.get<std::string>("THICKDIR");
  nodes_rearranged_ = false;

  // global X
  if (buffer == "xdir") thickdir_ = globx;
  // global Y
  else if (buffer == "ydir")
    thickdir_ = globy;
  // global Z
  else if (buffer == "zdir")
    thickdir_ = globz;
  // find automatically through Jacobian of Xrefe
  else if (buffer == "auto")
    thickdir_ = autoj;
  // local r
  else if (buffer == "rdir")
    thickdir_ = enfor;
  // local s
  else if (buffer == "sdir")
    thickdir_ = enfos;
  // local t
  else if (buffer == "tdir")
    thickdir_ = enfot;
  // no noderearrangement
  else if (buffer == "none")
  {
    thickdir_ = none;
    nodes_rearranged_ = true;
  }
  else
    FOUR_C_THROW("Reading of SO_SH8 thickness direction failed");

  // plasticity related stuff
  KbbInv_.resize(numgpt_, Core::LinAlg::SerialDenseMatrix(plspintype_, plspintype_, true));
  Kbd_.resize(numgpt_, Core::LinAlg::SerialDenseMatrix(plspintype_, numdofperelement_, true));
  fbeta_.resize(numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));
  dDp_last_iter_.resize(numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));
  dDp_inc_.resize(numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));

  Teuchos::ParameterList plparams = Global::Problem::instance()->semi_smooth_plast_params();
  Core::Utils::add_enum_class_to_parameter_list(
      "Core::ProblemType", Global::Problem::instance()->get_problem_type(), plparams);
  read_parameter_list(Core::Utils::shared_ptr_from_ref<Teuchos::ParameterList>(plparams));

  if (tsi_)
    FOUR_C_THROW(
        "no tsi for solid shells.\n"
        "Go back to the revision introducing this error to find a version with TSI.\n"
        "Note: the strain rate used for the Gough Joule effect was not calculated\n"
        "consistently with the ANS modifications");

  return true;
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
Discret::Elements::SoSh8Plast::ThicknessDirection Discret::Elements::SoSh8Plast::findthickdir()
{
  // update element geometry
  Core::LinAlg::Matrix<nen_, nsd_> xrefe(false);  // material coord. of element
  for (int i = 0; i < nen_; ++i)
  {
    xrefe(i, 0) = this->nodes()[i]->x()[0];
    xrefe(i, 1) = this->nodes()[i]->x()[1];
    xrefe(i, 2) = this->nodes()[i]->x()[2];
  }
  // vector of df(origin), ie parametric derivatives of shape functions
  // evaluated at the origin (r,s,t)=(0,0,0)
  const double df0_vector[] = {-0.125, -0.125, -0.125, +0.125, -0.125, -0.125, +0.125, +0.125,
      -0.125, -0.125, +0.125, -0.125, -0.125, -0.125, +0.125, +0.125, -0.125, +0.125, +0.125,
      +0.125, +0.125, -0.125, +0.125, +0.125};
  // shape function derivatives, evaluated at origin (r=s=t=0.0)
  Core::LinAlg::Matrix<nsd_, nen_> df0(df0_vector);

  // compute Jacobian, evaluated at element origin (r=s=t=0.0)
  // (J0_i^A) = (X^A_{,i})^T
  Core::LinAlg::Matrix<nsd_, nsd_> jac0;
  jac0.multiply_nn(df0, xrefe);
  // compute inverse of Jacobian at element origin
  // (Jinv0_A^i) = (X^A_{,i})^{-T}
  Core::LinAlg::Matrix<nsd_, nsd_> iJ0(jac0);
  iJ0.invert();

  // separate "stretch"-part of J-mapping between parameter and global space
  // (G0^ji) = (Jinv0^j_B) (krondelta^BA) (Jinv0_A^i)
  Core::LinAlg::Matrix<nsd_, nsd_> jac0stretch;
  jac0stretch.multiply_tn(iJ0, iJ0);
  const double r_stretch = sqrt(jac0stretch(0, 0));
  const double s_stretch = sqrt(jac0stretch(1, 1));
  const double t_stretch = sqrt(jac0stretch(2, 2));

  // minimal stretch equivalents with "thinnest" direction
  // const double max_stretch = max(r_stretch, max(s_stretch, t_stretch));
  double max_stretch = -1.0;

  ThicknessDirection thickdir = none;  // of actual element
  int thick_index = -1;

  if (r_stretch >= s_stretch and r_stretch >= t_stretch)
  {
    max_stretch = r_stretch;
    if ((max_stretch / s_stretch <= 1.5) || (max_stretch / t_stretch <= 1.5))
    {
      // std::cout << "ID: " << this->Id() << ", has aspect ratio of: ";
      // std::cout << max_stretch / s_stretch << " , " << max_stretch / t_stretch << std::endl;
      // FOUR_C_THROW("Solid-Shell element geometry has not a shell aspect ratio");
      return undefined;
    }
    thickdir = autor;
    thick_index = 0;
  }
  else if (s_stretch > r_stretch and s_stretch >= t_stretch)
  {
    max_stretch = s_stretch;
    if ((max_stretch / r_stretch <= 1.5) || (max_stretch / t_stretch <= 1.5))
    {
      // std::cout << "ID: " << this->Id() << ", has aspect ratio of: ";
      // std::cout << max_stretch / r_stretch << " , " << max_stretch / t_stretch << std::endl;
      // FOUR_C_THROW("Solid-Shell element geometry has not a shell aspect ratio");
      return undefined;
    }
    thickdir = autos;
    thick_index = 1;
  }
  else if (t_stretch > r_stretch and t_stretch > s_stretch)
  {
    max_stretch = t_stretch;
    if ((max_stretch / r_stretch <= 1.5) || (max_stretch / s_stretch <= 1.5))
    {
      // std::cout << "ID: " << this->Id() << ", has aspect ratio of: ";
      // std::cout << max_stretch / r_stretch << " , " << max_stretch / s_stretch << std::endl;
      // FOUR_C_THROW("Solid-Shell element geometry has not a shell aspect ratio");
      return undefined;
    }
    thickdir = autot;
    thick_index = 2;
  }

  if (thick_index == -1)
    FOUR_C_THROW("Trouble with thick_index=%d %g,%g,%g,%g", thick_index, r_stretch, s_stretch,
        t_stretch, max_stretch);

  // thickness-vector in parameter-space, has 1.0 in thickness-coord
  Core::LinAlg::Matrix<nsd_, 1> loc_thickvec(true);
  loc_thickvec(thick_index) = 1.0;
  // thickness-vector in global coord is J times local thickness-vector
  // (X^A) = (J0_i^A)^T . (xi_i)
  Core::LinAlg::Matrix<nsd_, 1> glo_thickvec;
  glo_thickvec.multiply_tn(jac0, loc_thickvec);
  // return doubles of thickness-vector
  thickvec_.resize(3);
  thickvec_[0] = glo_thickvec(0);
  thickvec_[1] = glo_thickvec(1);
  thickvec_[2] = glo_thickvec(2);

  return thickdir;
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
Discret::Elements::SoSh8Plast::ThicknessDirection Discret::Elements::SoSh8Plast::enfthickdir(
    Core::LinAlg::Matrix<nsd_, 1>& thickdirglo)
{
  // update element geometry
  Core::LinAlg::Matrix<nen_, nsd_> xrefe(false);  // material coord. of element
  for (int i = 0; i < nen_; ++i)
  {
    xrefe(i, 0) = this->nodes()[i]->x()[0];
    xrefe(i, 1) = this->nodes()[i]->x()[1];
    xrefe(i, 2) = this->nodes()[i]->x()[2];
  }
  // vector of df(origin), ie parametric derivatives of shape functions
  // evaluated at the origin (r,s,t)=(0,0,0)
  const double df0_vector[numdofperelement_ * nen_] = {-0.125, -0.125, -0.125, +0.125, -0.125,
      -0.125, +0.125, +0.125, -0.125, -0.125, +0.125, -0.125, -0.125, -0.125, +0.125, +0.125,
      -0.125, +0.125, +0.125, +0.125, +0.125, -0.125, +0.125, +0.125};
  // shape function derivatives, evaluated at origin (r=s=t=0.0)
  Core::LinAlg::Matrix<nsd_, nen_> df0(df0_vector);

  // compute Jacobian, evaluated at element origin (r=s=t=0.0)
  // (J0_i^A) = (X^A_{,i})^T
  Core::LinAlg::Matrix<nsd_, nsd_> jac0(false);
  jac0.multiply_nn(df0, xrefe);

  // compute inverse of Jacobian at element origin
  // (Jinv0_A^i) = (X^A_{,i})^{-T}
  Core::LinAlg::Matrix<nsd_, nsd_> iJ0(jac0);
  iJ0.invert();

  // make enforced global thickness direction a unit vector
  const double thickdirglolength = thickdirglo.norm2();
  thickdirglo.scale(1.0 / thickdirglolength);

  // pull thickness direction from global to contra-variant local
  // (dxi^i) = (Jinv0_A^i)^T . (dX^A)
  Core::LinAlg::Matrix<nsd_, 1> thickdirlocsharp(false);
  thickdirlocsharp.multiply_tn(iJ0, thickdirglo);

  // identify parametric co-ordinate closest to enforced thickness direction
  int thick_index = -1;
  double thickdirlocmax = 0.0;
  for (int i = 0; i < nsd_; ++i)
  {
    if (fabs(thickdirlocsharp(i)) > thickdirlocmax)
    {
      thickdirlocmax = fabs(thickdirlocsharp(i));
      thick_index = i;
    }
  }
  const double tol = 0.9;  // should be larger than 1/sqrt(2)=0.707
  // check if parametric co-ordinate is clear
  if (thickdirlocmax < tol * thickdirlocsharp.norm2())
    FOUR_C_THROW(
        "could not clearly identify a parametric direction pointing along enforced thickness "
        "direction");

  ThicknessDirection thickdir = none;  // of actual element
  if (thick_index == 0)
  {
    thickdir = autor;
  }
  else if (thick_index == 1)
  {
    thickdir = autos;
  }
  else if (thick_index == 2)
  {
    thickdir = autot;
  }
  else
  {
    FOUR_C_THROW("Trouble with thick_index=%g", thick_index);
  }

  // thickness-vector in parameter-space, has 1.0 in thickness-coord
  Core::LinAlg::Matrix<nsd_, 1> loc_thickvec(true);
  loc_thickvec(thick_index) = 1.0;
  // thickness-vector in global coord is J times local thickness-vector
  // (X^A) = (J0_i^A)^T . (xi_i)
  Core::LinAlg::Matrix<nsd_, 1> glo_thickvec;
  glo_thickvec.multiply_tn(jac0, loc_thickvec);
  // return doubles of thickness-vector
  thickvec_.resize(3);
  thickvec_[0] = glo_thickvec(0);
  thickvec_[1] = glo_thickvec(1);
  thickvec_[2] = glo_thickvec(2);

  return thickdir;
}

/*----------------------------------------------------------------------*
 |  evaluate 'T'-transformation matrix )                       maf 05/07|
 *----------------------------------------------------------------------*/
void Discret::Elements::SoSh8Plast::evaluate_t(
    const Core::LinAlg::Matrix<nsd_, nsd_>& jac, Core::LinAlg::Matrix<numstr_, numstr_>& TinvT)
{
  // build T^T transformation matrix which maps
  // between global (r,s,t)-coordinates and local (x,y,z)-coords
  // later, invert the transposed to map from local to global
  // see literature for details (e.g. Andelfinger)
  // it is based on the voigt notation for strains: xx,yy,zz,xy,yz,xz
  TinvT(0, 0) = jac(0, 0) * jac(0, 0);
  TinvT(1, 0) = jac(1, 0) * jac(1, 0);
  TinvT(2, 0) = jac(2, 0) * jac(2, 0);
  TinvT(3, 0) = 2 * jac(0, 0) * jac(1, 0);
  TinvT(4, 0) = 2 * jac(1, 0) * jac(2, 0);
  TinvT(5, 0) = 2 * jac(0, 0) * jac(2, 0);

  TinvT(0, 1) = jac(0, 1) * jac(0, 1);
  TinvT(1, 1) = jac(1, 1) * jac(1, 1);
  TinvT(2, 1) = jac(2, 1) * jac(2, 1);
  TinvT(3, 1) = 2 * jac(0, 1) * jac(1, 1);
  TinvT(4, 1) = 2 * jac(1, 1) * jac(2, 1);
  TinvT(5, 1) = 2 * jac(0, 1) * jac(2, 1);

  TinvT(0, 2) = jac(0, 2) * jac(0, 2);
  TinvT(1, 2) = jac(1, 2) * jac(1, 2);
  TinvT(2, 2) = jac(2, 2) * jac(2, 2);
  TinvT(3, 2) = 2 * jac(0, 2) * jac(1, 2);
  TinvT(4, 2) = 2 * jac(1, 2) * jac(2, 2);
  TinvT(5, 2) = 2 * jac(0, 2) * jac(2, 2);

  TinvT(0, 3) = jac(0, 0) * jac(0, 1);
  TinvT(1, 3) = jac(1, 0) * jac(1, 1);
  TinvT(2, 3) = jac(2, 0) * jac(2, 1);
  TinvT(3, 3) = jac(0, 0) * jac(1, 1) + jac(1, 0) * jac(0, 1);
  TinvT(4, 3) = jac(1, 0) * jac(2, 1) + jac(2, 0) * jac(1, 1);
  TinvT(5, 3) = jac(0, 0) * jac(2, 1) + jac(2, 0) * jac(0, 1);


  TinvT(0, 4) = jac(0, 1) * jac(0, 2);
  TinvT(1, 4) = jac(1, 1) * jac(1, 2);
  TinvT(2, 4) = jac(2, 1) * jac(2, 2);
  TinvT(3, 4) = jac(0, 1) * jac(1, 2) + jac(1, 1) * jac(0, 2);
  TinvT(4, 4) = jac(1, 1) * jac(2, 2) + jac(2, 1) * jac(1, 2);
  TinvT(5, 4) = jac(0, 1) * jac(2, 2) + jac(2, 1) * jac(0, 2);

  TinvT(0, 5) = jac(0, 0) * jac(0, 2);
  TinvT(1, 5) = jac(1, 0) * jac(1, 2);
  TinvT(2, 5) = jac(2, 0) * jac(2, 2);
  TinvT(3, 5) = jac(0, 0) * jac(1, 2) + jac(1, 0) * jac(0, 2);
  TinvT(4, 5) = jac(1, 0) * jac(2, 2) + jac(2, 0) * jac(1, 2);
  TinvT(5, 5) = jac(0, 0) * jac(2, 2) + jac(2, 0) * jac(0, 2);

  // now evaluate T^{-T} with solver
  Core::LinAlg::FixedSizeSerialDenseSolver<numstr_, numstr_, 1> solve_for_inverseT;
  solve_for_inverseT.set_matrix(TinvT);
  int err2 = solve_for_inverseT.factor();
  int err = solve_for_inverseT.invert();
  if ((err != 0) && (err2 != 0)) FOUR_C_THROW("Inversion of Tinv (Jacobian) failed");
  return;
}


/*----------------------------------------------------------------------*
 |  setup of constant ANS data (private)                       maf 05/07|
 *----------------------------------------------------------------------*/
void Discret::Elements::SoSh8Plast::anssetup(
    const Core::LinAlg::Matrix<nen_, nsd_>& xrefe,  // material element coords
    const Core::LinAlg::Matrix<nen_, nsd_>& xcurr,  // current element coords
    std::vector<Core::LinAlg::Matrix<nsd_, nen_>>**
        deriv_sp,                                            // derivs eval. at all sampling points
    std::vector<Core::LinAlg::Matrix<nsd_, nsd_>>& jac_sps,  // jac at all sampling points
    std::vector<Core::LinAlg::Matrix<nsd_, nsd_>>&
        jac_cur_sps,  // current jac at all sampling points
    Core::LinAlg::Matrix<num_ans * num_sp, numdofperelement_>& B_ans_loc)  // modified B
{
  // static matrix object of derivs at sampling points, kept in memory
  static std::vector<Core::LinAlg::Matrix<nsd_, nen_>> df_sp(num_sp);
  static bool dfsp_eval;  // flag for re-evaluate everything

  if (dfsp_eval != 0)
  {                      // if true f,df already evaluated
    *deriv_sp = &df_sp;  // return adress of static object to target of pointer
  }
  else
  {
    /*====================================================================*/
    /* 8-node hexhedra Solid-Shell node topology
     * and location of sampling points A to H                             */
    /*--------------------------------------------------------------------*/
    /*                      t
     *                      |
     *             4========|================7
     *          // |        |              //||
     *        //   |        |            //  ||
     *      //     |        |   D      //    ||
     *     5=======E=================6       H
     *    ||       |        |        ||      ||
     *    ||   A   |        o--------||-- C -------s
     *    ||       |       /         ||      ||
     *    F        0----- B ---------G ------3
     *    ||     //     /            ||    //
     *    ||   //     /              ||  //
     *    || //     r                ||//
     *     1=========================2
     *
     */
    /*====================================================================*/
    // (r,s,t) locations of sampling points A,B,C,D,E,F,G,H
    // numsp = 8 here set explicitly to allow direct initializing
    //                A,   B,   C,   D,   E,   F,   G,   H
    std::array<double, 8> r = {0.0, 1.0, 0.0, -1.0, -1.0, 1.0, 1.0, -1.0};
    std::array<double, 8> s = {-1.0, 0.0, 1.0, 0.0, -1.0, -1.0, 1.0, 1.0};
    std::array<double, 8> t = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

    // fill up df_sp w.r.t. rst directions (NUMDIM) at each sp
    for (int i = 0; i < num_sp; ++i)
    {
      // df wrt to r "+0" for each node(0..7) at each sp [i]
      df_sp[i](0, 0) = -(1.0 - s[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](0, 1) = (1.0 - s[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](0, 2) = (1.0 + s[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](0, 3) = -(1.0 + s[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](0, 4) = -(1.0 - s[i]) * (1.0 + t[i]) * 0.125;
      df_sp[i](0, 5) = (1.0 - s[i]) * (1.0 + t[i]) * 0.125;
      df_sp[i](0, 6) = (1.0 + s[i]) * (1.0 + t[i]) * 0.125;
      df_sp[i](0, 7) = -(1.0 + s[i]) * (1.0 + t[i]) * 0.125;

      // df wrt to s "+1" for each node(0..7) at each sp [i]
      df_sp[i](1, 0) = -(1.0 - r[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](1, 1) = -(1.0 + r[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](1, 2) = (1.0 + r[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](1, 3) = (1.0 - r[i]) * (1.0 - t[i]) * 0.125;
      df_sp[i](1, 4) = -(1.0 - r[i]) * (1.0 + t[i]) * 0.125;
      df_sp[i](1, 5) = -(1.0 + r[i]) * (1.0 + t[i]) * 0.125;
      df_sp[i](1, 6) = (1.0 + r[i]) * (1.0 + t[i]) * 0.125;
      df_sp[i](1, 7) = (1.0 - r[i]) * (1.0 + t[i]) * 0.125;

      // df wrt to t "+2" for each node(0..7) at each sp [i]
      df_sp[i](2, 0) = -(1.0 - r[i]) * (1.0 - s[i]) * 0.125;
      df_sp[i](2, 1) = -(1.0 + r[i]) * (1.0 - s[i]) * 0.125;
      df_sp[i](2, 2) = -(1.0 + r[i]) * (1.0 + s[i]) * 0.125;
      df_sp[i](2, 3) = -(1.0 - r[i]) * (1.0 + s[i]) * 0.125;
      df_sp[i](2, 4) = (1.0 - r[i]) * (1.0 - s[i]) * 0.125;
      df_sp[i](2, 5) = (1.0 + r[i]) * (1.0 - s[i]) * 0.125;
      df_sp[i](2, 6) = (1.0 + r[i]) * (1.0 + s[i]) * 0.125;
      df_sp[i](2, 7) = (1.0 - r[i]) * (1.0 + s[i]) * 0.125;
    }

    // return adresses of just evaluated matrices
    *deriv_sp = &df_sp;  // return adress of static object to target of pointer
    dfsp_eval = true;    // now all arrays are filled statically
  }

  for (int sp = 0; sp < num_sp; ++sp)
  {
    // compute (REFERENCE) Jacobian matrix at all sampling points
    jac_sps[sp].multiply(df_sp[sp], xrefe);
    // compute CURRENT Jacobian matrix at all sampling points
    jac_cur_sps[sp].multiply(df_sp[sp], xcurr);
  }

  /*
  ** Compute modified B-operator in local(parametric) space,
  ** evaluated at all sampling points
  */
  // loop over each sampling point
  Core::LinAlg::Matrix<nsd_, nsd_> jac_cur;
  for (int sp = 0; sp < num_sp; ++sp)
  {
    /* compute the CURRENT Jacobian matrix at the sampling point:
    **         [ xcurr_,r  ycurr_,r  zcurr_,r ]
    **  Jcur = [ xcurr_,s  ycurr_,s  zcurr_,s ]
    **         [ xcurr_,t  ycurr_,t  zcurr_,t ]
    ** Used to transform the global displacements into parametric space
    */
    jac_cur.multiply(df_sp[sp], xcurr);

    // fill up B-operator
    for (int inode = 0; inode < nen_; ++inode)
    {
      for (int dim = 0; dim < nsd_; ++dim)
      {
        // modify B_loc_tt = N_t.X_t
        B_ans_loc(sp * num_ans + 0, inode * 3 + dim) = df_sp[sp](2, inode) * jac_cur(2, dim);
        // modify B_loc_st = N_s.X_t + N_t.X_s
        B_ans_loc(sp * num_ans + 1, inode * 3 + dim) =
            df_sp[sp](1, inode) * jac_cur(2, dim) + df_sp[sp](2, inode) * jac_cur(1, dim);
        // modify B_loc_rt = N_r.X_t + N_t.X_r
        B_ans_loc(sp * num_ans + 2, inode * 3 + dim) =
            df_sp[sp](0, inode) * jac_cur(2, dim) + df_sp[sp](2, inode) * jac_cur(0, dim);
      }
    }
  }


  return;
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
void Discret::Elements::SoSh8Plast::re_init_eas(const Discret::Elements::So3PlastEasType EASType)
{
  neas_ = Discret::Elements::plast_eas_type_to_num_eas_v(EASType);
  eastype_ = EASType;

  if (eastype_ != soh8p_easnone)
  {
    KaaInv_ = std::make_shared<Core::LinAlg::SerialDenseMatrix>(neas_, neas_, true);
    Kad_ = std::make_shared<Core::LinAlg::SerialDenseMatrix>(neas_, numdofperelement_, true);
    feas_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    alpha_eas_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    alpha_eas_last_timestep_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    alpha_eas_delta_over_last_timestep_ =
        std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    alpha_eas_inc_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    Kba_ = std::make_shared<std::vector<Core::LinAlg::SerialDenseMatrix>>(
        numgpt_, Core::LinAlg::SerialDenseMatrix(5, neas_, true));
  }

  return;
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
void Discret::Elements::SoSh8Plast::nln_stiffmass(
    std::vector<double>& disp,  // current displacements
    std::vector<double>& vel,   // current velocities
    std::vector<double>& temp,  // current temperatures
    Core::LinAlg::Matrix<numdofperelement_, numdofperelement_>*
        stiffmatrix,  // element stiffness matrix
    Core::LinAlg::Matrix<numdofperelement_, numdofperelement_>* massmatrix,  // element mass matrix
    Core::LinAlg::Matrix<numdofperelement_, 1>* force,      // element internal force vector
    Core::LinAlg::Matrix<numgpt_post, numstr_>* elestress,  // stresses at GP
    Core::LinAlg::Matrix<numgpt_post, numstr_>* elestrain,  // strains at GP
    Teuchos::ParameterList& params,                         // algorithmic parameters e.g. time
    const Inpar::Solid::StressType iostress,                // stress output option
    const Inpar::Solid::StrainType iostrain                 // strain output option
)
{
  invalid_ele_data();
  const bool is_tangDis = str_params_interface().get_predictor_type() == Inpar::Solid::pred_tangdis;

  fill_position_arrays(disp, vel, temp);

  // get plastic hyperelastic material
  Mat::PlasticElastHyper* plmat = nullptr;
  if (material()->material_type() == Core::Materials::m_plelasthyper)
    plmat = dynamic_cast<Mat::PlasticElastHyper*>(material().get());
  else
    FOUR_C_THROW("so3_ssn_plast elements only with PlasticElastHyper material");

  if (eastype_ != soh8p_easnone)
  {
    evaluate_center();
    eas_setup();
  }

  // EAS matrix block
  Core::LinAlg::SerialDenseMatrix Kda(numdofperelement_, neas_);

  // ANS modified rows of bop in local(parameter) coords
  Core::LinAlg::Matrix<num_ans * num_sp, numdofperelement_> B_ans_loc;
  // Jacobian evaluated at all ANS sampling points
  std::vector<Core::LinAlg::Matrix<nsd_, nsd_>> jac_sps(num_sp);
  // CURRENT Jacobian evaluated at all ANS sampling points
  std::vector<Core::LinAlg::Matrix<nsd_, nsd_>> jac_cur_sps(num_sp);
  // pointer to derivs evaluated at all sampling points
  std::vector<Core::LinAlg::Matrix<nsd_, nen_>>* deriv_sp =
      nullptr;  // derivs eval. at all sampling points
  // evaluate all necessary variables for ANS
  anssetup(xrefe(), xcurr(), &deriv_sp, jac_sps, jac_cur_sps, set_b_ans_loc());

  /* =========================================================================*/
  /* ================================================= Loop over Gauss Points */
  /* =========================================================================*/
  for (int gp = 0; gp < numgpt_; ++gp)
  {
    invalid_gp_data();
    // shape functions (shapefunct) and their first derivatives (deriv)
    Core::FE::shape_function<Core::FE::CellType::hex8>(xsi_[gp], set_shape_function());
    Core::FE::shape_function_deriv1<Core::FE::CellType::hex8>(xsi_[gp], set_deriv_shape_function());

    kinematics(gp);

    /* get the inverse of the Jacobian matrix which looks like:
     **            [ x_,r  y_,r  z_,r ]^-1
     **     J^-1 = [ x_,s  y_,s  z_,s ]
     **            [ x_,t  y_,t  z_,t ]
     */
    // compute derivatives N_XYZ at gp w.r.t. material coordinates
    // by N_XYZ = J^-1 * N_rst
    set_deriv_shape_function_xyz().multiply(inv_j(), deriv_shape_function());  // (6.21)

    ans_strains(gp, jac_sps, jac_cur_sps);
    if (eastype_ != soh8p_easnone)
    {
      eas_shape(gp);
      eas_enhance_strains();
    }

    // strain output *********************************
    output_strains(gp, iostrain, elestress);

    // material call *********************************************
    plmat->evaluate_elast(&defgrd_mod(), &delta_lp(), &set_p_k2(), &set_cmat(), gp, id());
    // material call *********************************************

    // return gp stresses
    output_stress(gp, iostress, elestress);

    // integrate usual internal force and stiffness matrix
    double detJ_w = det_j() * wgt_[gp];
    // integrate elastic internal force vector **************************
    // update internal force vector
    if (force != nullptr) integrate_force(gp, *force);

    // update stiffness matrix
    if (stiffmatrix != nullptr)
    {
      // integrate `elastic' and `initial-displacement' stiffness matrix
      // keu = keu + (B^T . C . B) * detJ * w(gp)
      Core::LinAlg::Matrix<numstr_, numdofperelement_> cb;
      cb.multiply(cmat(), bop());
      stiffmatrix->multiply_tn(detJ_w, bop(), cb, 1.0);

      // intergrate `geometric' stiffness matrix and add to keu *****************
      // here also the ANS interpolation comes into play
      double r = xsi_.at(gp)(0);
      double s = xsi_.at(gp)(1);
      for (int inod = 0; inod < nen_; ++inod)
      {
        for (int jnod = 0; jnod < nen_; ++jnod)
        {
          Core::LinAlg::Matrix<numstr_, 1> G_ij;
          G_ij(0) = deriv_shape_function()(0, inod) * deriv_shape_function()(0, jnod);  // rr-dir
          G_ij(1) = deriv_shape_function()(1, inod) * deriv_shape_function()(1, jnod);  // ss-dir
          G_ij(3) = deriv_shape_function()(0, inod) * deriv_shape_function()(1, jnod) +
                    deriv_shape_function()(1, inod) * deriv_shape_function()(0, jnod);  // rs-dir

          // do the ANS related stuff if wanted!
          if (anstype_ == anssosh8_p)
          {
            // ANS modification in tt-dir
            G_ij(2) = 0.25 * (1 - r) * (1 - s) * (*deriv_sp)[4](2, inod) * (*deriv_sp)[4](2, jnod) +
                      0.25 * (1 + r) * (1 - s) * (*deriv_sp)[5](2, inod) * (*deriv_sp)[5](2, jnod) +
                      0.25 * (1 + r) * (1 + s) * (*deriv_sp)[6](2, inod) * (*deriv_sp)[6](2, jnod) +
                      0.25 * (1 - r) * (1 + s) * (*deriv_sp)[7](2, inod) * (*deriv_sp)[7](2, jnod);
            // ANS modification in st-dir
            G_ij(4) = 0.5 * ((1 + r) * ((*deriv_sp)[1](1, inod) * (*deriv_sp)[1](2, jnod) +
                                           (*deriv_sp)[1](2, inod) * (*deriv_sp)[1](1, jnod)) +
                                (1 - r) * ((*deriv_sp)[3](1, inod) * (*deriv_sp)[3](2, jnod) +
                                              (*deriv_sp)[3](2, inod) * (*deriv_sp)[3](1, jnod)));
            // ANS modification in rt-dir
            G_ij(5) = 0.5 * ((1 - s) * ((*deriv_sp)[0](0, inod) * (*deriv_sp)[0](2, jnod) +
                                           (*deriv_sp)[0](2, inod) * (*deriv_sp)[0](0, jnod)) +
                                (1 + s) * ((*deriv_sp)[2](0, inod) * (*deriv_sp)[2](2, jnod) +
                                              (*deriv_sp)[2](2, inod) * (*deriv_sp)[2](0, jnod)));
          }
          else if (anstype_ == ansnone_p)
          {
            G_ij(2) = deriv_shape_function()(2, inod) * deriv_shape_function()(2, jnod);  // tt-dir
            G_ij(4) = deriv_shape_function()(2, inod) * deriv_shape_function()(1, jnod) +
                      deriv_shape_function()(1, inod) * deriv_shape_function()(2, jnod);  // st-dir
            G_ij(5) = deriv_shape_function()(0, inod) * deriv_shape_function()(2, jnod) +
                      deriv_shape_function()(2, inod) * deriv_shape_function()(0, jnod);  // rt-dir
          }
          else
            FOUR_C_THROW("Cannot build geometric stiffness matrix on your ANS-choice!");

          // transformation of local(parameter) space 'back' to global(material) space
          Core::LinAlg::Matrix<Mat::NUM_STRESS_3D, 1> G_ij_glob;
          G_ij_glob.multiply(tinv_t(), G_ij);

          // Scalar Gij results from product of G_ij with stress, scaled with detJ*weights
          const double Gij = detJ_w * p_k2().dot(G_ij_glob);

          // add "geometric part" Gij times detJ*weights to stiffness matrix
          (*stiffmatrix)(nsd_ * inod + 0, nsd_ * jnod + 0) += Gij;
          (*stiffmatrix)(nsd_ * inod + 1, nsd_ * jnod + 1) += Gij;
          (*stiffmatrix)(nsd_ * inod + 2, nsd_ * jnod + 2) += Gij;
        }
      }  // end of intergrate `geometric' stiffness ******************************

      // EAS technology: integrate matrices --------------------------------- EAS
      if (eastype_ != soh8p_easnone)
      {
        // integrate Kaa: Kaa += (M^T . cmat . M) * detJ * w(gp)
        // integrate Kda: Kad += (M^T . cmat . B) * detJ * w(gp)
        // integrate feas: feas += (M^T . sigma) * detJ *wp(gp)
        Core::LinAlg::SerialDenseMatrix cM(numstr_, neas_, true);  // temporary c . M
        switch (eastype_)
        {
          case soh8p_eassosh8:
            Core::LinAlg::DenseFunctions::multiply<double, numstr_, numstr_,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas>(
                cM.values(), cmat().data(), m_eas().values());
            Core::LinAlg::DenseFunctions::multiply_tn<double,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas, numstr_,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas>(
                1.0, KaaInv_->values(), detJ_w, m_eas().values(), cM.values());
            Core::LinAlg::DenseFunctions::multiply_tn<double,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas, numstr_,
                numdofperelement_>(1.0, Kad_->values(), detJ_w, m_eas().values(), cb.data());
            Core::LinAlg::DenseFunctions::multiply_tn<double, numdofperelement_, numstr_,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas>(
                1.0, Kda.values(), detJ_w, cb.data(), m_eas().values());
            Core::LinAlg::DenseFunctions::multiply_tn<double,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas, numstr_, 1>(
                1.0, feas_->values(), detJ_w, m_eas().values(), p_k2().data());
            break;
          case soh8p_easmild:
            Core::LinAlg::DenseFunctions::multiply<double, numstr_, numstr_,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas>(
                cM.values(), cmat().data(), m_eas().values());
            Core::LinAlg::DenseFunctions::multiply_tn<double,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas, numstr_,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas>(
                1.0, KaaInv_->values(), detJ_w, m_eas().values(), cM.values());
            Core::LinAlg::DenseFunctions::multiply_tn<double,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas, numstr_,
                numdofperelement_>(1.0, Kad_->values(), detJ_w, m_eas().values(), cb.data());
            Core::LinAlg::DenseFunctions::multiply_tn<double, numdofperelement_, numstr_,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas>(
                1.0, Kda.values(), detJ_w, cb.data(), m_eas().values());
            Core::LinAlg::DenseFunctions::multiply_tn<double,
                PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas, numstr_, 1>(
                1.0, feas_->values(), detJ_w, m_eas().values(), p_k2().data());
            break;
          case soh8p_easnone:
            break;
          default:
            FOUR_C_THROW("Don't know what to do with EAS type %d", eastype_);
            break;
        }
      }  // ---------------------------------------------------------------- EAS
    }    // end of stiffness matrix

    if (massmatrix != nullptr)  // evaluate mass matrix +++++++++++++++++++++++++
      integrate_mass_matrix(gp, *massmatrix);

    // plastic modifications
    if ((stiffmatrix != nullptr || force != nullptr))
    {
      if (have_plastic_spin())
      {
        if (eastype_ != soh8p_easnone)
          condense_plasticity<plspin>(defgrd_mod(), delta_lp(), bop(), &deriv_shape_function_xyz(),
              nullptr, detJ_w, gp, 0, params, force, stiffmatrix, &m_eas(), &Kda);
        else
          condense_plasticity<plspin>(defgrd_mod(), delta_lp(), bop(), &deriv_shape_function_xyz(),
              nullptr, detJ_w, gp, 0, params, force, stiffmatrix);
      }
      else
      {
        if (eastype_ != soh8p_easnone)
          condense_plasticity<zerospin>(defgrd_mod(), delta_lp(), bop(),
              &deriv_shape_function_xyz(), nullptr, detJ_w, gp, 0, params, force, stiffmatrix,
              &m_eas(), &Kda);
        else
          condense_plasticity<zerospin>(defgrd_mod(), delta_lp(), bop(),
              &deriv_shape_function_xyz(), nullptr, detJ_w, gp, 0, params, force, stiffmatrix);
      }
    }  // plastic modifications
  }    // gp loop

  // Static condensation EAS --> stiff ********************************
  if (stiffmatrix != nullptr && !is_tangDis && eastype_ != soh8p_easnone)
  {
    using ordinalType = Core::LinAlg::SerialDenseMatrix::ordinalType;
    using scalarType = Core::LinAlg::SerialDenseMatrix::scalarType;
    Teuchos::SerialDenseSolver<ordinalType, scalarType> solve_for_inverseKaa;
    solve_for_inverseKaa.setMatrix(Teuchos::rcpFromRef(*KaaInv_));
    solve_for_inverseKaa.invert();

    Core::LinAlg::SerialDenseMatrix kdakaai(numdofperelement_, neas_);
    switch (eastype_)
    {
      case soh8p_eassosh8:
        Core::LinAlg::DenseFunctions::multiply<double, numdofperelement_,
            PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas,
            PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas>(
            0., kdakaai.values(), 1., Kda.values(), KaaInv_->values());
        if (stiffmatrix != nullptr)
          Core::LinAlg::DenseFunctions::multiply<double, numdofperelement_,
              PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas, numdofperelement_>(
              1., stiffmatrix->data(), -1., kdakaai.values(), Kad_->values());
        if (force != nullptr)
          Core::LinAlg::DenseFunctions::multiply<double, numdofperelement_,
              PlastEasTypeToNumEas<Discret::Elements::soh8p_eassosh8>::neas, 1>(
              1., force->data(), -1., kdakaai.values(), feas_->values());
        break;
      case soh8p_easmild:
        Core::LinAlg::DenseFunctions::multiply<double, numdofperelement_,
            PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas,
            PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas>(
            0., kdakaai.values(), 1., Kda.values(), KaaInv_->values());
        if (stiffmatrix != nullptr)
          Core::LinAlg::DenseFunctions::multiply<double, numdofperelement_,
              PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas, numdofperelement_>(
              1., stiffmatrix->data(), -1., kdakaai.values(), Kad_->values());
        if (force != nullptr)
          Core::LinAlg::DenseFunctions::multiply<double, numdofperelement_,
              PlastEasTypeToNumEas<Discret::Elements::soh8p_easmild>::neas, 1>(
              1., force->data(), -1., kdakaai.values(), feas_->values());
        break;
      case soh8p_easnone:
        break;
      default:
        FOUR_C_THROW("Don't know what to do with EAS type %d", eastype_);
        break;
    }
  }


  return;
}


/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
void Discret::Elements::SoSh8Plast::calculate_bop(
    Core::LinAlg::Matrix<numstr_, numdofperelement_>* bop,
    const Core::LinAlg::Matrix<nsd_, nsd_>* defgrd, const Core::LinAlg::Matrix<nsd_, nen_>* N_XYZ,
    const int gp)
{
  set_jac_refe().multiply(deriv_shape_function(), xrefe());
  set_jac_curr().multiply(deriv_shape_function(), xcurr());

  if (gp < 0 || gp > 7) FOUR_C_THROW("invalid gp number");

  // set up B-Operator in local(parameter) element space including ANS
  Core::LinAlg::Matrix<numstr_, numdofperelement_> bop_loc;
  for (int inode = 0; inode < NUMNOD_SOH8; ++inode)
  {
    for (int dim = 0; dim < NUMDIM_SOH8; ++dim)
    {
      // B_loc_rr = N_r.X_r
      bop_loc(0, inode * 3 + dim) = deriv_shape_function()(0, inode) * jac_curr()(0, dim);
      // B_loc_ss = N_s.X_s
      bop_loc(1, inode * 3 + dim) = deriv_shape_function()(1, inode) * jac_curr()(1, dim);
      // B_loc_rs = N_r.X_s + N_s.X_r
      bop_loc(3, inode * 3 + dim) = deriv_shape_function()(0, inode) * jac_curr()(1, dim) +
                                    deriv_shape_function()(1, inode) * jac_curr()(0, dim);

      // do the ANS related stuff
      if (anstype_ == anssosh8_p)
      {
        double r = xsi_.at(gp)(0);
        double s = xsi_.at(gp)(1);
        // B_loc_tt = interpolation along (r x s) of ANS B_loc_tt
        //          = (1-r)(1-s)/4 * B_ans(SP E) + (1+r)(1-s)/4 * B_ans(SP F)
        //           +(1+r)(1+s)/4 * B_ans(SP G) + (1-r)(1+s)/4 * B_ans(SP H)
        bop_loc(2, inode * 3 + dim) =
            0.25 * (1 - r) * (1 - s) * b_ans_loc()(0 + 4 * num_ans, inode * 3 + dim)     // E
            + 0.25 * (1 + r) * (1 - s) * b_ans_loc()(0 + 5 * num_ans, inode * 3 + dim)   // F
            + 0.25 * (1 + r) * (1 + s) * b_ans_loc()(0 + 6 * num_ans, inode * 3 + dim)   // G
            + 0.25 * (1 - r) * (1 + s) * b_ans_loc()(0 + 7 * num_ans, inode * 3 + dim);  // H
        // B_loc_st = interpolation along r of ANS B_loc_st
        //          = (1+r)/2 * B_ans(SP B) + (1-r)/2 * B_ans(SP D)
        bop_loc(4, inode * 3 + dim) =
            0.5 * (1.0 + r) * b_ans_loc()(1 + 1 * num_ans, inode * 3 + dim)     // B
            + 0.5 * (1.0 - r) * b_ans_loc()(1 + 3 * num_ans, inode * 3 + dim);  // D

        // B_loc_rt = interpolation along s of ANS B_loc_rt
        //          = (1-s)/2 * B_ans(SP A) + (1+s)/2 * B_ans(SP C)
        bop_loc(5, inode * 3 + dim) =
            0.5 * (1.0 - s) * b_ans_loc()(2 + 0 * num_ans, inode * 3 + dim)     // A
            + 0.5 * (1.0 + s) * b_ans_loc()(2 + 2 * num_ans, inode * 3 + dim);  // C
      }
      else if (anstype_ == ansnone_p)
      {
        // B_loc_tt = N_t.X_t
        bop_loc(2, inode * 3 + dim) = deriv_shape_function()(2, inode) * jac_curr()(2, dim);
        // B_loc_st = N_t.X_s + N_s.X_t
        bop_loc(4, inode * 3 + dim) = deriv_shape_function()(2, inode) * jac_curr()(1, dim) +
                                      deriv_shape_function()(1, inode) * jac_curr()(2, dim);

        // B_loc_rt = N_r.X_t + N_t.X_r
        bop_loc(5, inode * 3 + dim) = deriv_shape_function()(0, inode) * jac_curr()(2, dim) +
                                      deriv_shape_function()(2, inode) * jac_curr()(0, dim);
      }
      else
        FOUR_C_THROW("Cannot build bop_loc based on your ANS-choice!");
    }
  }

  // transformation from local (parameter) element space to global(material) space
  // with famous 'T'-matrix already used for EAS but now evaluated at each gp
  evaluate_t(jac_refe(), set_tinv_t());
  set_bop().multiply(tinv_t(), bop_loc);
}


void Discret::Elements::SoSh8Plast::ans_strains(const int gp,
    std::vector<Core::LinAlg::Matrix<nsd_, nsd_>>& jac_sps,  // jac at all sampling points
    std::vector<Core::LinAlg::Matrix<nsd_, nsd_>>&
        jac_cur_sps  // current jac at all sampling points
)
{
  // local GL strain vector lstrain={E11,E22,E33,2*E12,2*E23,2*E31}
  Core::LinAlg::Matrix<numstr_, 1> lstrain;
  // evaluate glstrains in local(parameter) coords
  // Err = 0.5 * (dx/dr * dx/dr^T - dX/dr * dX/dr^T)
  lstrain(0) =
      0.5 * (+(jac_curr()(0, 0) * jac_curr()(0, 0) + jac_curr()(0, 1) * jac_curr()(0, 1) +
                 jac_curr()(0, 2) * jac_curr()(0, 2)) -
                (jac_refe()(0, 0) * jac_refe()(0, 0) + jac_refe()(0, 1) * jac_refe()(0, 1) +
                    jac_refe()(0, 2) * jac_refe()(0, 2)));
  // Ess = 0.5 * (dy/ds * dy/ds^T - dY/ds * dY/ds^T)
  lstrain(1) =
      0.5 * (+(jac_curr()(1, 0) * jac_curr()(1, 0) + jac_curr()(1, 1) * jac_curr()(1, 1) +
                 jac_curr()(1, 2) * jac_curr()(1, 2)) -
                (jac_refe()(1, 0) * jac_refe()(1, 0) + jac_refe()(1, 1) * jac_refe()(1, 1) +
                    jac_refe()(1, 2) * jac_refe()(1, 2)));
  // Ers = (dx/ds * dy/dr^T - dX/ds * dY/dr^T)
  lstrain(3) = (+(jac_curr()(0, 0) * jac_curr()(1, 0) + jac_curr()(0, 1) * jac_curr()(1, 1) +
                    jac_curr()(0, 2) * jac_curr()(1, 2)) -
                (jac_refe()(0, 0) * jac_refe()(1, 0) + jac_refe()(0, 1) * jac_refe()(1, 1) +
                    jac_refe()(0, 2) * jac_refe()(1, 2)));


  // do the ANS related stuff if wanted!
  if (anstype_ == anssosh8_p)
  {
    double r = xsi_.at(gp)(0);
    double s = xsi_.at(gp)(1);
    // ANS modification of strains ************************************** ANS
    double dxdt_A = 0.0;
    double dXdt_A = 0.0;
    double dydt_B = 0.0;
    double dYdt_B = 0.0;
    double dxdt_C = 0.0;
    double dXdt_C = 0.0;
    double dydt_D = 0.0;
    double dYdt_D = 0.0;

    double dzdt_E = 0.0;
    double dZdt_E = 0.0;
    double dzdt_F = 0.0;
    double dZdt_F = 0.0;
    double dzdt_G = 0.0;
    double dZdt_G = 0.0;
    double dzdt_H = 0.0;
    double dZdt_H = 0.0;

    // vector product of rows of jacobians at corresponding sampling point    std::cout <<
    // jac_cur_sps;
    for (int dim = 0; dim < nsd_; ++dim)
    {
      dxdt_A += jac_cur_sps[0](0, dim) * jac_cur_sps[0](2, dim);  // g_13^A
      dXdt_A += jac_sps[0](0, dim) * jac_sps[0](2, dim);          // G_13^A
      dydt_B += jac_cur_sps[1](1, dim) * jac_cur_sps[1](2, dim);  // g_23^B
      dYdt_B += jac_sps[1](1, dim) * jac_sps[1](2, dim);          // G_23^B
      dxdt_C += jac_cur_sps[2](0, dim) * jac_cur_sps[2](2, dim);  // g_13^C
      dXdt_C += jac_sps[2](0, dim) * jac_sps[2](2, dim);          // G_13^C
      dydt_D += jac_cur_sps[3](1, dim) * jac_cur_sps[3](2, dim);  // g_23^D
      dYdt_D += jac_sps[3](1, dim) * jac_sps[3](2, dim);          // G_23^D

      dzdt_E += jac_cur_sps[4](2, dim) * jac_cur_sps[4](2, dim);
      dZdt_E += jac_sps[4](2, dim) * jac_sps[4](2, dim);
      dzdt_F += jac_cur_sps[5](2, dim) * jac_cur_sps[5](2, dim);
      dZdt_F += jac_sps[5](2, dim) * jac_sps[5](2, dim);
      dzdt_G += jac_cur_sps[6](2, dim) * jac_cur_sps[6](2, dim);
      dZdt_G += jac_sps[6](2, dim) * jac_sps[6](2, dim);
      dzdt_H += jac_cur_sps[7](2, dim) * jac_cur_sps[7](2, dim);
      dZdt_H += jac_sps[7](2, dim) * jac_sps[7](2, dim);
    }
    // E33: remedy of curvature thickness locking
    // Ett = 0.5* ( (1-r)(1-s)/4 * Ett(SP E) + ... + (1-r)(1+s)/4 * Ett(SP H) )
    lstrain(2) = 0.5 * (0.25 * (1 - r) * (1 - s) * (dzdt_E - dZdt_E) +
                           0.25 * (1 + r) * (1 - s) * (dzdt_F - dZdt_F) +
                           0.25 * (1 + r) * (1 + s) * (dzdt_G - dZdt_G) +
                           0.25 * (1 - r) * (1 + s) * (dzdt_H - dZdt_H));
    // E23: remedy of transverse shear locking
    // Est = (1+r)/2 * Est(SP B) + (1-r)/2 * Est(SP D)
    lstrain(4) = 0.5 * (1 + r) * (dydt_B - dYdt_B) + 0.5 * (1 - r) * (dydt_D - dYdt_D);
    // E13: remedy of transverse shear locking
    // Ert = (1-s)/2 * Ert(SP A) + (1+s)/2 * Ert(SP C)
    lstrain(5) = 0.5 * (1 - s) * (dxdt_A - dXdt_A) + 0.5 * (1 + s) * (dxdt_C - dXdt_C);
    // ANS modification of strains ************************************** ANS
  }
  else if (anstype_ == ansnone_p)
  {
    // No ANS!
    // Ett = 0.5 * (dz/dt * dz/dt^T - dZ/dt * dZ/dt^T)
    lstrain(2) =
        0.5 * (+(jac_curr()(2, 0) * jac_curr()(2, 0) + jac_curr()(2, 1) * jac_curr()(2, 1) +
                   jac_curr()(2, 2) * jac_curr()(2, 2)) -
                  (jac_refe()(2, 0) * jac_refe()(2, 0) + jac_refe()(2, 1) * jac_refe()(2, 1) +
                      jac_refe()(2, 2) * jac_refe()(2, 2)));
    // Est = (dz/ds * dy/dt^T - dZ/ds * dY/dt^T)
    lstrain(4) = (+(jac_curr()(2, 0) * jac_curr()(1, 0) + jac_curr()(2, 1) * jac_curr()(1, 1) +
                      jac_curr()(2, 2) * jac_curr()(1, 2)) -
                  (jac_refe()(2, 0) * jac_refe()(1, 0) + jac_refe()(2, 1) * jac_refe()(1, 1) +
                      jac_refe()(2, 2) * jac_refe()(1, 2)));
    // Est = (dz/dr * dx/dt^T - dZ/dr * dX/dt^T)
    lstrain(5) = (+(jac_curr()(2, 0) * jac_curr()(0, 0) + jac_curr()(2, 1) * jac_curr()(0, 1) +
                      jac_curr()(2, 2) * jac_curr()(0, 2)) -
                  (jac_refe()(2, 0) * jac_refe()(0, 0) + jac_refe()(2, 1) * jac_refe()(0, 1) +
                      jac_refe()(2, 2) * jac_refe()(0, 2)));
  }
  else
    FOUR_C_THROW("Cannot build local strains based on your ANS-choice!");

  // transformation of local glstrains 'back' to global(material) space
  static Core::LinAlg::Matrix<numstr_, 1> glstrain(false);
  glstrain.multiply(tinv_t(), lstrain);

  for (int i = 0; i < nsd_; ++i) set_rcg()(i, i) = 2. * glstrain(i) + 1.;
  set_rcg()(0, 1) = set_rcg()(1, 0) = glstrain(3);
  set_rcg()(2, 1) = set_rcg()(1, 2) = glstrain(4);
  set_rcg()(0, 2) = set_rcg()(2, 0) = glstrain(5);

  // calculate deformation gradient consistent with modified GL strain tensor
  calc_consistent_defgrd();
}

FOUR_C_NAMESPACE_CLOSE
