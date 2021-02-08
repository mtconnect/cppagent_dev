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

#include "period_filter.hpp"

#include "agent.hpp"
#include "device_model/device.hpp"
#include "transform.hpp"

#include <date/date.h>

#include <chrono>

using namespace std;

namespace mtconnect
{
  namespace pipeline
  {
    PeriodFilter::PeriodFilter(PipelineContextPtr context)
      : Transform("PeriodFilter"),
        m_state(context->getSharedState<State>(m_name)),
        m_contract(context->m_contract.get())
    {
      using namespace observation;
      m_guard = TypeGuard<Event, Sample>(RUN) || TypeGuard<Observation>(SKIP);

      // Scan DataItems for rate filters...
      m_contract->eachDataItem([this](const DataItem *di) {
        if (di->hasMinimumPeriod())
          addMinimumDuration(di->getId(), chrono::duration<double>(di->getFilterPeriod()));
      });
    }
  }  // namespace pipeline
}  // namespace mtconnect