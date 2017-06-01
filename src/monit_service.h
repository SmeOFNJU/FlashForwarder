#ifndef VSS_MONIT_SERVICE_H__
#define VSS_MONIT_SERVICE_H__

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>

#include "common.h"

namespace umdgw {

struct NodeInfo {
  std::string id;
  std::string ip;
  std::string port;
};

// The class that implements the http services for the app monitor.
class MonitService {
 public:
  MonitService(const char* uri, const std::vector<NodeInfo>& nodes);
  virtual ~MonitService();

  // start the monit service
  int Start();

  // stop the monit service
  void Stop();

  int64_t start_elapsed() const {
    return start_elapsed_;
  }

  int64_t start_timestamp() const {
    return start_timestamp_;
  }

  const std::vector<NodeInfo>& nodes() const {
    return nodes_;
  }

 private:
  // The method to execute in the worker thread
  void Run();

  // flag indicating whether this worker is being stopped
  volatile bool stopping_;

  // the listen uri
  std::string uri_;

  std::vector<NodeInfo> nodes_;
  int64_t start_elapsed_;
  int64_t start_timestamp_;

  // thread concurrent relative variables
  boost::scoped_ptr<boost::thread> thread_;

};

} // namespace umdgw

#endif // VSS_MONIT_SERVICE_H__