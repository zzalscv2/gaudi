// ============================================================================
#include "GaudiPython/CallbackStreamBuf.h" // this one first, so Python.h is first
// ============================================================================
#include "GaudiKernel/Bootstrap.h"
// ============================================================================
#include <algorithm>
// ============================================================================

// ============================================================================
/// call a python method with a string as an argument
// ============================================================================
int GaudiPython::call_python_method( PyObject* self, const char* method, char* buf )
{
  if ( !self || !method ) {
    return 1;
  }
  PyObject* r = PyObject_CallMethod( self, const_cast<char*>( method ), const_cast<char*>( "s" ), buf );
  if ( !r ) {
    std::string err( "Unsuccessful call to bound Python method" );
    err += method;
    PyErr_SetString( PyExc_TypeError, err.c_str() );
    PyErr_Print();
    return 1;
  }
  Py_DECREF( r );
  return 0;
}

// ============================================================================
/// CallbackStreamBuf constructor
// ============================================================================
GaudiPython::CallbackStreamBuf::CallbackStreamBuf( PyObject* self ) : m_self( self ) {}

// ============================================================================
/// reimplementation of stringbuf::sync()
// ============================================================================
int GaudiPython::CallbackStreamBuf::sync()
{
  size_t length;
  char*  x;
  for ( length = 0, x = pbase(); x < epptr(); ++x, ++length )
    ;
  // getting in a null terminated buffer the characters
  memcpy( m_callbackBuff.data(), pbase(), std::min( length, m_callbackBuff.size() ) );
  m_callbackBuff[std::min( length, m_callbackBuff.size() )] = '\0';
  // calling the python method
  GaudiPython::call_python_method( m_self.get(), "_sync", m_callbackBuff.data() );
  // reseting in/out buffer pointers
  setp( pbase(), pbase() );
  setg( eback(), eback(), eback() );
  return 0;
}
