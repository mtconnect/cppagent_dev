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

#include "embedded.hpp"

#include <boost/python.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/str.hpp>
#include <boost/python/tuple.hpp>

#include <iostream>
#include <string>

#include "adapter/adapter.hpp"
#include "agent.hpp"
#include "device_model/device.hpp"
#include "entity.hpp"
#include "pipeline/guard.hpp"
#include "pipeline/transform.hpp"

using namespace std;

namespace mtconnect::python {
  using namespace boost::python;
  namespace py = boost::python;
  using namespace entity;
  using namespace pipeline;
  using namespace observation;

#define None object(detail::borrowed_reference(Py_None))

  struct Context
  {
    object m_source;
    object m_device;
    object m_transform;
    object m_pipeline;
  };
  using ContextPtr = Context *;

  struct Wrapper
  {
    virtual ~Wrapper() {}
    ContextPtr m_context;
  };

  static object wrap(EntityPtr entity, ContextPtr context);
  
  class PythonObservationTransform : public Transform
  {
  public:
    PythonObservationTransform(const string &pythonFunction) : Transform(pythonFunction)
    {
      m_guard = TypeGuard<Observation>(RUN);
    }
    
    const entity::EntityPtr operator()(const entity::EntityPtr entity) override
    {
      using namespace entity;
      
      auto observation = dynamic_pointer_cast<Observation>(entity);
      dict props;
      
      // Copy the properties to the dictionary
      for (const auto& [name, value] : observation->getProperties())
      {
        insertDictEntry(props, name, value);
      }
      
      // Call the transformation function
      
      // Copy the value back to the entity
      auto result = observation->copy();

      return next(result);
    }

  public:
    object m_function;

  protected:
    void insertDictEntry(dict &props, const std::string &name, const Value &value)
    {
      visit(overloaded {
        [&props, &name](const std::string &s) { props[name] = s; },
        [&props, &name](const int64_t v) { props[name] = v; },
        [&props, &name](const double v) { props[name] = v; },
        [&props, &name](const bool v) { props[name] = v; },
        [&props, &name](const Vector &v) {
          props[name] = v;
        },
        [](const auto &v) {}
      }, value);
    }
  };
  


  Embedded::Embedded(Agent *agent, const ConfigOptions &options)
    : m_agent(agent), m_context(new Context()), m_options(options)
  {
    try
    {
      PyConfig config;
      PyConfig_InitPythonConfig(&config);
      PyConfig_Read(&config);
      config.dev_mode = true;
      PyWideStringList_Append(&config.module_search_paths,
                              L"/Users/will/projects/MTConnect/agent/cppagent_dev/modules");
      Py_InitializeFromConfig(&config);
      PyConfig_Clear(&config);

      wstring path(Py_GetPath());
      string s(path.begin(), path.end());
      cout << "Path: " << s << endl;

      object main_module = import("__main__");
      object main_namespace = main_module.attr("__dict__");

      
      // Py_RunMain();
    }

    catch (error_already_set const &)
    {
      PyErr_Print();
    }
  }

  Embedded::~Embedded() { delete m_context; }
}  // namespace mtconnect::python
