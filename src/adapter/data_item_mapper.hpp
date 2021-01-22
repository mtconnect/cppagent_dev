// Copyright Copyright 2009-2019, AMT – The Association For Manufacturing Technology (“AMT”)
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

#include "adapter.hpp"
#include "entity/entity.hpp"
#include "shdr_parser.hpp"

#include <chrono>
#include <regex>

namespace mtconnect
{
  namespace adapter
  {
    struct ShdrObservation;

    // Takes a tokenized set of fields and maps them to timestamp and data items
    void MapTokensToDataItem(ShdrObservation &obs, TokenList::const_iterator &token,
                             const TokenList::const_iterator &end, Context &context);

  }  // namespace adapter
}  // namespace mtconnect