// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <windows.h>

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/locale.hpp>

#include "common.h"
#include "simple_app.h"
#include "monit_service.h"
#include "include/cef_sandbox_win.h"


// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically if using the required compiler version. Pass -DUSE_SANDBOX=OFF
// to the CMake command-line to disable use of the sandbox.
// Uncomment this line to manually enable sandbox support.
// #define CEF_USE_SANDBOX 1

#if defined(CEF_USE_SANDBOX)
// The cef_sandbox.lib static library is currently built with VS2013. It may not
// link successfully with other VS versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

static const char* kConfigPath = "config.ini";

// Entry point function for all processes.
int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPTSTR    lpCmdLine,
                      int       nCmdShow) {
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);

  // read the config and start the local http server
  // read the file firstly
  std::vector<char> data;
  int res = vss::ReadFile(kConfigPath, &data, true);
  if (0 != res) {
    return res;
  }
  try {
    std::string config_str(&data[0], data.size());
    std::stringstream ss(config_str);
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(ss, pt);

    std::vector<vss::NodeInfo> nodes;
    std::string local_port = pt.get<std::string>("LOCAL.port");
    boost::property_tree::ptree monit_pt = pt.get_child(
      "MONIT");
    boost::property_tree::ptree::iterator it;
    for (it = monit_pt.begin(); it != monit_pt.end(); ++it) {
      std::string id = it->first;
      std::string uri = it->second.get_value<std::string>();
      std::string utf8_id = boost::locale::conv::between(id, "UTF-8",
        "GBK");
      std::vector<std::string> items;
      boost::split(items, uri, boost::is_any_of(":"));
      if (2 != items.size()) continue;
      std::string ip = items[0];
      std::string port = items[1];
      vss::NodeInfo node;
      node.id = utf8_id;
      node.ip = ip;
      node.port = port;
      nodes.push_back(node);
    }

    std::string local_uri = "127.0.0.1:";
    local_uri += local_port;
    vss::MonitService monit_service(local_uri.c_str(), nodes);
    if (0 != monit_service.Start()) {
      return 0;
    }

    // Enable High-DPI support on Windows 7 or newer.
    CefEnableHighDPISupport();

    void* sandbox_info = NULL;

#if defined(CEF_USE_SANDBOX)
    // Manage the life span of the sandbox information object. This is necessary
    // for sandbox support on Windows. See cef_sandbox_win.h for complete details.
    CefScopedSandboxInfo scoped_sandbox;
    sandbox_info = scoped_sandbox.sandbox_info();
#endif

    // Provide CEF with command-line arguments.
    CefMainArgs main_args(hInstance);

    // CEF applications have multiple sub-processes (render, plugin, GPU, etc)
    // that share the same executable. This function checks the command-line and,
    // if this is a sub-process, executes the appropriate logic.
    int exit_code = CefExecuteProcess(main_args, NULL, sandbox_info);
    if (exit_code >= 0) {
      // The sub-process has completed so return here.
      return exit_code;
    }

    // Specify CEF global settings here.
    CefSettings settings;

#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif

    std::string uri = "http://127.0.0.1:";
    uri += local_port;
    uri += "/table_basic.html";
    // SimpleApp implements application-level callbacks for the browser process.
    // It will create the first browser instance in OnContextInitialized() after
    // CEF has initialized.
    CefRefPtr<SimpleApp> app(new SimpleApp(uri.c_str()));

    // Initialize CEF.
    CefInitialize(main_args, settings, app.get(), sandbox_info);

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is
    // called.
    CefRunMessageLoop();

    // Shut down CEF.
    CefShutdown();

    monit_service.Stop();
  } catch (std::runtime_error& err) {
    return VSS_INVALID;
  }
  return 0;
}
