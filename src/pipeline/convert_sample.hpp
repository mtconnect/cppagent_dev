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

#include "transform.hpp"
#include "observation/observation.hpp"
#include "assets/asset.hpp"

namespace mtconnect
{
  namespace pipeline
  {
    class ConvertSample : public Transform
    {
    public:
      
      ConvertSample()
      {
        using namespace observation;
        m_guard = TypeGuard<Sample>() || TypeGuard<Observation>(SKIP);
      }
      const entity::EntityPtr operator()(const entity::EntityPtr entity) override
      {
        using namespace observation;
        using namespace entity;
        auto sample = std::dynamic_pointer_cast<Sample>(entity);
        if (sample)
        {
          auto di = sample->getDataItem();
          if (di->conversionRequired())
          {
            auto ns = sample->copy();
            Value &value = ns->getValue();
            di->convertValue(value);
            
            return next(ns);
          }
        }
        return next(entity);
      }
    };
  }
}  // namespace mtconnect
