// -*- C++ -*-
// AID-GENERATED
// =========================================================================
// This class was generated by AID - Abstract Interface Definition
// DO NOT MODIFY, but use the org.freehep.aid.Aid utility to regenerate it.
// =========================================================================
#ifndef GAUDISVC__IAXIS_H
#define GAUDISVC__IAXIS_H 1
/// @FIXME: AIDA interfaces visibility
#include "AIDA_visibility_hack.h"

//  This file is part of the AIDA library
//  Copyright (C) 2002 by the AIDA team.  All rights reserved.
//  This library is free software and under the terms of the
//  GNU Library General Public License described in the LGPL.txt

#include "AIDA/IAxis.h"

#include "TAxis.h"

namespace Gaudi {

  /**
  * An IAxis represents a binned histogram axis. A 1D Histogram would have
  * one Axis representing the X axis, while a 2D Histogram would have two
  * axes representing the X and Y Axis.
  *
  * @author The AIDA team (http://aida.freehep.org/)
  *
  */
  class Axis  : public AIDA::IAxis
  {
  public:

    typedef Axis self;

    static int toRootIndex(int index, int nbins) {
      if (index==AIDA::IAxis::OVERFLOW_BIN) return nbins+1;
      if (index==AIDA::IAxis::UNDERFLOW_BIN) return 0;
      return index+1;
    }

    static int toAidaIndex(int index, int bins) {
      if ( index == bins + 1 ) return AIDA::IAxis::OVERFLOW_BIN ;
      if ( index == 0 ) return AIDA::IAxis::UNDERFLOW_BIN ;
      return index - 1 ;
    }

    /**
     * Convert a AIDA bin number on the axis to the ROOT bin number.
     * @param index TheAIDA bin number
     * @return      The corresponding ROOT bin number.
     *
     */
    int rIndex(int index) const { return toRootIndex(index, bins());}

    /**
     * Convert a ROOT bin number on the axis to the AIDA bin number.
     * @param index The ROOT bin number: 1 to bins() for the in-range bins or bins()+1 for OVERFLOW or 0 for UNDERFLOW.
     * @return      The corresponding AIDA bin number.
     *
     */
    int aIndex( int index ) const { return toAidaIndex(index, bins()); }

  public:

    Axis() = default;

    explicit Axis ( TAxis * itaxi ) : taxis_(itaxi) {}

    void initialize (TAxis * itaxi , bool ) { taxis_ = itaxi; }

    /// Destructor.
    virtual ~Axis() = default;

    /**
    * Check if the IAxis has fixed binning, i.e. if all the bins have the same width.
    * @return <code>true</code> if the binning is fixed, <code>false</code> otherwise.
    *
    */
    virtual bool isFixedBinning() const
    {
      return 0 == taxis_ ? true : !taxis_->IsVariableBinSize() ;
    }

    /**
    * Get the lower edge of the IAxis.
    * @return The IAxis's lower edge.
    *
    */
    virtual double lowerEdge() const { return taxis().GetXmin();}

    /**
    * Get the upper edge of the IAxis.
    * @return The IAxis's upper edge.
    *
    */
    virtual double upperEdge() const { return taxis().GetXmax();}

    /**
    * The number of bins (excluding underflow and overflow) on the IAxis.
    * @return The IAxis's number of bins.
    *
    */
    virtual int bins() const { return taxis().GetNbins();}

    /**
    * Get the lower edge of the specified bin.
    * @param index The bin number: 0 to bins()-1 for the in-range bins or OVERFLOW or UNDERFLOW.
    * @return      The lower edge of the corresponding bin; for the underflow bin this is <tt>Double.NEGATIVE_INFINITY</tt>.
    *
    */
    virtual double binLowerEdge(int index) const { return taxis().GetBinLowEdge(rIndex(index));}
    /**
    * Get the upper edge of the specified bin.
    * @param index The bin number: 0 to bins()-1 for the in-range bins or OVERFLOW or UNDERFLOW.
    * @return      The upper edge of the corresponding bin; for the overflow bin this is <tt>Double.POSITIVE_INFINITY</tt>.
    *
    */
    virtual double binUpperEdge(int index) const { return taxis().GetBinUpEdge(rIndex(index));}

    /**
    * Get the width of the specified bin.
    * @param index The bin number: 0 to bins()-1) for the in-range bins or OVERFLOW or UNDERFLOW.
    * @return      The width of the corresponding bin.
    *
    */
    virtual double binWidth(int index) const { return taxis().GetBinWidth(rIndex(index));}

    /**
    * Convert a coordinate on the axis to a bin number.
    * If the coordinate is less than the lowerEdge UNDERFLOW is returned; if the coordinate is greater or
    * equal to the upperEdge OVERFLOW is returned.
    * @param coord The coordinate to be converted.
    * @return      The corresponding bin number.
    *
    */

    virtual int coordToIndex(double coord) const
    {
      return aIndex( taxis().FindBin(coord) );
    }

    /**
    *
    */
    TAxis & taxis() const { return *me().taxis_;}

  private:

  private:

    self & me() const { return const_cast<self&>(*this);}


    TAxis * taxis_ = nullptr;

  }; // class

} // namespace Gaudi

#endif /* ifndef AIDA_IAXIS_H */
