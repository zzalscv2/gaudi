#ifndef _FILEREADTOOL_H
#define _FILEREADTOOL_H

#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/IFileAccess.h"

/** @class FileReadTool FileReadTool.h
 *
 *  Basic implementation of the IFileAccess interface.
 *  This tool simply takes a path to a file as url and return the std::istream interface
 *  of std::ifstream.
 *
 *  @author Marco Clemencic
 *  @date 2008-01-18
 */
struct FileReadTool : extends<AlgTool, IFileAccess> {
  /// Standard constructor
  using extends::extends;

  std::unique_ptr<std::istream> open( const std::string& url ) override;

  /// Protocols supported by the instance.
  const std::vector<std::string>& protocols() const override;
};

#endif // _FILEREADTOOL_H
