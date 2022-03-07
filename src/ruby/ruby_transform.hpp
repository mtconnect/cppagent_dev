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

#include <rice/rice.hpp>
#include <rice/stl.hpp>
#include <ruby/thread.h>

#include "pipeline/guard.hpp"
#include "pipeline/transform.hpp"

namespace mtconnect::ruby {
  using namespace mtconnect::pipeline;
  using namespace Rice;
  using namespace Rice::detail;
  using namespace std::literals;
  using namespace date::literals;
  using namespace observation;

  class RubyTransform : public pipeline::Transform, public Director
  {
  public:
    RubyTransform(Object self, const std::string &name)
    : Transform(name), Director(self)

    {
    }
    
    using calldata = pair<RubyTransform*, observation::ObservationPtr>;
    
    static void *gvlCall(void *data)
    {
      using namespace Rice;
      using namespace observation;
      
      calldata *obs = static_cast<calldata*>(data);
      
      return nullptr;
    }
    
    const entity::EntityPtr operator()(const entity::EntityPtr entity) override
    {
      
      using namespace observation;
      
      ObservationPtr obs = dynamic_pointer_cast<Observation>(entity)->copy();
      calldata data(this, obs);

      rb_thread_call_with_gvl(&gvlCall, &data);
      
      return obs;
    }
    
  protected:
    PipelineContract *m_contract;
  };
}
