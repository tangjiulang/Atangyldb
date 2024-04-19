#ifndef __COMMON_OS_PIDFILE_H__
#define __COMMON_OS_PIDFILE_H__
#include <string>
namespace common {


//! Generates a PID file for the current component
/**
 * Gets the process ID (PID) of the calling process and writes a file
 * dervied from the input argument containing that value in a system
 * standard directory, e.g. /var/run/progName.pid
 *
 * @param[in] programName as basis for file to write
 * @return    0 for success, error otherwise
 */
int writePidFile(const char *progName);

//! Cleanup PID file for the current component
/**
 * Removes the PID file for the current component
 *
 */
void removePidFile(void);

std::string& getPidPath();

} //namespace common
#endif // __COMMON_OS_PIDFILE_H__