/***********************************************************************************\
* (c) Copyright 1998-2019 CERN for the benefit of the LHCb and ATLAS collaborations *
*                                                                                   *
* This software is distributed under the terms of the Apache version 2 licence,     *
* copied verbatim in the file "LICENSE".                                            *
*                                                                                   *
* In applying this licence, CERN does not waive the privileges and immunities       *
* granted to it by virtue of its status as an Intergovernmental Organization        *
* or submit itself to any jurisdiction.                                             *
\***********************************************************************************/
#ifndef GAUDISVC_GAUDIPI_H
#define GAUDISVC_GAUDIPI_H

#include <AIDA/IHistogram1D.h>
#include <AIDA/IHistogram2D.h>
#include <AIDA/IHistogram3D.h>
#include <AIDA/IProfile1D.h>
#include <AIDA/IProfile2D.h>
#include <GaudiKernel/HistogramBase.h>
#include <GaudiKernel/ISvcLocator.h>
#include <utility>
#include <vector>

class DataObject;
class TH2D;

namespace Gaudi {
  typedef std::vector<double> Edges;

  /// Copy constructor
  std::pair<DataObject*, AIDA::IHistogram1D*> createH1D( ISvcLocator* svcLocator, const std::string& path,
                                                         const AIDA::IHistogram1D& hist );
  /// Creator for 1D histogram with fixed bins
  std::pair<DataObject*, AIDA::IHistogram1D*> createH1D( ISvcLocator* svcLocator, const std::string& path,
                                                         const std::string& title, int nBins = 10,
                                                         double lowerEdge = 0., double upperEdge = 1. );
  /// Creator for 1D histogram with variable bins
  std::pair<DataObject*, AIDA::IHistogram1D*> createH1D( ISvcLocator* svcLocator, const std::string& path,
                                                         const std::string& title, const Edges& e );

  /// Copy constructor
  std::pair<DataObject*, AIDA::IHistogram2D*> createH2D( ISvcLocator* svcLocator, const std::string& path,
                                                         const AIDA::IHistogram2D& hist );
  /// "Adopt" constructor
  std::pair<DataObject*, AIDA::IHistogram2D*> createH2D( ISvcLocator* svcLocator, const std::string& path, TH2D* rep );
  /// Creator for 2 D histograms with fixed bins
  std::pair<DataObject*, AIDA::IHistogram2D*> createH2D( ISvcLocator* svcLocator, const std::string& path,
                                                         const std::string& title, int binsX, double iminX,
                                                         double imaxX, int binsY, double iminY, double imaxY );
  /// Creator for 2 D histograms with variable bins
  std::pair<DataObject*, AIDA::IHistogram2D*> createH2D( ISvcLocator* svcLocator, const std::string& path,
                                                         const std::string& title, const Edges& eX, const Edges& eY );
  /// Create 1D slice from 2D histogram
  std::pair<DataObject*, AIDA::IHistogram1D*> slice1DX( const std::string& name, const AIDA::IHistogram2D& h,
                                                        int firstbin, int lastbin );
  /// Create 1D profile in X from 2D histogram
  std::pair<DataObject*, AIDA::IProfile1D*> profile1DX( const std::string& name, const AIDA::IHistogram2D& h,
                                                        int firstbin, int lastbin );
  /// Create 1D projection in X from 2D histogram
  std::pair<DataObject*, AIDA::IHistogram1D*> project1DX( const std::string& name, const AIDA::IHistogram2D& h,
                                                          int firstbin, int lastbin );
  /// Create 1D slice from 2D histogram
  std::pair<DataObject*, AIDA::IHistogram1D*> slice1DY( const std::string& name, const AIDA::IHistogram2D& h,
                                                        int firstbin, int lastbin );
  /// Create 1D profile in Y from 2D histogram
  std::pair<DataObject*, AIDA::IProfile1D*> profile1DY( const std::string& name, const AIDA::IHistogram2D& h,
                                                        int firstbin, int lastbin );
  /// Create 1D projection in Y from 2D histogram
  std::pair<DataObject*, AIDA::IHistogram1D*> project1DY( const std::string& name, const AIDA::IHistogram2D& h,
                                                          int firstbin, int lastbin );

  /// Copy constructor
  std::pair<DataObject*, AIDA::IHistogram3D*> createH3D( ISvcLocator* svcLocator, const std::string& path,
                                                         const AIDA::IHistogram3D& hist );
  /// Create 3D histogram with fixed bins
  std::pair<DataObject*, AIDA::IHistogram3D*> createH3D( ISvcLocator* svcLocator, const std::string& path,
                                                         const std::string& title, int nBinsX, double xlow, double xup,
                                                         int nBinsY, double ylow, double yup, int nBinsZ, double zlow,
                                                         double zup );
  /// Create 3D histogram with variable bins
  std::pair<DataObject*, AIDA::IHistogram3D*> createH3D( ISvcLocator* svcLocator, const std::string& path,
                                                         const std::string& title, const Edges& eX, const Edges& eY,
                                                         const Edges& eZ );

  /// Copy constructor
  std::pair<DataObject*, AIDA::IProfile1D*> createProf1D( ISvcLocator* svcLocator, const std::string& path,
                                                          const AIDA::IProfile1D& hist );
  /// Creator of 1D profile with fixed bins
  std::pair<DataObject*, AIDA::IProfile1D*> createProf1D( ISvcLocator* svcLocator, const std::string& path,
                                                          const std::string& title, int nBins, double xlow, double xup,
                                                          double ylow, double yup, const std::string& opt = "" );
  /// Creator of 1D profile with variable bins
  std::pair<DataObject*, AIDA::IProfile1D*> createProf1D( ISvcLocator* svcLocator, const std::string& path,
                                                          const std::string& title, const Edges& e, double ylow,
                                                          double yup, const std::string& opt = "" );

  /// Copy constructor
  std::pair<DataObject*, AIDA::IProfile2D*> createProf2D( ISvcLocator* svcLocator, const std::string& path,
                                                          const AIDA::IProfile2D& hist );
  /// Creator for 2 D profile with fixed bins
  std::pair<DataObject*, AIDA::IProfile2D*> createProf2D( ISvcLocator* svcLocator, const std::string& path,
                                                          const std::string& title, int binsX, double iminX,
                                                          double imaxX, int binsY, double iminY, double imaxY,
                                                          double lowerValue, double upperValue );
  /// Creator for 2 D profile with variable bins
  std::pair<DataObject*, AIDA::IProfile2D*> createProf2D( ISvcLocator* svcLocator, const std::string& path,
                                                          const std::string& title, const Edges& eX, const Edges& eY,
                                                          double lowerValue, double upperValue );
} // namespace Gaudi
#endif // GAUDIPI_GAUDIPI_H
