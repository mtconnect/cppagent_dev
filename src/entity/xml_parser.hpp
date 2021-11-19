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

#include <map>
#include <utility>
#include <vector>

#include "entity.hpp"
#include "factory.hpp"
#include "requirement.hpp"
#include "utilities.hpp"

struct _xmlNode;
namespace mtconnect::entity {
  class XmlParser
  {
  public:
    XmlParser() = default;
    ~XmlParser() = default;
    using xmlNodePtr = _xmlNode *;

    static EntityPtr parseXmlNode(FactoryPtr factory, xmlNodePtr node, ErrorList &errors,
                                  bool parseNamespaces = true);
    static EntityPtr parse(FactoryPtr factory, const std::string &document,
                           const std::string &version, ErrorList &errors,
                           bool parseNamespaces = true);
  };
}  // namespace mtconnect::entity
