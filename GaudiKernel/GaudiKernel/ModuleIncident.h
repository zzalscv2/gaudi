#ifndef GAUDIKERNEL_MODULEINCIDENT_H
#define GAUDIKERNEL_MODULEINCIDENT_H

// Include files
#include <string>
#include "GaudiKernel/Incident.h"

/**
 * @class ModuleIncident
 * @brief base class for Module-related incident
 * @author P. Calafiura
 * $Id: ModuleIncident.h,v 1.1 2006/12/10 20:29:17 leggett Exp $
 */
class ModuleIncident : public Incident {
protected:
  /// @name protected structors 
  //@{
  ModuleIncident(std::string source, // Source(service or alg) name)
                 std::string type,   // Type (load, unload, ...)
		         std::string module  // module(DLL) in question
           ) : Incident(std::move(source), std::move(type)), m_module(std::move(module)) { }
  virtual ~ModuleIncident() = default;
  //@}

public:
  /// @name Accessors
  //@{
  using Incident::type;
  using Incident::source;
  ///the model (DLL) being worked on
  const std::string& module() const { return m_module; }
  //@}
private:
  ///the model (DLL) being worked on
  std::string m_module;
};

/**
 * @class ModuleLoadedIncident
 * @brief fired when a module (DLL) is loaded
 * @author P. Calafiura
 * $Id: ModuleIncident.h,v 1.1 2006/12/10 20:29:17 leggett Exp $
 */
class ModuleLoadedIncident : public ModuleIncident {
public:
  ModuleLoadedIncident(std::string source, // Source(service or alg) name)
		               std::string module  // module(DLL) in question
           ) : ModuleIncident(std::move(source), "ModuleLoaded", std::move(module)) { }
};

#endif //GAUDIKERNEL_MODULEINCIDENT_H
