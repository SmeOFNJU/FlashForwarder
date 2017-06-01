
#include "monit_service.h"

#include <boost/date_time.hpp>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <mongoose/mongoose.h>
#include <boost/algorithm/string.hpp>
#include <string>
#include <vector>

namespace umdgw {

static std::string ToString(const std::vector<NodeInfo>& nodes) {
  rapidjson::StringBuffer output;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(output);
  writer.StartObject();

  writer.String("data");
  writer.StartArray();
  for (size_t i = 0; i < nodes.size(); ++i) {
    writer.StartObject();
    const NodeInfo& node = nodes[i];
    writer.String("id");
    writer.String(node.id.c_str());
    writer.String("ip");
    writer.String(node.ip.c_str());
    writer.String("port");
    writer.String(node.port.c_str());
    writer.EndObject();
  }
  writer.EndArray();

  writer.EndObject();
  return output.GetString();
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
  struct http_message *hm = (struct http_message *) ev_data;
  MonitService* ms = reinterpret_cast<MonitService*>(nc->mgr->user_data);
  // the http options
  struct mg_serve_http_opts http_opts;
  memset(&http_opts, 0, sizeof(struct mg_serve_http_opts));
  http_opts.document_root = "./web";

  switch (ev) {
  case MG_EV_HTTP_REQUEST:
    {
      std::vector<std::string> query_array;
      std::string query_ = "";
      if (mg_vcmp(&hm->uri, "/list") == 0) {

        std::string json = ToString(ms->nodes());
        if (mg_vcmp(&hm->query_string, "") != 0) {
          query_ = hm->query_string.p;
          boost::split(query_array, query_, boost::is_any_of("=& "));
          query_ = query_array[1];
          json = "(" + json + ")";
        }
        mg_printf(nc, "HTTP/1.0 200 OK\r\nContent-Length: %d\r\n"
          "Content-Type: application/json\r\n\r\n%s%s",
          static_cast<int>(json.size()) + static_cast<int>(query_.size())
          , query_.c_str(), json.c_str());
      } else {
        mg_serve_http(nc, hm, http_opts);  /* Serve static content */
      }
    }
    break;

  default:
    break;
  }
}

MonitService::MonitService(const char* uri, const std::vector<NodeInfo>& nodes)
  : uri_(uri)
  , nodes_(nodes)
  , stopping_(false) {
  // nothing
}

MonitService::~MonitService() {
  // nothing
}

int MonitService::Start() {
  thread_.reset(new boost::thread(boost::bind(&MonitService::Run, this)));
  return 0;
}

void MonitService::Stop() {
  stopping_ = true;
  if (thread_) {
    thread_->join();
    thread_.reset();
  }
}

void MonitService::Run() {
  struct mg_mgr mgr;
  struct mg_connection *nc;

  using namespace boost::gregorian;
  using namespace boost::posix_time;
  using namespace boost::local_time;

  static ptime const epoch(date(1970, 1, 1));

  start_elapsed_ = GetCurrentMicroseconds();
  start_timestamp_ = (microsec_clock::local_time() - epoch).
    total_microseconds();

  mg_mgr_init(&mgr, this);
  nc = mg_bind(&mgr, uri_.c_str(), ev_handler);
  mg_set_protocol_http_websocket(nc);

  if (NULL == nc) {
    //VSS_LOG(logger_, ILogger::LOG_ERROR) << "Monit service start failed!";
  } else {
    //VSS_LOG(logger_, ILogger::LOG_INFO) << "Monit service started : " << uri_;

    while (!stopping_) {
      mg_mgr_poll(&mgr, 500);
    }
  }
  
  mg_mgr_free(&mgr);
}

} // namespace umdgw