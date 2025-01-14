// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_so3_plast_ssn.hpp"

#include "4C_comm_utils_factory.hpp"
#include "4C_global_data.hpp"
#include "4C_inpar_tsi.hpp"
#include "4C_linalg_serialdensevector.hpp"
#include "4C_mat_plasticelasthyper.hpp"
#include "4C_so3_line.hpp"
#include "4C_so3_surface.hpp"
#include "4C_thermo_ele_impl_utils.hpp"
#include "4C_utils_parameter_list.hpp"

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------*
 | ctor (public)                                            seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
Discret::Elements::So3Plast<distype>::So3Plast(int id, int owner)
    : SoBase(id, owner),
      fbar_(false),
      KbbInv_(std::vector<Core::LinAlg::SerialDenseMatrix>(0)),
      Kbd_(std::vector<Core::LinAlg::SerialDenseMatrix>(0)),
      fbeta_(std::vector<Core::LinAlg::SerialDenseVector>(0)),
      dDp_last_iter_(std::vector<Core::LinAlg::SerialDenseVector>(0)),
      dDp_inc_(std::vector<Core::LinAlg::SerialDenseVector>(0)),
      plspintype_(plspin),
      KaaInv_(nullptr),
      Kad_(nullptr),
      KaT_(nullptr),
      KdT_eas_(nullptr),
      feas_(nullptr),
      Kba_(nullptr),
      alpha_eas_(nullptr),
      alpha_eas_last_timestep_(nullptr),
      alpha_eas_delta_over_last_timestep_(nullptr),
      alpha_eas_inc_(nullptr),
      eastype_(soh8p_easnone),
      neas_(0),
      tsi_(false),
      is_nitsche_contact_(false)
{
}


/*----------------------------------------------------------------------*
 | copy-ctor (public)                                       seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
Discret::Elements::So3Plast<distype>::So3Plast(const Discret::Elements::So3Plast<distype>& old)
    : SoBase(old)
{
}


/*----------------------------------------------------------------------*
 | deep copy this instance of Solid3 and return pointer to  seitz 07/13 |
 | it (public)                                                          |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
Core::Elements::Element* Discret::Elements::So3Plast<distype>::clone() const
{
  auto* newelement = new Discret::Elements::So3Plast<distype>(*this);

  return newelement;
}


template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nen_, 1>>
    Discret::Elements::So3Plast<distype>::shapefunct_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nen_>>
    Discret::Elements::So3Plast<distype>::deriv_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::invJ_;
template <Core::FE::CellType distype>
std::pair<bool, double> Discret::Elements::So3Plast<distype>::detJ_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nen_>>
    Discret::Elements::So3Plast<distype>::N_XYZ_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::defgrd_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::defgrd_mod_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::rcg_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::delta_Lp_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::numstr_,
                    Discret::Elements::So3Plast<distype>::numdofperelement_>>
    Discret::Elements::So3Plast<distype>::bop_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::numstr_, 1>>
    Discret::Elements::So3Plast<distype>::pk2_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::numstr_,
                    Discret::Elements::So3Plast<distype>::numstr_>>
    Discret::Elements::So3Plast<distype>::cmat_;

template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nen_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::xrefe_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nen_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::xcurr_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nen_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::xcurr_rate_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nen_, 1>>
    Discret::Elements::So3Plast<distype>::etemp_;

template <Core::FE::CellType distype>
std::pair<bool, double> Discret::Elements::So3Plast<distype>::detF_;
template <Core::FE::CellType distype>
std::pair<bool, double> Discret::Elements::So3Plast<distype>::detF_0_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::inv_defgrd_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::inv_defgrd_0_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nen_>>
    Discret::Elements::So3Plast<distype>::N_XYZ_0_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::numstr_, 1>>
    Discret::Elements::So3Plast<distype>::rcg_vec_;
template <Core::FE::CellType distype>
std::pair<bool, double> Discret::Elements::So3Plast<distype>::f_bar_fac_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::numdofperelement_, 1>>
    Discret::Elements::So3Plast<distype>::htensor_;

template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::numstr_,
                    Discret::Elements::So3Plast<distype>::numstr_>>
    Discret::Elements::So3Plast<distype>::T0invT_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nsd_,
                    Discret::Elements::So3Plast<distype>::nsd_>>
    Discret::Elements::So3Plast<distype>::jac_0_;
template <Core::FE::CellType distype>
std::pair<bool, double> Discret::Elements::So3Plast<distype>::det_jac_0_;
template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::SerialDenseMatrix> Discret::Elements::So3Plast<distype>::M_eas_;

template <Core::FE::CellType distype>
std::pair<bool, Core::LinAlg::Matrix<Discret::Elements::So3Plast<distype>::nen_, 1>>
    Discret::Elements::So3Plast<distype>::weights_;
template <Core::FE::CellType distype>
std::pair<bool, std::vector<Core::LinAlg::SerialDenseVector>>
    Discret::Elements::So3Plast<distype>::knots_;

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
int Discret::Elements::So3Plast<distype>::num_volume() const
{
  switch (distype)
  {
    case Core::FE::CellType::tet4:
    case Core::FE::CellType::hex8:
    case Core::FE::CellType::hex18:
    case Core::FE::CellType::hex27:
    case Core::FE::CellType::nurbs27:
      return 0;
    default:
      FOUR_C_THROW("unknown distpye for So3Plast");
  }
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
int Discret::Elements::So3Plast<distype>::num_surface() const
{
  switch (distype)
  {
    case Core::FE::CellType::hex8:
    case Core::FE::CellType::hex18:
    case Core::FE::CellType::hex27:
    case Core::FE::CellType::nurbs27:
      return 6;
    case Core::FE::CellType::tet4:
      return 4;
    default:
      FOUR_C_THROW("unknown distpye for So3Plast");
  }
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
int Discret::Elements::So3Plast<distype>::num_line() const
{
  switch (distype)
  {
    case Core::FE::CellType::hex8:
    case Core::FE::CellType::hex18:
    case Core::FE::CellType::hex27:
    case Core::FE::CellType::nurbs27:
      return 12;
    case Core::FE::CellType::tet4:
      return 6;
    default:
      FOUR_C_THROW("unknown distpye for So3Plast");
  }
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
std::vector<std::shared_ptr<Core::Elements::Element>> Discret::Elements::So3Plast<distype>::lines()
{
  return Core::Communication::element_boundary_factory<StructuralLine, Core::Elements::Element>(
      Core::Communication::buildLines, *this);
}

/*----------------------------------------------------------------------*
 |                                                          seitz 05/14 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
std::vector<std::shared_ptr<Core::Elements::Element>>
Discret::Elements::So3Plast<distype>::surfaces()
{
  return Core::Communication::element_boundary_factory<StructuralSurface, Core::Elements::Element>(
      Core::Communication::buildSurfaces, *this);
}

/*----------------------------------------------------------------------*
 | pack data (public)                                       seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
void Discret::Elements::So3Plast<distype>::pack(Core::Communication::PackBuffer& data) const
{
  // pack type of this instance of ParObject
  int type = unique_par_object_id();
  add_to_pack(data, type);

  // add base class Element
  SoBase::pack(data);

  // Gauss points and weights
  add_to_pack(data, xsi_);
  add_to_pack(data, wgt_);

  // parameters
  add_to_pack(data, fbar_);

  // plastic spin type
  add_to_pack(data, plspintype_);

  // tsi
  add_to_pack(data, tsi_);
  if (tsi_)
  {
    add_to_pack(data, *dFintdT_);
    add_to_pack(data, *KbT_);
    add_to_pack(data, *temp_last_);
  }

  // EAS element technology
  add_to_pack(data, eastype_);
  add_to_pack(data, neas_);
  if (eastype_ != soh8p_easnone)
  {
    add_to_pack(data, (*alpha_eas_));
    add_to_pack(data, (*alpha_eas_last_timestep_));
    add_to_pack(data, (*alpha_eas_delta_over_last_timestep_));
  }

  // history at each Gauss point
  add_to_pack(data, dDp_last_iter_);

  // nitsche contact
  add_to_pack(data, is_nitsche_contact_);
  if (is_nitsche_contact_)
  {
    add_to_pack(data, cauchy_);
    add_to_pack(data, cauchy_deriv_);
    if (tsi_) add_to_pack(data, cauchy_deriv_T_);
  }
}  // pack()


/*----------------------------------------------------------------------*
 | unpack data (public)                                     seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
void Discret::Elements::So3Plast<distype>::unpack(Core::Communication::UnpackBuffer& buffer)
{
  Core::Communication::extract_and_assert_id(buffer, unique_par_object_id());

  // extract base class Element
  SoBase::unpack(buffer);

  // Gauss points and weights
  extract_from_pack(buffer, xsi_);
  extract_from_pack(buffer, wgt_);
  numgpt_ = wgt_.size();

  // paramters
  extract_from_pack(buffer, fbar_);

  // plastic spin type
  extract_from_pack(buffer, plspintype_);

  // tsi
  extract_from_pack(buffer, tsi_);
  if (tsi_)
  {
    dFintdT_ = std::make_shared<std::vector<Core::LinAlg::Matrix<numdofperelement_, 1>>>(numgpt_);
    KbT_ = std::make_shared<std::vector<Core::LinAlg::SerialDenseVector>>(
        numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));
    temp_last_ = std::make_shared<std::vector<double>>(numgpt_);

    extract_from_pack(buffer, (*dFintdT_));
    extract_from_pack(buffer, (*KbT_));
    extract_from_pack(buffer, (*temp_last_));
  }

  // EAS element technology
  extract_from_pack(buffer, eastype_);
  extract_from_pack(buffer, neas_);

  // no EAS
  if (eastype_ == soh8p_easnone)
  {
    KaaInv_ = nullptr;
    Kad_ = nullptr;
    KaT_ = nullptr;
    KdT_eas_ = nullptr;
    feas_ = nullptr;
    Kba_ = nullptr;
    alpha_eas_ = nullptr;
    alpha_eas_last_timestep_ = nullptr;
    alpha_eas_delta_over_last_timestep_ = nullptr;
    alpha_eas_inc_ = nullptr;
  }
  else
  {
    KaaInv_ = std::make_shared<Core::LinAlg::SerialDenseMatrix>(neas_, neas_, true);
    Kad_ = std::make_shared<Core::LinAlg::SerialDenseMatrix>(neas_, numdofperelement_, true);
    if (tsi_)
    {
      KaT_ = std::make_shared<Core::LinAlg::SerialDenseMatrix>(neas_, nen_, true);
      KdT_eas_ = std::make_shared<Core::LinAlg::Matrix<numdofperelement_, nen_>>();
    }
    feas_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    Kba_ = std::make_shared<std::vector<Core::LinAlg::SerialDenseMatrix>>(
        numgpt_, Core::LinAlg::SerialDenseMatrix(plspintype_, neas_, true));
    alpha_eas_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    alpha_eas_last_timestep_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    alpha_eas_delta_over_last_timestep_ =
        std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
    alpha_eas_inc_ = std::make_shared<Core::LinAlg::SerialDenseVector>(neas_, true);
  }

  KbbInv_.resize(numgpt_, Core::LinAlg::SerialDenseMatrix(plspintype_, plspintype_, true));
  Kbd_.resize(numgpt_, Core::LinAlg::SerialDenseMatrix(plspintype_, numdofperelement_, true));
  fbeta_.resize(numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));
  dDp_last_iter_.resize(numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));
  dDp_inc_.resize(numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));

  if (eastype_ != soh8p_easnone)
  {
    extract_from_pack(buffer, (*alpha_eas_));
    extract_from_pack(buffer, (*alpha_eas_last_timestep_));
    extract_from_pack(buffer, (*alpha_eas_delta_over_last_timestep_));
  }

  extract_from_pack(buffer, dDp_last_iter_);

  // Nitsche contact stuff
  extract_from_pack(buffer, is_nitsche_contact_);
  if (is_nitsche_contact_)
  {
    extract_from_pack(buffer, cauchy_);
    extract_from_pack(buffer, cauchy_deriv_);
    if (tsi_)
      extract_from_pack(buffer, cauchy_deriv_T_);
    else
      cauchy_deriv_T_.resize(0);
  }
  else
  {
    cauchy_.resize(0);
    cauchy_deriv_.resize(0);
    cauchy_deriv_T_.resize(0);
  }


}  // unpack()


/*----------------------------------------------------------------------*
 | print this element (public)                              seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
void Discret::Elements::So3Plast<distype>::print(std::ostream& os) const
{
  os << "So3Plast ";
}


/*----------------------------------------------------------------------*
 | read this element, get the material (public)             seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
bool Discret::Elements::So3Plast<distype>::read_element(const std::string& eletype,
    const std::string& eledistype, const Core::IO::InputParameterContainer& container)
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
  {
    FOUR_C_THROW("Reading of SO3_PLAST element failed! KINEM unknown");
  }

  // fbar
  if (container.get_if<std::string>("FBAR") != nullptr)
  {
    auto fb = container.get<std::string>("FBAR");
    if (fb == "yes")
      fbar_ = true;
    else if (fb == "no")
      fbar_ = false;
    else
      FOUR_C_THROW("unknown fbar option (valid: yes/no)");
  }

  // quadrature
  if (container.get_if<int>("NUMGP") != nullptr)
  {
    if (distype != Core::FE::CellType::hex8)
      FOUR_C_THROW("You may only choose the Gauss point number for SOLIDH8PLAST");
    if (Global::Problem::instance()->get_problem_type() == Core::ProblemType::tsi)
      FOUR_C_THROW("You may not choose the Gauss point number in TSI problems");

    int ngp = container.get<int>("NUMGP");

    switch (ngp)
    {
      case 8:
      {
        Core::FE::IntPointsAndWeights<nsd_> intpoints(Core::FE::GaussRule3D::hex_8point);
        numgpt_ = intpoints.ip().nquad;
        xsi_.resize(numgpt_);
        wgt_.resize(numgpt_);
        for (int gp = 0; gp < numgpt_; ++gp)
        {
          wgt_[gp] = (intpoints.ip().qwgt)[gp];
          const double* gpcoord = (intpoints.ip().qxg)[gp];
          for (int idim = 0; idim < nsd_; idim++) xsi_[gp](idim) = gpcoord[idim];
        }
        break;
      }
      case 9:
      {
        Core::FE::GaussIntegration ip(distype, 3);
        numgpt_ = ip.num_points() + 1;
        xsi_.resize(numgpt_);
        wgt_.resize(numgpt_);
        for (int gp = 0; gp < numgpt_ - 1; ++gp)
        {
          wgt_[gp] = 5. / 9.;
          const double* gpcoord = ip.point(gp);
          for (int idim = 0; idim < nsd_; idim++) xsi_[gp](idim) = gpcoord[idim];
        }
        // 9th quadrature point at element center
        xsi_[numgpt_ - 1](0) = 0.;
        xsi_[numgpt_ - 1](1) = 0.;
        xsi_[numgpt_ - 1](2) = 0.;
        wgt_[numgpt_ - 1] = 32. / 9.;
        break;
      }
      case 27:
      {
        Core::FE::IntPointsAndWeights<nsd_> intpoints(Core::FE::GaussRule3D::hex_27point);
        numgpt_ = intpoints.ip().nquad;
        xsi_.resize(numgpt_);
        wgt_.resize(numgpt_);
        for (int gp = 0; gp < numgpt_; ++gp)
        {
          wgt_[gp] = (intpoints.ip().qwgt)[gp];
          const double* gpcoord = (intpoints.ip().qxg)[gp];
          for (int idim = 0; idim < nsd_; idim++) xsi_[gp](idim) = gpcoord[idim];
        }
        break;
      }
      default:
        FOUR_C_THROW("so3_plast doesn't know what to do with %i Gauss points", ngp);
    }
  }
  else  // default integration
  {
    Core::FE::IntPointsAndWeights<nsd_> intpoints(Thermo::DisTypeToOptGaussRule<distype>::rule);
    numgpt_ = intpoints.ip().nquad;
    xsi_.resize(numgpt_);
    wgt_.resize(numgpt_);
    for (int gp = 0; gp < numgpt_; ++gp)
    {
      wgt_[gp] = (intpoints.ip().qwgt)[gp];
      const double* gpcoord = (intpoints.ip().qxg)[gp];
      for (int idim = 0; idim < nsd_; idim++) xsi_[gp](idim) = gpcoord[idim];
    }
  }

  // read number of material model
  int material_id = container.get<int>("MAT");

  set_material(0, Mat::factory(material_id));

  std::shared_ptr<Mat::So3Material> so3mat = solid_material();
  so3mat->setup(numgpt_, container);
  so3mat->valid_kinematics(Inpar::Solid::KinemType::nonlinearTotLag);

  // Validate that materials doesn't use extended update call.
  if (solid_material()->uses_extended_update())
    FOUR_C_THROW("This element currently does not support the extended update call.");

  if (so3mat->material_type() != Core::Materials::m_plelasthyper)
    std::cout << "*** warning *** so3plast used w/o PlasticElastHyper material. Better use "
                 "standard solid element!\n";
  if (have_plastic_spin())
    plspintype_ = plspin;
  else
    plspintype_ = zerospin;

  // EAS
  buffer = container.get_or<std::string>("EAS", "none");

  if (buffer == "none")
    eastype_ = soh8p_easnone;
  else if (buffer == "mild")
  {
    if (distype != Core::FE::CellType::hex8)
      FOUR_C_THROW("EAS in so3 plast currently only for HEX8 elements");
    else
      eastype_ = soh8p_easmild;
  }

  else if (buffer == "full")
  {
    if (distype != Core::FE::CellType::hex8)
      FOUR_C_THROW("EAS in so3 plast currently only for HEX8 elements");
    else
      eastype_ = soh8p_easfull;
  }
  else
  {
    FOUR_C_THROW("unknown EAS type for so3_plast");
  }

  if (fbar_ && eastype_ != soh8p_easnone) FOUR_C_THROW("no combination of Fbar and EAS");

  // initialize EAS data
  eas_init();

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


  return true;

}  // read_element()

/*----------------------------------------------------------------------*
 | get the nodes from so3 (public)                          seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
int Discret::Elements::So3Plast<distype>::unique_par_object_id() const
{
  switch (distype)
  {
    case Core::FE::CellType::hex8:
    {
      return SoHex8PlastType::instance().unique_par_object_id();
    }  // hex8
    case Core::FE::CellType::hex27:
      return SoHex27PlastType::instance().unique_par_object_id();
    case Core::FE::CellType::tet4:
      return SoTet4PlastType::instance().unique_par_object_id();
    case Core::FE::CellType::nurbs27:
      return SoNurbs27PlastType::instance().unique_par_object_id();
    default:
      FOUR_C_THROW("unknown element type!");
  }
  // Intel compiler needs a return
  return -1;

}  // unique_par_object_id()


/*----------------------------------------------------------------------*
 | get the nodes from so3 (public)                          seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
Core::Elements::ElementType& Discret::Elements::So3Plast<distype>::element_type() const
{
  switch (distype)
  {
    case Core::FE::CellType::hex8:
    {
      return SoHex8PlastType::instance();
    }
    case Core::FE::CellType::hex27:
      return SoHex27PlastType::instance();
    case Core::FE::CellType::tet4:
      return SoTet4PlastType::instance();
    case Core::FE::CellType::nurbs27:
      return SoNurbs27PlastType::instance();
    default:
      FOUR_C_THROW("unknown element type!");
  }
  // Intel compiler needs a return
  return SoHex8PlastType::instance();

};  // element_type()


/*----------------------------------------------------------------------*
 | return names of visualization data (public)              seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
void Discret::Elements::So3Plast<distype>::vis_names(std::map<std::string, int>& names)
{
  Core::Elements::Element::vis_names(names);
  solid_material()->vis_names(names);
}  // vis_names()

/*----------------------------------------------------------------------*
 | return visualization data (public)                       seitz 07/13 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
bool Discret::Elements::So3Plast<distype>::vis_data(
    const std::string& name, std::vector<double>& data)
{
  // Put the owner of this element into the file (use base class method for this)
  if (Core::Elements::Element::vis_data(name, data)) return true;

  return solid_material()->vis_data(name, data, numgpt_, id());

}  // vis_data()

/*----------------------------------------------------------------------*
 | read relevant parameters from paramter list              seitz 01/14 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
void Discret::Elements::So3Plast<distype>::read_parameter_list(
    std::shared_ptr<Teuchos::ParameterList> plparams)
{
  double cpl = plparams->get<double>("SEMI_SMOOTH_CPL");
  double s = plparams->get<double>("STABILIZATION_S");
  if (material()->material_type() == Core::Materials::m_plelasthyper)
    static_cast<Mat::PlasticElastHyper*>(material().get())->get_params(s, cpl);

  auto probtype = Teuchos::getIntegralValue<Core::ProblemType>(*plparams, "Core::ProblemType");
  if (probtype == Core::ProblemType::tsi)
    tsi_ = true;
  else
    tsi_ = false;
  if (tsi_)
  {
    // get plastic hyperelastic material
    Mat::PlasticElastHyper* plmat = nullptr;
    if (material()->material_type() == Core::Materials::m_plelasthyper)
      plmat = static_cast<Mat::PlasticElastHyper*>(material().get());
    else
      FOUR_C_THROW("so3_ssn_plast elements only with PlasticElastHyper material");

    // get dissipation mode
    auto mode =
        Teuchos::getIntegralValue<Inpar::TSI::DissipationMode>(*plparams, "DISSIPATION_MODE");

    // prepare material for tsi
    plmat->setup_tsi(numgpt_, numdofperelement_, (eastype_ != soh8p_easnone), mode);

    // setup element data
    dFintdT_ = std::make_shared<std::vector<Core::LinAlg::Matrix<numdofperelement_, 1>>>(numgpt_);
    temp_last_ = std::make_shared<std::vector<double>>(numgpt_, plmat->init_temp());
    KbT_ = std::make_shared<std::vector<Core::LinAlg::SerialDenseVector>>(
        numgpt_, Core::LinAlg::SerialDenseVector(plspintype_, true));

    if (eastype_ != soh8p_easnone)
    {
      KaT_ = std::make_shared<Core::LinAlg::SerialDenseMatrix>(neas_, nen_, true);
      KdT_eas_ = std::make_shared<Core::LinAlg::Matrix<numdofperelement_, nen_>>();
    }
    else
    {
      KaT_ = nullptr;
      KdT_eas_ = nullptr;
    }
  }
}


/*----------------------------------------------------------------------*
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
template <unsigned int num_cols>
void Discret::Elements::So3Plast<distype>::soh8_expol(
    Core::LinAlg::Matrix<numgpt_post, num_cols>& data, Core::LinAlg::MultiVector<double>& expolData)
{
  if (distype != Core::FE::CellType::hex8) FOUR_C_THROW("soh8_expol called from non-hex8 element");

  // static variables, that are the same for every element
  static Core::LinAlg::Matrix<nen_, numgpt_post> expolOperator;
  static bool isfilled;

  if (isfilled == false)
  {
    double sq3 = sqrt(3.0);

    expolOperator(0, 0) = 1.25 + 0.75 * sq3;
    expolOperator(0, 1) = -0.25 - 0.25 * sq3;
    expolOperator(0, 2) = -0.25 + 0.25 * sq3;
    expolOperator(0, 3) = -0.25 - 0.25 * sq3;
    expolOperator(0, 4) = -0.25 - 0.25 * sq3;
    expolOperator(0, 5) = -0.25 + 0.25 * sq3;
    expolOperator(0, 6) = 1.25 - 0.75 * sq3;
    expolOperator(0, 7) = -0.25 + 0.25 * sq3;
    expolOperator(1, 1) = 1.25 + 0.75 * sq3;
    expolOperator(1, 2) = -0.25 - 0.25 * sq3;
    expolOperator(1, 3) = -0.25 + 0.25 * sq3;
    expolOperator(1, 4) = -0.25 + 0.25 * sq3;
    expolOperator(1, 5) = -0.25 - 0.25 * sq3;
    expolOperator(1, 6) = -0.25 + 0.25 * sq3;
    expolOperator(1, 7) = 1.25 - 0.75 * sq3;
    expolOperator(2, 2) = 1.25 + 0.75 * sq3;
    expolOperator(2, 3) = -0.25 - 0.25 * sq3;
    expolOperator(2, 4) = 1.25 - 0.75 * sq3;
    expolOperator(2, 5) = -0.25 + 0.25 * sq3;
    expolOperator(2, 6) = -0.25 - 0.25 * sq3;
    expolOperator(2, 7) = -0.25 + 0.25 * sq3;
    expolOperator(3, 3) = 1.25 + 0.75 * sq3;
    expolOperator(3, 4) = -0.25 + 0.25 * sq3;
    expolOperator(3, 5) = 1.25 - 0.75 * sq3;
    expolOperator(3, 6) = -0.25 + 0.25 * sq3;
    expolOperator(3, 7) = -0.25 - 0.25 * sq3;
    expolOperator(4, 4) = 1.25 + 0.75 * sq3;
    expolOperator(4, 5) = -0.25 - 0.25 * sq3;
    expolOperator(4, 6) = -0.25 + 0.25 * sq3;
    expolOperator(4, 7) = -0.25 - 0.25 * sq3;
    expolOperator(5, 5) = 1.25 + 0.75 * sq3;
    expolOperator(5, 6) = -0.25 - 0.25 * sq3;
    expolOperator(5, 7) = -0.25 + 0.25 * sq3;
    expolOperator(6, 6) = 1.25 + 0.75 * sq3;
    expolOperator(6, 7) = -0.25 - 0.25 * sq3;
    expolOperator(7, 7) = 1.25 + 0.75 * sq3;

    for (int i = 0; i < NUMNOD_SOH8; ++i)
    {
      for (int j = 0; j < i; ++j)
      {
        expolOperator(i, j) = expolOperator(j, i);
      }
    }

    isfilled = true;
  }

  Core::LinAlg::Matrix<nen_, num_cols> nodalData;
  nodalData.multiply(expolOperator, data);

  // "assembly" of extrapolated nodal data
  for (int i = 0; i < nen_; ++i)
  {
    const int lid = expolData.Map().LID(node_ids()[i]);
    if (lid >= 0)  // rownode
    {
      const double invmyadjele = 1.0 / nodes()[i]->num_element();
      for (unsigned int j = 0; j < num_cols; ++j)
        ((expolData(j)))[lid] += nodalData(i, j) * invmyadjele;
    }
  }
}

template void Discret::Elements::So3Plast<Core::FE::CellType::hex8>::soh8_expol(
    Core::LinAlg::Matrix<numgpt_post, 1>&, Core::LinAlg::MultiVector<double>&);
template void Discret::Elements::So3Plast<Core::FE::CellType::hex8>::soh8_expol(
    Core::LinAlg::Matrix<numgpt_post, numstr_>&, Core::LinAlg::MultiVector<double>&);
template void Discret::Elements::So3Plast<Core::FE::CellType::hex8>::soh8_expol(
    Core::LinAlg::Matrix<numgpt_post, 9>&, Core::LinAlg::MultiVector<double>&);

/*----------------------------------------------------------------------*
 | Have plastic spin                                        seitz 05/14 |
 *----------------------------------------------------------------------*/
template <Core::FE::CellType distype>
bool Discret::Elements::So3Plast<distype>::have_plastic_spin()
{
  // get plastic hyperelastic material
  Mat::PlasticElastHyper* plmat = nullptr;
  if (material()->material_type() == Core::Materials::m_plelasthyper)
    plmat = static_cast<Mat::PlasticElastHyper*>(material().get());

  if (plmat != nullptr) return plmat->have_plastic_spin();

  return false;
}

int Discret::Elements::plast_eas_type_to_num_eas_v(Discret::Elements::So3PlastEasType et)
{
  switch (et)
  {
    case soh8p_easnone:
      return PlastEasTypeToNumEas<soh8p_easnone>::neas;
    case soh8p_easmild:
      return PlastEasTypeToNumEas<soh8p_easmild>::neas;
    case soh8p_easfull:
      return PlastEasTypeToNumEas<soh8p_easfull>::neas;
    case soh8p_eassosh8:
      return PlastEasTypeToNumEas<soh8p_eassosh8>::neas;
    case soh18p_eassosh18:
      return PlastEasTypeToNumEas<soh18p_eassosh18>::neas;
    default:
      FOUR_C_THROW("EAS type not implemented");
  }
}

FOUR_C_NAMESPACE_CLOSE

#include "4C_so3_ssn_plast_fwd.hpp"
