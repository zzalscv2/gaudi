#ifndef GAUDIKERNEL_IDATAHANDLEMETADATA_H
#define GAUDIKERNEL_IDATAHANDLEMETADATA_H 1

#include <string>

#include "GaudiKernel/DataObjID.h"

namespace Gaudi
{
  /// Interface to some class-wide metadata of DataHandle subclasses
  ///
  /// The Property machinery allowing one to set the target of DataHandles via
  /// Python code is centrally managed by the DataHandle base class. However,
  /// it needs some customization points. For example, the Python side must be
  /// able to tell whether a DataHandle is meant for reading or writing, and
  /// what kind of whiteboard it is accessing. And there are more radical
  /// proposals on the horizon stating that DataHandle subclasses should fully
  /// control the generation and parsing of the Python representation of a
  /// DataHandle's configuration.
  ///
  /// Now, where do we put these customization points? They need to be
  /// accessible right from the point in time where the DataHandle base class
  /// constructor is called, because that's when the associated Property is
  /// initialized. So they cannot be virtual methods of DataHandle, as calling
  /// them at that time is illegal. They could be _static_ virtual methods if
  /// C++ had such a thing, but alas that is not the case. So our only option
  /// left is to define, for each DataHandle subclass, an associated global
  /// constant which implements the associated customization points.
  ///
  /// The following interface defines what this global object should look like
  ///
  struct IDataHandleMetadata {
    /// Identifier of the Whiteboard implementation used by the subclass
    ///
    /// We must use an open-ended identifier like a string here, in order
    /// to allow Gaudi-based experiments to use their own whiteboard
    /// implementation without framework-level modifications.
    ///
    virtual std::string whiteBoard() const = 0;

    /// Tell how the whiteboard class will be accessed. The set of access
    /// modes is defined by the framework:
    ///
    /// * The Write access mode provides the ability to insert data once
    ///   in a previously empty whiteboard location.
    /// * The Read access mode provides the ability to read data from a
    ///   previously written whiteboard location any number of times.
    ///
    enum struct AccessMode { Read     = 0, Write, NUM_ACCESS_MODES };
    virtual AccessMode access() const = 0;

    // TODO: Conversion to and from the Python representation of a
    //       DataHandle is probably another thing which will be very
    //       subclass-specific and needed in the base class constructor.
  };
}

#endif
