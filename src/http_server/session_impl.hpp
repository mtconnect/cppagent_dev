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

#pragma once

#include "session.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "configuration/config_options.hpp"
#include "utilities.hpp"

#include <memory>
#include <functional>

namespace mtconnect
{
  class Printer;

  namespace http_server
  {    
    class SessionImpl : public Session
    {
    public:
      SessionImpl(boost::asio::ip::tcp::socket &socket,
                  const StringList &list,
                  Dispatch dispatch,ErrorFunction error)
      : Session(dispatch, error), m_stream(std::move(socket)), m_fields(list)
      {}
      SessionImpl(const SessionImpl &) = delete;
      virtual ~SessionImpl() {}
      std::shared_ptr<SessionImpl> shared_this_ptr() {
        return std::dynamic_pointer_cast<SessionImpl>(shared_from_this());
      }
      

      void run() override;
      void writeResponse(const Response &response, Complete complete = nullptr)  override;
      void beginStreaming(const std::string &mimeType, Complete complete) override;
      void writeChunk(const std::string &chunk, Complete complete) override;
      void fail(boost::beast::http::status status, const std::string &message,
                boost::system::error_code ec = boost::system::error_code{});
      void close() override;
      
    protected:
      void requested(boost::system::error_code ec, size_t len);
      void sent(boost::system::error_code ec, size_t len);
      void read();

    protected:
      boost::beast::tcp_stream m_stream;
      RequestPtr m_request;
      std::optional<boost::beast::http::request_parser<boost::beast::http::string_body>> m_parser;
      boost::beast::flat_buffer m_buffer;
      Complete    m_complete;
      std::string m_boundary;
      std::string m_mimeType;
      StringList  m_fields;
      Printer    *m_printer{nullptr};
      std::shared_ptr<void> m_response;
      std::shared_ptr<void> m_serializer;
    };
  }
}