
/// @file Contains basic utilities used by the rest of the application.

#ifndef VSS_COMMON_H__
#define VSS_COMMON_H__

#include <assert.h>
#include <stdlib.h>
#include <cstddef>
#include <string>
#include <vector>
#include <map>

#if defined(__osf__)
#include <inttypes.h>
// Define integer types needed for message write/read APIs for VC
#elif _MSC_VER && _MSC_VER < 1600
#ifndef int8_t
typedef signed char int8_t;
#endif
#ifndef int16_t
typedef short int16_t;
#endif
#ifndef int32_t
typedef int int32_t;
#endif
#ifndef int64_t
typedef long long int64_t;
#endif
#ifndef uint8_t
typedef unsigned char uint8_t;
#endif
#ifndef uint16_t
typedef unsigned short uint16_t;
#endif
#ifndef uint32_t
typedef unsigned int uint32_t;
#endif
#ifndef uint64_t
typedef unsigned long long uint64_t;
#endif
#else
#include <stdint.h>
#endif // _MSC_VER

namespace umdgw {

/// The vss system error codes definition
#define VSS_INVALID     (-1)                // Invalid parameter, or API misuse
#define VSS_NOMEM       (-2)                // Memory allocation failed
#define VSS_EOVERFLOW   (-3)                // Data exceeds the buffer
#define VSS_TIMEOUT     (-4)                // Operation timeout, try again 
                                            // later
#define VSS_TERM        (-5)                // The underlying stream has
                                            // closed/terminated
#define VSS_EOF         (-6)                // End of the file/stream reached 
                                            // unexpected
#define VSS_ID_EXISTS   (-7)                // The element with the same id 
                                            // has exists
#define VSS_NO_EXISTS   (-8)                // The requested element does not 
                                            // exist
#define VSS_DATA_LOST   (-9)                // Data lost
#define VSS_EINTR       (-10)               // EINTR
#define VSS_BUSY        (-11)               // busy
#define VSS_FAIL        (-100)              // Operation failed caused by the 
                                            // internal logic errors

#undef VSS_DISALLOW_CONSTRUCTORS
#define VSS_DISALLOW_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&);                   \
  void operator=(const TypeName&)

/// The VSS_ARRAYSIZE(arr) macro returns the # of elements in an array arr.
/// The expression is a compile-time constant, and therefore can be
/// used in defining new arrays, for example.
///
/// KMQ_ARRAYSIZE catches a few type errors.  If you see a compiler error
///
///   "warning: division by zero in ..."
///
/// when using VSS_ARRAYSIZE, you are (wrongfully) giving it a pointer.
/// You should only use VSS_ARRAYSIZE on statically allocated arrays.

#define VSS_ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

// Check the return value, we do not log here as the error
// should have been logged when generating the error code
#define VSS_CHECK_RESULT(EXPRESSION) \
{int ret=(EXPRESSION);if(0 != ret) return ret;}

/// Gets current time in microseconds from system startup
int64_t GetCurrentMicroseconds();

/// Gets current formatted time
std::string GetCurrentFormattedTime();

// Getst the current day
std::string GetCurrentDay();

//  Seeds the random number generator.
void SeedRandom();

//  Generates random value.
uint32_t GenerateRandom();

// read content of a file
int ReadFile(const char* file_path, std::vector<char>* o_data, bool to_gbk);
int ReadFileWP(const wchar_t* file_path, std::vector<char>* o_data, 
  bool to_gbk);

// read string with space terminated
void ReadSpaceTermString(const char* str, const int size, std::string* o_str);

// write string with space terminated
void WriteSpaceTermString(char* target, const int size, const char* source);

// help method to parse uri
void DoParseUri(const std::string& uri, std::string* o_prefix, 
  std::string* o_path, std::map<std::string, std::string>* o_params);

// help method to replace date placeholder with current day
std::string DoReplaceDayPlaceholder(const std::string& str);

} // namespace umdgw

#endif // VSS_COMMON_H__