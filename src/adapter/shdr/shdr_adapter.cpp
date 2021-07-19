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

namespace mtconnect {
  namespace adapter {
    namespace shdr {
      // Adapter public methods
      ShdrAdapter::ShdrAdapter(boost::asio::io_context &context, const string &server,
                               const unsigned int port, const ConfigOptions &options,
                               std::unique_ptr<ShdrPipeline> &pipeline)
        : Connector(context, server, port, 60s),
          Adapter("Adapter", options),
          m_pipeline(std::move(pipeline)),
          m_running(true)
      {
        auto timeout = options.find(configuration::LegacyTimeout);
        if (timeout != options.end())
          m_legacyTimeout = get<Seconds>(timeout->second);

        stringstream url;
        url << "shdr://" << server << ':' << port;
        m_url = url.str();

        stringstream identity;
        identity << '_' << server << '_' << port;
        m_name = identity.str();
        boost::uuids::detail::sha1 sha1;
        sha1.process_bytes(identity.str().c_str(), identity.str().length());
        boost::uuids::detail::sha1::digest_type digest;
        sha1.get_digest(digest);

        identity.str("");
        identity << std::hex << digest[0] << digest[1] << digest[2];
        m_identity = string("_") + (identity.str()).substr(0, 10);

        m_options[configuration::AdapterIdentity] = m_identity;
        m_handler = m_pipeline->makeHandler();
        if (m_pipeline->hasContract())
          m_pipeline->build(m_options);
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

        if (std::regex_match(data, match, pattern))
        {
          auto command = match[1].str();
          auto value = match[2].str();

          ConfigOptions options;

          if (command == "conversionRequired")
            options[configuration::ConversionRequired] = is_true(value);
          else if (command == "relativeTime")
            options[configuration::RelativeTime] = is_true(value);
          else if (command == "realTime")
            options[configuration::RealTime] = is_true(value);
          else if (command == "device")
            options[configuration::Device] = value;
          else if (command == "shdrVersion")
            options[configuration::ShdrVersion] = value;

          if (options.size() > 0)
            setOptions(options);
          else if (m_handler && m_handler->m_command)
            m_handler->m_command(data, getIdentity());
        }
      }
    }  // namespace shdr
  }    // namespace adapter
}  // namespace mtconnect