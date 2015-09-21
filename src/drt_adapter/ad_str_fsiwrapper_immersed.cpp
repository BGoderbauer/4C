/*----------------------------------------------------------------------*/
/*!
\file ad_str_fsiwrapper_immersed.cpp

\brief Structural adapter for Immersed and Immersed + ALE FSI problems containing the interface
       and methods dependent on the interface

<pre>
Maintainer: Georg Hammerl
            hammerl@lnm.mw.tum.de
            http://www.lnm.mw.tum.de
            089 - 289-15237
</pre>
*/



#include "ad_str_fsiwrapper_immersed.H"
#include "../drt_lib/drt_globalproblem.H"
#include "../drt_structure/stru_aux.H"
#include "../linalg/linalg_mapextractor.H"
#include "../linalg/linalg_utils.H"

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
ADAPTER::FSIStructureWrapperImmersed::FSIStructureWrapperImmersed(Teuchos::RCP<Structure> structure)
: FPSIStructureWrapper(structure)
{
  // immersed_ale fsi part
  std::vector<Teuchos::RCP<const Epetra_Map> > vecSpaces;

  vecSpaces.push_back(interface_->FSICondMap()); // fsi
  vecSpaces.push_back(interface_->IMMERSEDCondMap()); // immersed

  combinedmap_ = LINALG::MultiMapExtractor::MergeMaps(vecSpaces);

  // full blockmap
  LINALG::MultiMapExtractor blockrowdofmap;
  blockrowdofmap.Setup(*combinedmap_, vecSpaces);

  combinedinterface_ = Teuchos::rcp(new LINALG::MapExtractor(*combinedmap_,interface_->FSICondMap(),interface_->IMMERSEDCondMap()));
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
// Apply interface forces
void ADAPTER::FSIStructureWrapperImmersed::ApplyImmersedInterfaceForces(Teuchos::RCP<Epetra_Vector> iforce_fsi,
                                Teuchos::RCP<Epetra_Vector> iforce_immersed)
{
  Teuchos::RCP<Epetra_Vector> fifc = LINALG::CreateVector(*DofRowMap(), true);

  if(iforce_fsi!=Teuchos::null)
    interface_->AddFSICondVector(iforce_fsi, fifc);
  if(iforce_immersed!=Teuchos::null)
    interface_->AddIMMERSEDCondVector(iforce_immersed, fifc);

  SetForceInterface(fifc);

  PreparePartitionStep();

  return;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FSIStructureWrapperImmersed::ExtractImmersedInterfaceDispnp()
{
  dsassert(interface_->FullMap()->PointSameAs(Dispnp()->Map()),
      "Full map of map extractor and Dispnp() do not match.");

    return interface_->ExtractIMMERSEDCondVector(Dispnp());
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FSIStructureWrapperImmersed::ExtractFullInterfaceDispnp()
{
  dsassert(interface_->FullMap()->PointSameAs(Dispnp()->Map()),
      "Full map of map extractor and Dispnp() do not match.");

  Teuchos::RCP<Epetra_Vector> fullvec = Teuchos::rcp(new Epetra_Vector(*combinedinterface_->FullMap(),true));

  // CondVector is FSI vector
  combinedinterface_->AddCondVector(interface_->ExtractFSICondVector(Dispnp()),fullvec);
  // OtherVector is IMMERSED vector
  combinedinterface_->AddOtherVector(interface_->ExtractIMMERSEDCondVector(Dispnp()),fullvec);

  return fullvec;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FSIStructureWrapperImmersed::PredictImmersedInterfaceDispnp()
{

  Teuchos::RCP<Epetra_Vector> idis;

  switch (predictor_)
  {
  case 1:
  {

    idis  = interface_->ExtractIMMERSEDCondVector(Dispn());

    break;
  }
  case 2:
    // d(n)+dt*(1.5*v(n)-0.5*v(n-1))
    dserror("interface velocity v(n-1) not available");
    break;
  case 3:
  {
    // d(n)+dt*v(n)
    double dt = Dt();

    idis = interface_->ExtractIMMERSEDCondVector(Dispn());
    Teuchos::RCP<Epetra_Vector> ivel
      = interface_->ExtractIMMERSEDCondVector(Veln());

    idis->Update(dt,* ivel, 1.0);
    break;
  }
  case 4:
  {
    // d(n)+dt*v(n)+0.5*dt^2*a(n)
    double dt = Dt();

    idis = interface_->ExtractIMMERSEDCondVector(Dispn());
    Teuchos::RCP<Epetra_Vector> ivel
      = interface_->ExtractIMMERSEDCondVector(Veln());
    Teuchos::RCP<Epetra_Vector> iacc
      = interface_->ExtractIMMERSEDCondVector(Accn());

    idis->Update(dt, *ivel, 0.5*dt*dt, *iacc, 1.0);
    break;
  }
  default:
    dserror("unknown interface displacement predictor '%s'",
        DRT::Problem::Instance()->FSIDynamicParams().sublist("PARTITIONED SOLVER").get<std::string>("PREDICTOR").c_str());
    break;
  }

  return idis;
}

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
Teuchos::RCP<Epetra_Vector> ADAPTER::FSIStructureWrapperImmersed::PredictFullInterfaceDispnp()
{

  Teuchos::RCP<Epetra_Vector> idis  = Teuchos::rcp(new Epetra_Vector(*combinedinterface_->FullMap(),true));

  switch (predictor_)
  {
  case 1:
  {

    // CondVector is FSI vector
    combinedinterface_->AddCondVector(interface_->ExtractFSICondVector(Dispn()),idis);
    // OtherVector is IMMERSED vector
    combinedinterface_->AddOtherVector(interface_->ExtractIMMERSEDCondVector(Dispn()),idis);

    break;
  }
  case 2:
    // d(n)+dt*(1.5*v(n)-0.5*v(n-1))
    dserror("interface velocity v(n-1) not available");
    break;
  case 3:
  {
    // d(n)+dt*v(n)
    double dt = Dt();

    // CondVector is FSI vector
    combinedinterface_->AddCondVector(interface_->ExtractFSICondVector(Dispn()),idis);
    // OtherVector is IMMERSED vector
    combinedinterface_->AddOtherVector(interface_->ExtractIMMERSEDCondVector(Dispn()),idis);

    Teuchos::RCP<Epetra_Vector> ivel = Teuchos::rcp(new Epetra_Vector(*combinedinterface_->FullMap(),true));

    // CondVector is FSI vector
    combinedinterface_->AddCondVector(interface_->ExtractFSICondVector(Veln()),ivel);
    // OtherVector is IMMERSED vector
    combinedinterface_->AddOtherVector(interface_->ExtractIMMERSEDCondVector(Veln()),ivel);

    idis->Update(dt,* ivel, 1.0);
    break;
  }
  case 4:
  {
    // d(n)+dt*v(n)+0.5*dt^2*a(n)
    double dt = Dt();

    // CondVector is FSI vector
    combinedinterface_->AddCondVector(interface_->ExtractFSICondVector(Dispn()),idis);
    // OtherVector is IMMERSED vector
    combinedinterface_->AddOtherVector(interface_->ExtractIMMERSEDCondVector(Dispn()),idis);

    Teuchos::RCP<Epetra_Vector> ivel = Teuchos::rcp(new Epetra_Vector(*combinedinterface_->FullMap(),true));

    // CondVector is FSI vector
    combinedinterface_->AddCondVector(interface_->ExtractFSICondVector(Veln()),ivel);
    // OtherVector is IMMERSED vector
    combinedinterface_->AddOtherVector(interface_->ExtractIMMERSEDCondVector(Veln()),ivel);

    Teuchos::RCP<Epetra_Vector> iacc = Teuchos::rcp(new Epetra_Vector(*combinedinterface_->FullMap(),true));

    // CondVector is FSI vector
    combinedinterface_->AddCondVector(interface_->ExtractFSICondVector(Accn()),iacc);
    // OtherVector is IMMERSED vector
    combinedinterface_->AddOtherVector(interface_->ExtractIMMERSEDCondVector(Accn()),iacc);

    idis->Update(dt, *ivel, 0.5*dt*dt, *iacc, 1.0);
    break;
  }
  default:
    dserror("unknown interface displacement predictor '%s'",
        DRT::Problem::Instance()->FSIDynamicParams().sublist("PARTITIONED SOLVER").get<std::string>("PREDICTOR").c_str());
    break;
  }

  return idis;
}
