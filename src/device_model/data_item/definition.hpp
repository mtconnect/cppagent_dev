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

#include <regex>

#include "entity.hpp"

namespace mtconnect::device_model::data_item {
  class Definition : public entity::Entity
  {
  public:
    static entity::FactoryPtr getFactory()
    {
      using namespace mtconnect::entity;
      using namespace std;
      static FactoryPtr definition;
      if (!definition)
      {
        auto cell = make_shared<Factory>(Requirements {{"Description", false},
                                                       {"key", false},
                                                       {"keyType", false},
                                                       {"type", false},
                                                       {"subType", false},
                                                       {"units", false}});

        auto cells = make_shared<Factory>(
            Requirements {{"CellDefinition", ENTITY, cell, 1, Requirement::Infinite}});

        auto entry =
            make_shared<Factory>(Requirements {{"Description", false},
                                               {"key", false},
                                               {"keyType", false},
                                               {"type", false},
                                               {"subType", false},
                                               {"units", false},
                                               {"CellDefinitions", ENTITY_LIST, cells, false}});
        entry->setOrder({"Description", "CellDefinitions"});

        auto entries = make_shared<Factory>(
            Requirements {{"EntryDefinition", ENTITY, entry, 1, Requirement::Infinite}});
        definition =
            make_shared<Factory>(Requirements {{"Description", false},
                                               {"EntryDefinitions", ENTITY_LIST, entries, false},
                                               {"CellDefinitions", ENTITY_LIST, cells, false}});
        definition->setOrder({"Description", "EntryDefinitions", "CellDefinitions"});
      }

      return definition;
    }
  };
}  // namespace mtconnect::device_model::data_item
