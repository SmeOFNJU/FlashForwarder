
#include "common.h"

#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/locale.hpp>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // We only need minimal includes
#include <windows.h>
// MSVC has only _snprintf, not snprintf.
//
// MinGW has both snprintf and _snprintf, but they appear to be different
// functions.  The former is buggy.  When invoked like so:
//   char buffer[32];
//   snprintf(buffer, 32, "%.*g\n", FLT_DIG, 1.23e10f);
// it prints "1.23000e+10".  This is plainly wrong:  %g should never print
// trailing zeros after the decimal point.  For some reason this bug only
// occurs with some input values, not all.  In any case, _snprintf does the
// right thing, so we use it.
#define snprintf _snprintf    // see comment in strutil.cc
#else
#include <sys/time.h>
#endif

#if defined HAVE_CLOCK_GETTIME || defined HAVE_GETHRTIME
#include <time.h>
#endif

namespace umdgw {

int64_t GetCurrentMicroseconds() {
#if defined _WIN32
  //  Get the high resolution counter's accuracy.
  LARGE_INTEGER ticksPerSecond;
  QueryPerformanceFrequency(&ticksPerSecond);

  //  What time is it?
  LARGE_INTEGER tick;
  QueryPerformanceCounter(&tick);

  //  Convert the tick number into the number of seconds
  //  since the system was started.
  double ticks_div = (double)(ticksPerSecond.QuadPart / 1000000.0);
  return (int64_t)(tick.QuadPart / ticks_div);

#elif defined HAVE_CLOCK_GETTIME && defined CLOCK_MONOTONIC

  //  Use POSIX clock_gettime function to get precise monotonic time.
  struct timespec tv;
  int rc = clock_gettime(CLOCK_MONOTONIC, &tv);
  // Fix case where system has clock_gettime but CLOCK_MONOTONIC is not supported.
  // This should be a configuration check, but I looked into it and writing an 
  // AC_FUNC_CLOCK_MONOTONIC seems beyond my powers.
  if (rc != 0) {
    //  Use POSIX gettimeofday function to get precise time.
    struct timeval tv;
    int rc = gettimeofday(&tv, NULL);
    assert(rc == 0);
    return (tv.tv_sec * (int64_t)1000000 + tv.tv_usec);
  }
  return (tv.tv_sec * (int64_t)1000000 + tv.tv_nsec / 1000);

#elif defined HAVE_GETHRTIME

  return (gethrtime() / 1000);

#else

  //  Use POSIX gettimeofday function to get precise time.
  struct timeval tv;
  int rc = gettimeofday(&tv, NULL);
  return (tv.tv_sec * (int64_t)1000000 + tv.tv_usec);

#endif

}

std::string GetCurrentFormattedTime() {
	using namespace boost::posix_time;
	using namespace boost::local_time;

	std::stringstream ss;

	std::locale outputLocale(ss.getloc(), new time_facet("%Y%m%d-%H:%M:%S"));
	ss.imbue(outputLocale);
	ss << microsec_clock::local_time();
	std::string result(ss.str());
	return result;
}

std::string GetCurrentDay() {
  using namespace boost::gregorian;

	date current_date = day_clock::local_day();
  int year = current_date.year();
	int month = current_date.month();
	int day = current_date.day();
  
  char file_name[256] = {0};
  sprintf_s(file_name, "%04d%02d%02d", year, month, day);
  return file_name;
}

void SeedRandom() {
#if defined _WIN32
  int pid = (int) GetCurrentProcessId ();
#else
  int pid = (int) getpid ();
#endif
  srand((unsigned int) (GetCurrentMicroseconds() + pid));
}

uint32_t GenerateRandom() {
  //  Compensate for the fact that rand() returns signed integer.
  uint32_t low = (uint32_t) rand ();
  uint32_t high = (uint32_t) rand ();
  high <<= (sizeof (int) * 8 - 1);
  return high | low;
}

int ReadFile(const char* file_path, std::vector<char>* o_data, 
  bool to_gbk) {
  std::ifstream in (file_path, std::ios::binary);
	if (!in.good ()) {
		return VSS_FAIL;
	}

	in.seekg (0, std::ifstream::end);
	size_t size = (size_t) in.tellg ();
	in.seekg (0, std::ifstream::beg);
	o_data->resize (size);
	in.read (&(*o_data)[0], size);
  if (in.gcount() != size) {
    return VSS_FAIL;
  }

#ifdef _WIN32
  if (size > 3) {
    unsigned char b0 = (*o_data)[0];
    unsigned char b1 = (*o_data)[1];
    unsigned char b2 = (*o_data)[2];
    if (size > 3 && b0 == 0xEF && b1 == 0xBB && b2 == 0xBF) {
      // UTF8 discard 3 bytes
      if (to_gbk) {
        std::string content(&(*o_data)[0] + 3, size - 3);
        std::string gbk_content = boost::locale::conv::between(content, "GBK",
          "UTF-8");
        o_data->assign(gbk_content.begin(), gbk_content.end());
      } else {
        memmove_s(&(*o_data)[0], size - 3, &(*o_data)[0] + 3, size - 3);
        o_data->resize(size - 3);
      }
    } 
  }
#endif
  return 0;
}

int ReadFileWP(const wchar_t* file_path, std::vector<char>* o_data, 
  bool to_gbk) {
  std::ifstream in (file_path, std::ios::binary);
	if (!in.good ()) {
    return VSS_FAIL;
	}

	in.seekg (0, std::ifstream::end);
	size_t size = (size_t) in.tellg ();
	in.seekg (0, std::ifstream::beg);
	o_data->resize (size);
	in.read (&(*o_data)[0], size);
  if (in.gcount() != size) {
    return VSS_FAIL;
  }

#ifdef _WIN32
  if (size > 3) {
    unsigned char b0 = (*o_data)[0];
    unsigned char b1 = (*o_data)[1];
    unsigned char b2 = (*o_data)[2];

    if (size > 3 && b0 == 0xEF && b1 == 0xBB && b2 == 0xBF) {
      // UTF8 discard 3 bytes
      if (to_gbk) {
        std::string content(&(*o_data)[0] + 3, size - 3);
        std::string gbk_content = boost::locale::conv::between(content, "GBK",
          "UTF-8");
        o_data->assign(gbk_content.begin(), gbk_content.end());
      } else {
        memmove_s(&(*o_data)[0], size - 3, &(*o_data)[0] + 3, size - 3);
        o_data->resize(size - 3);
      }
    } 
  }
#endif
  return 0;
}

void ReadSpaceTermString(const char* str, const int size, 
  std::string* o_str) {
  for (int i = 0; i < size; ++i) {
    char ch = str[i];
    if (ch == ' ' || ch == '\0') break;
    o_str->push_back(ch);
  }
}

void WriteSpaceTermString(char* target, const int size, const char* source) {
  bool ended = false;
  for (int i = 0; i < size; ++i) {
    if (ended) {
      target[i] = ' ';
    } else {
      if ('\0' == source[i]) {
        ended = true;
        target[i] = ' ';
      } else {
        target[i] = source[i];
      }
    }
  }
}

void DoParseUri(const std::string& uri, std::string* o_prefix, 
  std::string* o_path, std::map<std::string, std::string>* o_params) {
  std::string uri_str = uri;
  std::string::size_type pos = uri_str.find("://");
  if (std::string::npos != pos) {
    *o_prefix = uri_str.substr(0, pos);
    uri_str = uri_str.substr(pos + 3);
  }
  pos = uri_str.find('?');
  if (std::string::npos != pos) {
    std::string params_str = uri_str.substr(pos + 1);
    std::vector<std::string> parts;
      boost::algorithm::split(parts, params_str, 
        boost::algorithm::is_any_of("&"));
    for (size_t i = 0; i < parts.size(); ++i) {
      std::string part = parts[i];
      std::string::size_type epos = part.find('=');
      if (std::string::npos != epos) {
        std::string name = part.substr(0, epos);
        std::string value = part.substr(epos + 1);
        (*o_params)[name] = value;
      }
    }
    *o_path = uri_str.substr(0, pos);
  } else {
    *o_path = uri_str;
  }
}

std::string DoReplaceDayPlaceholder(const std::string& str) {
  using namespace boost::gregorian;
  // get current day
  date current_date = day_clock::local_day();
  int year = current_date.year();
  int month = current_date.month();
  int day = current_date.day();

  std::string result;
  std::string source = str;
  std::vector<std::string> tokens;
  boost::algorithm::split(tokens, source, boost::algorithm::is_any_of("$"));
  for (size_t i = 0; i < tokens.size(); i++) {
    if (i % 2 == 0) {
      result += tokens[i];
    } else {
      std::string token = tokens[i];
      boost::algorithm::to_lower(token);
      if (token == "yyyymmdd") {
        char file_name[256] = { 0 };
        sprintf_s(file_name, "%04d%02d%02d", year, month, day);
        result += file_name;
      } else if (token == "yymmdd") {
        char file_name[256] = { 0 };
        sprintf_s(file_name, "%02d%02d%02d", year % 100, month, day);
        result += file_name;
      } else if (token == "mmdd") {
        char file_name[256] = { 0 };
        sprintf_s(file_name, "%02d%02d", month, day);
        result += file_name;
      } else if (token == "mdd") {
        char file_name[256] = { 0 };
        sprintf_s(file_name, "%x%02d", month, day);
        result += file_name;
      } else {
        result += "$";
        result += token;
        result += "$";
      }
    }
  }
  return result;
}

} // namespace umdgw