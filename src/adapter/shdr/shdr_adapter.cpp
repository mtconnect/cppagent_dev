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

#define __STDC_LIMIT_MACROS 1
#include "shdr_adapter.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/uuid/name_generator_sha1.hpp>

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>

#include "configuration/config_options.hpp"
#include "device_model/device.hpp"
#include "logging.hpp"

using namespace std;
using namespace std::literals;
using namespace date::literals;

namespace mtconnect::adapter::shdr {
  // Adapter public methods
  ShdrAdapter::ShdrAdapter(boost::asio::io_context &io,
                           pipeline::PipelineContextPtr pipelineContext,
                           const ConfigOptions &options, const boost::property_tree::ptree &block)
    : Adapter("ShdrAdapter", io, options),
      Connector(Source::m_strand, "", 0, 60s),
      m_pipeline(pipelineContext, Source::m_strand),
      m_running(true)
  {
    GetOptions(block, m_options, options);
    AddOptions(block, m_options,
               {
                   {configuration::UUID, string()},
                   {configuration::Manufacturer, string()},
                   {configuration::Station, string()},
                   {configuration::Url, string()},
               });

    m_options.erase(configuration::Host);
    m_options.erase(configuration::Port);

    AddDefaultedOptions(block, m_options,
                        {{configuration::Host, "localhost"s},
                         {configuration::Port, 7878},
                         {configuration::AutoAvailable, false},
                         {configuration::RealTime, false},
                         {configuration::RelativeTime, false}});

    m_server = get<string>(m_options[configuration::Host]);
    m_port = get<int>(m_options[configuration::Port]);

    auto timeout = m_options.find(configuration::LegacyTimeout);
    if (timeout != m_options.end())
      m_legacyTimeout = get<Seconds>(timeout->second);

    stringstream url;
    url << "shdr://" << m_server << ':' << m_port;
    m_url = url.str();

    stringstream identity;
    identity << '_' << m_server << '_' << m_port;
    m_name = identity.str();
    boost::uuids::detail::sha1 sha1;
    sha1.process_bytes(identity.str().c_str(), identity.str().length());
    boost::uuids::detail::sha1::digest_type digest;
    sha1.get_digest(digest);

    identity.str("");
    identity << std::hex << digest[0] << digest[1] << digest[2];
    m_identity = string("_") + (identity.str()).substr(0, 10);

    m_options[configuration::AdapterIdentity] = m_identity;
    m_handler = m_pipeline.makeHandler();
    if (m_pipeline.hasContract())
      m_pipeline.build(m_options);
    auto intv = GetOption<Milliseconds>(options, configuration::ReconnectInterval);
    if (intv)
      m_reconnectInterval = *intv;
  }

  void ShdrAdapter::processData(const string &data)
  {
    if (m_terminator)
    {
      if (data == *m_terminator)
      {
        if (m_handler && m_handler->m_processData)
          m_handler->m_processData(m_body.str(), getIdentity());
        m_terminator.reset();
        m_body.str("");
      }
      else
      {
        m_body << endl << data;
      }

      return;
    }

    if (size_t multi = data.find("--multiline--"); multi != string::npos)
    {
      m_body.str("");
      m_body << data.substr(0, multi);
      m_terminator = data.substr(multi);
      return;
    }

    if (m_handler && m_handler->m_processData)
      m_handler->m_processData(data, getIdentity());
  }

  void ShdrAdapter::stop()
  {
    NAMED_SCOPE("input.adapter.stop");
    // Will stop threaded object gracefully Adapter::thread()
    LOG(debug) << "Waiting for adapter to stop: " << m_url;
    m_running = false;
    close();
    LOG(debug) << "Adapter exited: " << m_url;
  }

  inline bool is_true(const std::string &value) { return value == "yes" || value == "true"; }

  void ShdrAdapter::protocolCommand(const std::string &data)
  {
    static auto pattern = regex("\\*[ ]*([^:]+):[ ]*(.+)");
    smatch match;

    using namespace boost::algorithm;

    if (std::regex_match(data, match, pattern))
    {
      auto command = to_lower_copy(match[1].str());
      auto value = match[2].str();

      ConfigOptions options;

      if (command == "conversionrequired")
        options[configuration::ConversionRequired] = is_true(value);
      else if (command == "relativetime")
        options[configuration::RelativeTime] = is_true(value);
      else if (command == "realtime")
        options[configuration::RealTime] = is_true(value);
      else if (command == "device")
        options[configuration::Device] = value;
      else if (command == "shdrversion")
        options[configuration::ShdrVersion] = stringToInt(value, 1);

      if (options.size() > 0)
        setOptions(options);
      else if (m_handler && m_handler->m_command)
        m_handler->m_command(data, getIdentity());
    }
  }
}  // namespace mtconnect::adapter::shdr
