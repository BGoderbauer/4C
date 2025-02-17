// This file is part of 4C multiphysics licensed under the
// GNU Lesser General Public License v3.0 or later.
//
// See the LICENSE.md file in the top-level for license information.
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "4C_so3_plast_ssn_eletypes.hpp"

#include "4C_io_linedefinition.hpp"
#include "4C_so3_plast_ssn.hpp"

FOUR_C_NAMESPACE_OPEN

/*----------------------------------------------------------------------------*
 *  HEX8 element
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
 | build an instance of plast type                         seitz 07/13 |
 *----------------------------------------------------------------------*/
Discret::Elements::SoHex8PlastType Discret::Elements::SoHex8PlastType::instance_;

Discret::Elements::SoHex8PlastType& Discret::Elements::SoHex8PlastType::instance()
{
  return instance_;
}


/*----------------------------------------------------------------------*
 | create the new element type (public)                     seitz 07/13 |
 | is called in ElementRegisterType                                     |
 *----------------------------------------------------------------------*/
Core::Communication::ParObject* Discret::Elements::SoHex8PlastType::create(
    Core::Communication::UnpackBuffer& buffer)
{
  auto* object = new Discret::Elements::So3Plast<Core::FE::CellType::hex8>(-1, -1);
  object->unpack(buffer);
  return object;
}  // Create()


/*----------------------------------------------------------------------*
 | create the new element type (public)                     seitz 07/13 |
 | is called from ParObjectFactory                                      |
 *----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoHex8PlastType::create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == get_element_type_string())
  {
    std::shared_ptr<Core::Elements::Element> ele =
        std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::hex8>>(id, owner);

    return ele;
  }
  return nullptr;
}  // Create()


/*----------------------------------------------------------------------*
 | create the new element type (public)                     seitz 07/13 |
 | virtual method of ElementType                                        |
 *----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoHex8PlastType::create(
    const int id, const int owner)
{
  std::shared_ptr<Core::Elements::Element> ele =
      std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::hex8>>(id, owner);
  return ele;

}  // Create()


/*----------------------------------------------------------------------*
 | setup the element definition (public)                    seitz 07/13 |
 *----------------------------------------------------------------------*/
void Discret::Elements::SoHex8PlastType::setup_element_definition(
    std::map<std::string, std::map<std::string, Input::LineDefinition>>& definitions)
{
  std::map<std::string, std::map<std::string, Input::LineDefinition>> definitions_hex8;
  SoHex8Type::setup_element_definition(definitions_hex8);

  std::map<std::string, Input::LineDefinition>& defs_hex8 = definitions_hex8["SOLIDH8_DEPRECATED"];

  std::map<std::string, Input::LineDefinition>& defs = definitions[get_element_type_string()];

  defs["HEX8"] = Input::LineDefinition::Builder(defs_hex8["HEX8"])
                     .add_named_string("FBAR")
                     .add_optional_named_int("NUMGP")
                     .build();

}  // setup_element_definition()


/*----------------------------------------------------------------------*
 | initialise the element (public)                          seitz 07/13 |
 *----------------------------------------------------------------------*/
int Discret::Elements::SoHex8PlastType::initialize(Core::FE::Discretization& dis)
{
  for (int i = 0; i < dis.num_my_col_elements(); ++i)
  {
    if (dis.l_col_element(i)->element_type() != *this) continue;

    auto* actele =
        dynamic_cast<Discret::Elements::So3Plast<Core::FE::CellType::hex8>*>(dis.l_col_element(i));
    if (!actele) FOUR_C_THROW("cast to So_hex8_Plast* failed");
    // initialise all quantities
    actele->init_jacobian_mapping();
  }

  return 0;
}  // initialize()
/*----------------------------------------------------------------------------*
 | END HEX8 Element
 *----------------------------------------------------------------------------*/



/*----------------------------------------------------------------------------*
 *  HEX18 element
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
| build an instance of Plast type                         seitz 07/13 |
*----------------------------------------------------------------------*/
Discret::Elements::SoHex18PlastType Discret::Elements::SoHex18PlastType::instance_;

Discret::Elements::SoHex18PlastType& Discret::Elements::SoHex18PlastType::instance()
{
  return instance_;
}

/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called in ElementRegisterType                                     |
*----------------------------------------------------------------------*/
Core::Communication::ParObject* Discret::Elements::SoHex18PlastType::create(
    Core::Communication::UnpackBuffer& buffer)
{
  auto* object = new Discret::Elements::So3Plast<Core::FE::CellType::hex18>(-1, -1);
  object->unpack(buffer);
  return object;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called from ParObjectFactory                                      |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoHex18PlastType::create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == get_element_type_string())
  {
    std::shared_ptr<Core::Elements::Element> ele =
        std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::hex18>>(id, owner);
    return ele;
  }
  return nullptr;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| virtual method of ElementType                                        |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoHex18PlastType::create(
    const int id, const int owner)
{
  std::shared_ptr<Core::Elements::Element> ele =
      std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::hex18>>(id, owner);
  return ele;
}  // Create()


/*---------------------------------------------------------------------*
|                                                          seitz 07/13 |
*----------------------------------------------------------------------*/
void Discret::Elements::SoHex18PlastType::setup_element_definition(
    std::map<std::string, std::map<std::string, Input::LineDefinition>>& definitions)
{
  std::map<std::string, Input::LineDefinition>& defs = definitions[get_element_type_string()];

  defs["HEX18"] = Input::LineDefinition::Builder()
                      .add_int_vector("HEX18", 18)
                      .add_named_int("MAT")
                      .add_named_string("KINEM")
                      .add_optional_named_double_vector("RAD", 3)
                      .add_optional_named_double_vector("AXI", 3)
                      .add_optional_named_double_vector("CIR", 3)
                      .add_optional_named_double_vector("FIBER1", 3)
                      .add_optional_named_double_vector("FIBER2", 3)
                      .add_optional_named_double_vector("FIBER3", 3)
                      .add_optional_named_double("STRENGTH")
                      .build();
}  // setup_element_definition()


/*----------------------------------------------------------------------*
| initialise the element (public)                          seitz 07/13 |
*----------------------------------------------------------------------*/
int Discret::Elements::SoHex18PlastType::initialize(Core::FE::Discretization& dis)
{
  for (int i = 0; i < dis.num_my_col_elements(); ++i)
  {
    if (dis.l_col_element(i)->element_type() != *this) continue;

    auto* actele =
        dynamic_cast<Discret::Elements::So3Plast<Core::FE::CellType::hex18>*>(dis.l_col_element(i));
    if (!actele) FOUR_C_THROW("cast to So_hex18_Plast* failed");

    actele->init_jacobian_mapping();
  }

  return 0;
}  // initialize()
/*----------------------------------------------------------------------------*
| END HEX18 Element
*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *  HEX27 element
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
| build an instance of Plast type                         seitz 07/13 |
*----------------------------------------------------------------------*/
Discret::Elements::SoHex27PlastType Discret::Elements::SoHex27PlastType::instance_;

Discret::Elements::SoHex27PlastType& Discret::Elements::SoHex27PlastType::instance()
{
  return instance_;
}

/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called in ElementRegisterType                                     |
*----------------------------------------------------------------------*/
Core::Communication::ParObject* Discret::Elements::SoHex27PlastType::create(
    Core::Communication::UnpackBuffer& buffer)
{
  auto* object = new Discret::Elements::So3Plast<Core::FE::CellType::hex27>(-1, -1);
  object->unpack(buffer);
  return object;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called from ParObjectFactory                                      |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoHex27PlastType::create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == get_element_type_string())
  {
    std::shared_ptr<Core::Elements::Element> ele =
        std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::hex27>>(id, owner);
    return ele;
  }
  return nullptr;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| virtual method of ElementType                                        |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoHex27PlastType::create(
    const int id, const int owner)
{
  std::shared_ptr<Core::Elements::Element> ele =
      std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::hex27>>(id, owner);
  return ele;
}  // Create()


/*---------------------------------------------------------------------*
|                                                          seitz 07/13 |
*----------------------------------------------------------------------*/
void Discret::Elements::SoHex27PlastType::setup_element_definition(
    std::map<std::string, std::map<std::string, Input::LineDefinition>>& definitions)
{
  std::map<std::string, std::map<std::string, Input::LineDefinition>> definitions_hex27;
  SoHex27Type::setup_element_definition(definitions_hex27);

  std::map<std::string, Input::LineDefinition>& defs_hex27 =
      definitions_hex27["SOLIDH27_DEPRECATED"];

  std::map<std::string, Input::LineDefinition>& defs = definitions[get_element_type_string()];

  defs["HEX27"] = defs_hex27["HEX27"];

}  // setup_element_definition()


/*----------------------------------------------------------------------*
| initialise the element (public)                          seitz 07/13 |
*----------------------------------------------------------------------*/
int Discret::Elements::SoHex27PlastType::initialize(Core::FE::Discretization& dis)
{
  for (int i = 0; i < dis.num_my_col_elements(); ++i)
  {
    if (dis.l_col_element(i)->element_type() != *this) continue;

    auto* actele =
        dynamic_cast<Discret::Elements::So3Plast<Core::FE::CellType::hex27>*>(dis.l_col_element(i));
    if (!actele) FOUR_C_THROW("cast to So_hex27_Plast* failed");

    actele->init_jacobian_mapping();
  }

  return 0;
}  // initialize()
/*----------------------------------------------------------------------------*
| END HEX27 Element
*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------------*
 *  tet4 element
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
| build an instance of Plast type                         seitz 07/13 |
*----------------------------------------------------------------------*/
Discret::Elements::SoTet4PlastType Discret::Elements::SoTet4PlastType::instance_;

Discret::Elements::SoTet4PlastType& Discret::Elements::SoTet4PlastType::instance()
{
  return instance_;
}

/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called in ElementRegisterType                                     |
*----------------------------------------------------------------------*/
Core::Communication::ParObject* Discret::Elements::SoTet4PlastType::create(
    Core::Communication::UnpackBuffer& buffer)
{
  auto* object = new Discret::Elements::So3Plast<Core::FE::CellType::tet4>(-1, -1);
  object->unpack(buffer);
  return object;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called from ParObjectFactory                                      |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoTet4PlastType::create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == get_element_type_string())
  {
    std::shared_ptr<Core::Elements::Element> ele =
        std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::tet4>>(id, owner);
    return ele;
  }
  return nullptr;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| virtual method of ElementType                                        |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoTet4PlastType::create(
    const int id, const int owner)
{
  std::shared_ptr<Core::Elements::Element> ele =
      std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::tet4>>(id, owner);
  return ele;
}  // Create()


/*---------------------------------------------------------------------*
|                                                          seitz 07/13 |
*----------------------------------------------------------------------*/
void Discret::Elements::SoTet4PlastType::setup_element_definition(
    std::map<std::string, std::map<std::string, Input::LineDefinition>>& definitions)
{
  std::map<std::string, std::map<std::string, Input::LineDefinition>> definitions_tet4;
  SoTet4Type::setup_element_definition(definitions_tet4);

  std::map<std::string, Input::LineDefinition>& defs_tet4 = definitions_tet4["SOLIDT4_DEPRECATED"];

  std::map<std::string, Input::LineDefinition>& defs = definitions[get_element_type_string()];

  defs["TET4"] = defs_tet4["TET4"];

}  // setup_element_definition()


/*----------------------------------------------------------------------*
| initialise the element (public)                          seitz 07/13 |
*----------------------------------------------------------------------*/
int Discret::Elements::SoTet4PlastType::initialize(Core::FE::Discretization& dis)
{
  for (int i = 0; i < dis.num_my_col_elements(); ++i)
  {
    if (dis.l_col_element(i)->element_type() != *this) continue;

    auto* actele =
        dynamic_cast<Discret::Elements::So3Plast<Core::FE::CellType::tet4>*>(dis.l_col_element(i));
    if (!actele) FOUR_C_THROW("cast to So_tet4_Plast* failed");

    actele->init_jacobian_mapping();
  }

  return 0;
}  // initialize()
/*----------------------------------------------------------------------------*
| END tet4 Element
*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*
 *  nurbs27 element
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*
| build an instance of Plast type                         seitz 07/13 |
*----------------------------------------------------------------------*/
Discret::Elements::SoNurbs27PlastType Discret::Elements::SoNurbs27PlastType::instance_;

Discret::Elements::SoNurbs27PlastType& Discret::Elements::SoNurbs27PlastType::instance()
{
  return instance_;
}

/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called in ElementRegisterType                                     |
*----------------------------------------------------------------------*/
Core::Communication::ParObject* Discret::Elements::SoNurbs27PlastType::create(
    Core::Communication::UnpackBuffer& buffer)
{
  auto* object = new Discret::Elements::So3Plast<Core::FE::CellType::nurbs27>(-1, -1);
  object->unpack(buffer);
  return object;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| is called from ParObjectFactory                                      |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoNurbs27PlastType::create(
    const std::string eletype, const std::string eledistype, const int id, const int owner)
{
  if (eletype == get_element_type_string())
  {
    std::shared_ptr<Core::Elements::Element> ele =
        std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::nurbs27>>(id, owner);
    return ele;
  }
  return nullptr;
}  // Create()


/*----------------------------------------------------------------------*
| create the new element type (public)                     seitz 07/13 |
| virtual method of ElementType                                        |
*----------------------------------------------------------------------*/
std::shared_ptr<Core::Elements::Element> Discret::Elements::SoNurbs27PlastType::create(
    const int id, const int owner)
{
  std::shared_ptr<Core::Elements::Element> ele =
      std::make_shared<Discret::Elements::So3Plast<Core::FE::CellType::nurbs27>>(id, owner);
  return ele;
}  // Create()


/*---------------------------------------------------------------------*
|                                                          seitz 07/13 |
*----------------------------------------------------------------------*/
void Discret::Elements::SoNurbs27PlastType::setup_element_definition(
    std::map<std::string, std::map<std::string, Input::LineDefinition>>& definitions)
{
  std::map<std::string, Input::LineDefinition>& defs = definitions[get_element_type_string()];

  defs["NURBS27"] = Input::LineDefinition::Builder()
                        .add_int_vector("NURBS27", 27)
                        .add_named_int("MAT")
                        .add_named_string("KINEM")
                        .build();
}  // setup_element_definition()


/*----------------------------------------------------------------------*
| initialise the element (public)                          seitz 07/13 |
*----------------------------------------------------------------------*/
int Discret::Elements::SoNurbs27PlastType::initialize(Core::FE::Discretization& dis)
{
  for (int i = 0; i < dis.num_my_col_elements(); ++i)
  {
    if (dis.l_col_element(i)->element_type() != *this) continue;

    auto* actele = dynamic_cast<Discret::Elements::So3Plast<Core::FE::CellType::nurbs27>*>(
        dis.l_col_element(i));
    if (!actele) FOUR_C_THROW("cast to So_tet4_Plast* failed");

    actele->init_jacobian_mapping();
  }

  return 0;
}  // initialize()
/*----------------------------------------------------------------------------*
| END nurbs27 Element
*----------------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/

FOUR_C_NAMESPACE_CLOSE
