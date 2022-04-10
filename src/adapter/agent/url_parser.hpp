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

#include <boost/asio/ip/address.hpp>
#include <boost/lexical_cast.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <map>
#include <sstream>

namespace mtconnect::adapter::agent {
  using UrlQueryPair = std::pair<std::string, std::string>;

  struct UrlQuery : public std::map<std::string, std::string>
  {
    using std::map<std::string, std::string>::map;
    std::string join() const
    {
      std::stringstream ss;
      bool has_pre = false;

      for (const auto& kv : *this)
      {
        if (has_pre)
          ss << '&';

        ss << kv.first << '=' << kv.second;
        has_pre = true;
      }

      return ss.str();
    }
    
    void merge(UrlQuery query)
    {
      for (const auto& kv : query)
      {
        insert_or_assign(kv.first, kv.second);
      }
    }
  };

  struct Url
  {
    using Host = std::variant<std::string, boost::asio::ip::address>;

    std::string m_protocol;  // http/https

    Host m_host;
    std::optional<std::string> m_username;
    std::optional<std::string> m_password;
    std::optional<int> m_port;
    std::string m_path = "/";
    UrlQuery m_query;
    std::string m_fragment;
    
    struct HostVisitor
    {
      std::string operator()(std::string v) const { return v; }

      std::string operator()(boost::asio::ip::address v) const { return v.to_string(); }
    };

    std::string getHost() const
    {
      return std::visit(HostVisitor(), m_host);
    }
    
    std::string getService() const
    {
      return boost::lexical_cast<std::string>(m_port.value_or(80));
    }

    // the path with query and without fragment
    std::string getTarget() const
    {
      if (m_query.size())
        return m_path + '?' + m_query.join();
      return m_path;
    }

    static Url parse(const std::string_view& url);
  };

}  // namespace mtconnect::adapter::agent