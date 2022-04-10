//
// Copyright Copyright 2009-2021, AMT – The Association For Manufacturing Technology (“AMT”)
// All rights reserved.
//
//    Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//    See the License for the specific language governing permissions and
//    limitations under the License.
//

#include "agent_adapter.hpp"

#include <boost/algorithm/string.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "configuration/config_options.hpp"
#include "http_session.hpp"
#include "https_session.hpp"
#include "logging.hpp"
#include "session_impl.hpp"

using namespace std;
using namespace mtconnect;

namespace mtconnect::adapter::agent {

  AgentAdapter::AgentAdapter(boost::asio::io_context &io, pipeline::PipelineContextPtr context,
                             const ConfigOptions &options, const boost::property_tree::ptree &block)
    : Adapter("AgentAdapter", io, options), m_pipeline(context, Source::m_strand)
  {
    GetOptions(block, m_options, options);
    AddOptions(block, m_options,
               {{configuration::UUID, string()},
                {configuration::Manufacturer, string()},
                {configuration::Station, string()},
                {configuration::Url, string()}});

    AddDefaultedOptions(block, m_options,
                        {{configuration::Host, "localhost"s},
                         {configuration::Port, 5000},
                         {configuration::AutoAvailable, false},
                         {configuration::RealTime, false},
                         {configuration::RelativeTime, false}});

    auto urlOpt = GetOption<std::string>(m_options, configuration::Url);
    if (urlOpt)
    {
      m_url = Url::parse(*urlOpt);
    }
    else
    {
      m_url.m_protocol = "http";
      m_url.m_host = *GetOption<string>(m_options, configuration::Host);
      m_url.m_port = GetOption<int>(m_options, configuration::Port);
      m_url.m_path = GetOption<string>(m_options, configuration::Device).value_or("/");
    }
  }

  AgentAdapter::~AgentAdapter() {}

  bool AgentAdapter::start()
  {
    if (m_url.m_protocol == "https")
    {
      // The SSL context is required, and holds certificates
      ssl::context ctx {ssl::context::tlsv12_client};
      ctx.set_verify_mode(ssl::verify_peer);
      m_session = make_shared<HttpsSession>(m_strand, ctx);
    }
    else if (m_url.m_protocol == "https")
    {
      m_session = make_shared<HttpSession>(m_strand);
    }
    else
    {
      LOG(error) << "Unknown protocol: " << m_url.m_protocol;
      return false;
    }

    m_session->connect(m_url, [this](boost::beast::error_code ec) {
      
    });

    return true;
  }
}  // namespace mtconnect::adapter::agent