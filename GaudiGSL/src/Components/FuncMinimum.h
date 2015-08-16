#ifndef ALGTOOLS_FUNCMINIMUM_H
#define ALGTOOLS_FUNCMINIMUM_H 1
// Include files
// from CLHEP
#include "CLHEP/GenericFunctions/GenericFunctions.hh"
#include "CLHEP/GenericFunctions/Argument.hh"
#include "CLHEP/GenericFunctions/AbsFunction.hh"
#include "CLHEP/Matrix/SymMatrix.h"
// from Gaudi
#include "GaudiAlg/GaudiTool.h"
//from GSL
#include "gsl/gsl_multimin.h"

// local
#include "GaudiGSL/IFuncMinimum.h"

/** @class  FuncMinimum FuncMinimum.h
 *  The simplest concrete implementation of IFuncMinimum interface
 *  @see    GaudiGSL/IFuncMinimum.h
 *  @author Kirill Miklyaev kirillm@iris1.itep.ru
 *  @date   2002-09-14
 */

class FuncMinimum : public extends1<GaudiTool, IFuncMinimum> {
public:
  typedef std::vector<const GenFunc*> Gradient;
public:

  /** Find minimum of the function "GenFunc"
   *  @see IFuncMinimum.h
   *  @return StatusCode
   */
  StatusCode minimum( const GenFunc& func  ,
                      Arg&           arg   ) const override;

  /** Find minimum of the function "GenFunc"
   *  @see IFuncMinimum.h
   *  @return StatusCode
   */
  StatusCode minimum( const GenFunc& func  ,
                      Arg&           arg   ,
                      Covariance&    covar ) const override;

  /// Overriding initialize
  StatusCode initialize () override;
  StatusCode finalize   () override;

  ~FuncMinimum( ) override = default; ///< Destructor

  class FuncMinimumMisc final
  {
  public:
    /** standard constructor
     *
     */
    FuncMinimumMisc (const GenFunc& func ,
                     Arg&           arg  );

  public:
    inline const Arg&       argument () const { return m_argum ;}
    inline       Arg&       argument ()       { return m_argum ;}
    inline const GenFunc*   equation () const { return m_eq    ;}
    inline const Gradient&  gradient () const { return m_grad  ;}
  private:
    // default constructor is disabled
    FuncMinimumMisc () = delete;
    // copy constructor is disabled
    FuncMinimumMisc           ( const FuncMinimumMisc& ) = delete;
    // assignment operator is disabled
    FuncMinimumMisc& operator=( const FuncMinimumMisc& ) = delete;
  private:
    Arg              m_argum ;
    const GenFunc*   m_eq    ;
    Gradient         m_grad  ;
  };


  /** Standard constructor
   *  @see GaudiTool
   *  @param type    tool type
   *  @param name    tool name
   *  @param parent  parent of the tool
   */
  FuncMinimum( const std::string& type,
               const std::string& name,
               const IInterface* parent);

private:

  /// default constructor is private
  FuncMinimum() = delete;
  /// copy constructor is private
  FuncMinimum           ( const FuncMinimum& ) = delete;
  /// assignment operator is
  FuncMinimum& operator=( const FuncMinimum& ) = delete;

private:

  std::string m_algType         = "conjugate_fr";
  double      m_max_iter        = 200;
  double      m_norm_gradient   = 1.0e-10;
  double      m_step_size       = 0.01;
  double      m_tol             = 1e-10;
  const gsl_multimin_fdfminimizer_type* m_type = nullptr;
};

// ============================================================================
// The END
// ============================================================================
#endif // ALGTOOLS_FUNCMINIMUM_H
// ============================================================================
