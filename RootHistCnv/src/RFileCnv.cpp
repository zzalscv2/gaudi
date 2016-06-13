// Include files
#include "GaudiKernel/IOpaqueAddress.h"
#include "GaudiKernel/IRegistry.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/IJobOptionsSvc.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/Bootstrap.h"

// ROOT
#include "TROOT.h"
#include "RFileCnv.h"
#include "TFile.h"

// local
#include "RootCompressionSettings.h"
#include <map>
#include <string>
#include <GaudiKernel/IMetaDataSvc.h>
#include <GaudiKernel/PropertyMgr.h>

// Instantiation of a static factory class used by clients to create
// instances of this service
DECLARE_NAMESPACE_CONVERTER_FACTORY(RootHistCnv,RFileCnv)

// Standard constructor
RootHistCnv::RFileCnv::RFileCnv( ISvcLocator* svc )
: RDirectoryCnv ( svc, classID() )
{ }

namespace {
  const std::string emptyName{};
  /// Helper to allow instantiation of PropertyMgr.
  struct AnonymousPropertyMgr: public implements<IProperty, INamedInterface>,
                               public PropertyMgr {
    const std::string& name() const override { return emptyName; }
  };
}
//------------------------------------------------------------------------------
StatusCode RootHistCnv::RFileCnv::initialize()
{
  // Set compression level property ...
  std::unique_ptr<PropertyMgr> pmgr ( new AnonymousPropertyMgr() );
  pmgr->declareProperty(m_compLevel);
  ISvcLocator * svcLoc = Gaudi::svcLocator();
  auto jobSvc = svcLoc->service<IJobOptionsSvc>("JobOptionsSvc");
  const StatusCode sc = ( jobSvc &&
                          jobSvc->setMyProperties("RFileCnv",pmgr.get()) );

  // initialise base class
  return ( sc && RDirectoryCnv::initialize() );
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
StatusCode RootHistCnv::RFileCnv::createObj( IOpaqueAddress* pAddress,
                                             DataObject*& refpObject )
//------------------------------------------------------------------------------
{
  MsgStream log(msgSvc(), "RFileCnv");
  unsigned long*     ipar = (unsigned long*)pAddress->ipar();
  char mode[2] = { char(ipar[1]), 0 };

  std::string fname  = pAddress->par()[0]; // Container name
  std::string ooname = pAddress->par()[1]; // Object name

  const std::string* spar = pAddress->par();
  // Strip of store name to get the top level RZ directory
  std::string oname = spar[1].substr(spar[1].find("/",1)+1);

  // Protect against multiple instances of TROOT
  if ( !gROOT )   {
    static TROOT root("root","ROOT I/O");
    //    gDebug = 99;
  } else {
    log << MSG::VERBOSE << "ROOT already initialized, debug = "
        << gDebug<< endmsg;
  }

  // Determine access mode:

  if ( mode[0] == 'O' ) {

    if (findTFile(ooname,rfile).isFailure()) {

      log << MSG::INFO << "opening Root file \"" << fname << "\" for reading"
          << endmsg;

      rfile = TFile::Open(fname.c_str(),"READ");
      if ( rfile && rfile->IsOpen() ) {
        regTFile(ooname,rfile).ignore();

        ipar[0] = (unsigned long)rfile;
        NTuple::File* pFile = new NTuple::File(objType(), fname, oname);
        pFile->setOpen(true);
        refpObject = pFile;

        return StatusCode::SUCCESS;

      } else {
        log << MSG::ERROR << "Couldn't open \"" << fname << "\" for reading"
            << endmsg;
        return StatusCode::FAILURE;
      }

    } else {
      log << MSG::DEBUG << "Root file \"" << fname << "\" already opened"
          << endmsg;
      return StatusCode::SUCCESS;
    }


  } else if ( mode[0] == 'U' ) {

    log << MSG::INFO << "opening Root file \"" << fname << "\" for updating"
        << endmsg;

    log << MSG::ERROR << "don't know how to do this yet. Aborting." << endmsg;
    exit(1);

  } else if ( mode[0] == 'N' ) {

    const auto& compLevel = m_compLevel.value();
    log << MSG::INFO << "opening Root file \"" << fname << "\" for writing";
    if ( !compLevel.empty() )
    { log << ", CompressionLevel='" << compLevel << "'"; }
    log << endmsg;

    rfile = TFile::Open( fname.c_str(), "RECREATE", "Gaudi Trees" );
    if ( ! ( rfile && rfile->IsOpen() ) ) {
      log << MSG::ERROR << "Could not open file " << fname << " for writing"
          << endmsg;
      return StatusCode::FAILURE;
    }
    if ( !compLevel.empty() )
    {
      const RootCompressionSettings settings(compLevel);
      rfile->SetCompressionSettings(settings.level());
    }

    regTFile(ooname,rfile).ignore();

    log << MSG::DEBUG << "creating ROOT file " << fname << endmsg;

    ipar[0] = (unsigned long)rfile;
    NTuple::File* pFile = new NTuple::File(objType(), fname, oname);
    pFile->setOpen(true);
    refpObject = pFile;
    return StatusCode::SUCCESS;

  } else {

    log << MSG::ERROR << "Uknown mode to access ROOT file" << endmsg;
    return StatusCode::FAILURE;

  }

  return StatusCode::FAILURE;
}

//------------------------------------------------------------------------------
StatusCode RootHistCnv::RFileCnv::createRep( DataObject* pObject,
                                             IOpaqueAddress*& refpAddress )
//------------------------------------------------------------------------------
{
  refpAddress = pObject->registry()->address();
  return RFileCnv::updateRep( refpAddress, pObject );
}

//-----------------------------------------------------------------------------
StatusCode RootHistCnv::RFileCnv::updateRep( IOpaqueAddress* pAddress,
                                             DataObject* pObject )
//-----------------------------------------------------------------------------
{
  MsgStream log(msgSvc(), "RFileCnv");
  std::string ooname = pAddress->par()[1];

  NTuple::File* pFile = dynamic_cast<NTuple::File*>(pObject);
  if ( pFile && pFile->isOpen() )    {

    unsigned long* ipar = (unsigned long*)pAddress->ipar();
    if (findTFile(ooname,rfile).isFailure()) {
      log << MSG::ERROR << "Problems closing TFile " << ooname << endmsg;
      return StatusCode::FAILURE;
    }

    rfile->Write(nullptr,TObject::kOverwrite);
    if ( log.level() <= MSG::INFO ) {
      log << MSG::INFO << "dumping contents of " << ooname << endmsg;
      rfile->Print();
    }

  /*
  * MetaData
  * Ana Trisovic
  * March 2015
  * */
  SmartIF <IMetaDataSvc> mds;
  mds = serviceLocator()->service("Gaudi::MetaDataSvc", false);
  //auto mds = service<IMetaDataSvc>("MetaDataSvc", false);
  if (mds) {
          std::map <std::string, std::string> m_metadata = mds->getMetaDataMap();
          if(!rfile->WriteObject(&m_metadata, "info")){
                  return StatusCode::FAILURE;
          }
  }
  /* */

    rfile->Close();
    delete rfile;

    ipar[0] = 0;
    pFile->setOpen(false);
    return StatusCode::SUCCESS;

  } else {
    log << MSG::ERROR << "TFile " << ooname << " is not open" << endmsg;
  }
  return StatusCode::FAILURE;
}
