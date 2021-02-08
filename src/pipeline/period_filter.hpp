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

#include "observation/observation.hpp"
#include "transform.hpp"

namespace mtconnect
{
  class Agent;
  namespace pipeline
  {
    class PeriodFilter : public Transform
    {
    public:
      struct State : TransformState
      {
        std::unordered_map<std::string, std::chrono::duration<double>> m_minimumDuration;
        std::unordered_map<std::string, Timestamp> m_lastTimeOffset;
      };

      PeriodFilter(PipelineContextPtr context);
      ~PeriodFilter() override = default;

      bool filterPeriod(const std::string &id, Timestamp &value,
                        const std::chrono::duration<double> md)
      {
        using namespace std;
        auto last = m_state->m_lastTimeOffset.find(id);
        if (last != m_state->m_lastTimeOffset.end())
        {
          auto lv = last->second;
          if (value < (lv + md))
          {
            return true;
          }
          last->second = value;
        }
        else
        {
          m_state->m_lastTimeOffset[id] = value;
        }

        return false;
      }

      const entity::EntityPtr operator()(const entity::EntityPtr entity) override
      {
        using namespace std;
        using namespace observation;
        using namespace entity;

        if (m_state->m_minimumDuration.size() > 0)
        {
          if (auto o = std::dynamic_pointer_cast<Observation>(entity); o)
          {
            auto di = o->getDataItem();
            auto &id = di->getId();
            if (o->isUnavailable())
            {
              m_state->m_lastTimeOffset.erase(id);
              return next(entity);
            }

            if (auto md = m_state->m_minimumDuration.find(id);
                md != m_state->m_minimumDuration.end())
            {
              auto value = o->getTimestamp();
              if (filterPeriod(di->getId(), value, md->second))
                return EntityPtr();
            }
          }
        }
        return next(entity);
      }

      void addMinimumDuration(const std::string &id, const std::chrono::duration<double> &d)
      {
        m_state->m_minimumDuration[id] = d;
      }
      void addMinimumDuration(const DataItem *di, const std::chrono::duration<double> &d)
      {
        m_state->m_minimumDuration[di->getId()] = d;
      }

    protected:
      std::shared_ptr<State> m_state;
      PipelineContract *m_contract;
    };
  }  // namespace pipeline
}  // namespace mtconnect