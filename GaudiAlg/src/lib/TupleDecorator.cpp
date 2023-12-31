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
// ============================================================================
// Include files
// ============================================================================
// CLHEP
// ============================================================================/
#include "CLHEP/Matrix/GenMatrix.h"
#include "CLHEP/Matrix/Matrix.h"
#include "CLHEP/Matrix/Vector.h"
// Handle CLHEP 2.0.x move to CLHEP namespace
namespace CLHEP {}
using namespace CLHEP;
// CLHEP is just #()$)*#)*@#)$@ Not even the git master (as of Aug 2015) has HepVector::begin and HepVector::end
// defined!!!
// fortunately, ADL comes to the rescue...
namespace CLHEP {
  class HepVector;
  double*       begin( CLHEP::HepVector& v ) { return &v[0]; }
  const double* begin( const CLHEP::HepVector& v ) { return &v[0]; }
} // namespace CLHEP

// ============================================================================/
// GaudiAlg
// ============================================================================
#include "GaudiAlg/Tuple.h"
#include "GaudiAlg/TupleObj.h"
// ============================================================================
// GaudiPython
// ============================================================================
#include "GaudiPython/TupleDecorator.h"
#include "GaudiPython/Vector.h"
// ===========================================================================
/** @file
 *  Implementation file for class GaudiPython::TupleDecorator
 *  @author Vanya  BELYAEV Ivan.Belyaev@lapp.in2p3.fr
 *  @date 2005-08-04
 */
// ============================================================================
namespace {
  // ==========================================================================
  template <class TYPE>
  inline StatusCode _fill( const Tuples::Tuple& tuple, const std::string& name, const TYPE& value ) {
    return tuple.valid() ? tuple->column( name, value ) : StatusCode::FAILURE;
  }
  // ==========================================================================
} // namespace
// ============================================================================
INTuple* GaudiPython::TupleDecorator::nTuple( const Tuples::Tuple& tuple ) {
  if ( !tuple.valid() ) { return 0; }
  return tuple->tuple();
}
// ============================================================================
NTuple::Tuple* GaudiPython::TupleDecorator::ntuple( const Tuples::Tuple& tuple ) {
  if ( !tuple.valid() ) { return 0; }
  return tuple->tuple();
}
// ============================================================================
bool GaudiPython::TupleDecorator::valid( const Tuples::Tuple& tuple ) { return tuple.valid(); }
// ============================================================================
StatusCode GaudiPython::TupleDecorator::write( const Tuples::Tuple& tuple ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->write();
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name, const int value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name, const int value,
                                                const int minv, const int maxv ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->column( name, value, minv, maxv );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name,
                                                const double value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column_ll( const Tuples::Tuple& tuple, const std::string& name,
                                                   const long long value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column_ull( const Tuples::Tuple& tuple, const std::string& name,
                                                    const unsigned long long value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name,
                                                const bool value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name,
                                                IOpaqueAddress* value ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->column( name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, IOpaqueAddress* value ) {
  return column( tuple, "Address", value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::LorentzVector& value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::XYZVector& value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::XYZPoint& value ) {
  return _fill( tuple, name, value );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::farray( const Tuples::Tuple& tuple, const std::string& name,
                                                const std::vector<double>& data, const std::string& length,
                                                const size_t maxv ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->farray( name, data.begin(), data.end(), length, maxv );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::fmatrix( const Tuples::Tuple& tuple, const std::string& name,
                                                 const GaudiPython::Matrix&     data,
                                                 const Tuples::TupleObj::MIndex cols, // fixed !!!
                                                 const std::string& length, const size_t maxv ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  if ( !data.empty() && cols != data.front().size() ) {
    return tuple->Error( "GP:fmatrix(1): mismatch in matrix dimensions!" );
  }
  return tuple->fmatrix( name, data, data.size(), cols, length, maxv );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::fmatrix( const Tuples::Tuple& tuple, const std::string& name,
                                                 const GaudiUtils::VectorMap<int, double>& info,
                                                 const std::string& length, const size_t maxv ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->fmatrix( name, info, length, maxv );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const std::vector<double>& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data.begin(), data.end() );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector1& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector2& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector3& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data.begin(), data.begin() + 3 );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector4& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  // return tuple->array( name , data ) ;
  return tuple->array( name, data.begin(), data.begin() + 4 );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector5& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector6& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector7& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector8& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const Gaudi::Vector9& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const GaudiPython::Matrix&     data,
                                                const Tuples::TupleObj::MIndex cols ) // fixed !!!
{
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  if ( data.empty() ) { return tuple->Warning( "GP:matrix(1): empty fixed matrix, skip matrix " ); }
  if ( cols != data.front().size() ) { return tuple->Error( "GP:matrix(1): mismatch in fixed matrix dimensions!" ); }
  return tuple->matrix( name, data, data.size(), cols );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix1x1& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix2x2& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix3x3& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix4x4& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix5x5& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix6x6& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix7x7& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix8x8& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix9x9& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix1x3& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix1x5& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix1x6& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix4x3& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix3x4& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix3x5& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix3x6& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix2x3& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Matrix3x2& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix1x1& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix2x2& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix3x3& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix4x4& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix5x5& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix6x6& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix7x7& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix8x8& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::SymMatrix9x9& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->matrix( name, data );
}
// ============================================================================
// advanced column: time
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const Gaudi::Time& value ) {
  return column( tuple, "", value );
}
// ============================================================================
// advanced column: time
// ============================================================================
StatusCode GaudiPython::TupleDecorator::column( const Tuples::Tuple& tuple, const std::string& name,
                                                const Gaudi::Time& value ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  StatusCode sc = StatusCode::SUCCESS;
  //
  sc = tuple->column( name + "year", value.year( true ), 1970, 2070 );
  if ( sc.isFailure() ) { return sc; }
  sc = tuple->column( name + "month", value.month( true ) + 1, 1, 16 );
  if ( sc.isFailure() ) { return sc; }
  sc = tuple->column( name + "day", value.day( true ), 0, 32 );
  if ( sc.isFailure() ) { return sc; }
  sc = tuple->column( name + "hour", value.hour( true ), 0, 25 );
  if ( sc.isFailure() ) { return sc; }
  sc = tuple->column( name + "minute", value.minute( true ), 0, 61 );
  if ( sc.isFailure() ) { return sc; }
  sc = tuple->column( name + "second", value.second( true ), 0, 61 );
  if ( sc.isFailure() ) { return sc; }
  sc = tuple->column( name + "nsecond", value.nsecond() );
  //
  return sc;
}
// ============================================================================

// ============================================================================
// Legacy:
// ============================================================================
StatusCode GaudiPython::TupleDecorator::array( const Tuples::Tuple& tuple, const std::string& name,
                                               const CLHEP::HepVector& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  return tuple->array( name, data, data.num_row() );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::farray( const Tuples::Tuple& tuple, const std::string& name,
                                                const CLHEP::HepVector& data, const std::string& length,
                                                const size_t maxv ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  // use the trick!
  const double* begin = &( data[0] );
  const double* end   = begin + data.num_row();
  return tuple->farray( name, begin, end, length, maxv );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::matrix( const Tuples::Tuple& tuple, const std::string& name,
                                                const CLHEP::HepGenMatrix& data ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  if ( 1 > data.num_col() ) { return tuple->Error( "GP:matrix(2): illegal fixed matrix num_col" ); }
  if ( 1 > data.num_row() ) { return tuple->Error( "GP:matrix(2): illegal fixed matrix num_row" ); }
  return tuple->matrix( name, data, data.num_row(), data.num_col() );
}
// ============================================================================
StatusCode GaudiPython::TupleDecorator::fmatrix( const Tuples::Tuple& tuple, const std::string& name,
                                                 const CLHEP::HepGenMatrix&     data,
                                                 const Tuples::TupleObj::MIndex cols, // fixed !!!
                                                 const std::string& length, const size_t maxv ) {
  if ( !tuple.valid() ) { return StatusCode::FAILURE; }
  if ( cols != data.num_col() ) { return tuple->Error( "GP:fmatrix(2): mismatch in matrix dimensions!" ); }
  return tuple->fmatrix( name, data, data.num_row(), cols, length, maxv );
}
// ============================================================================

// ============================================================================
// Tuple-Alg-decorator
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::nTuple( const GaudiTupleAlg& algo, const std::string& title,
                                                      const CLID& clid ) {
  return algo.nTuple( title, clid );
}
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::nTuple( const GaudiTupleAlg& algo, const GaudiAlg::TupleID& ID,
                                                      const std::string& title, const CLID& clid ) {
  return algo.nTuple( ID, title, clid );
}
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::nTuple( const GaudiTupleAlg& algo, const int ID, const std::string& title,
                                                      const CLID& clid ) {
  return algo.nTuple( ID, title, clid );
}
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::nTuple( const GaudiTupleAlg& algo, const std::string& ID,
                                                      const std::string& title, const CLID& clid ) {
  return algo.nTuple( ID, title, clid );
}
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::evtCol( const GaudiTupleAlg& algo, const std::string& title,
                                                      const CLID& clid ) {
  return algo.evtCol( title, clid );
}
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::evtCol( const GaudiTupleAlg& algo, const GaudiAlg::TupleID& ID,
                                                      const std::string& title, const CLID& clid ) {
  return algo.evtCol( ID, title, clid );
}
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::evtCol( const GaudiTupleAlg& algo, const int ID, const std::string& title,
                                                      const CLID& clid ) {
  return algo.evtCol( ID, title, clid );
}
// ============================================================================
Tuples::Tuple GaudiPython::TupleAlgDecorator::evtCol( const GaudiTupleAlg& algo, const std::string& ID,
                                                      const std::string& title, const CLID& clid ) {
  return algo.evtCol( ID, title, clid );
}
// ============================================================================
// The END
// ============================================================================
