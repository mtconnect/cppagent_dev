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

#include <cmath>
#include <date/date.h>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "data_set.hpp"
#include "device_model/component.hpp"
#include "device_model/data_item/data_item.hpp"
#include "entity/entity.hpp"
#include "utilities.hpp"

namespace mtconnect::observation {
  // Types of Observations:
  // Event, Sample, Timeseries, DataSetEvent, Message, Alarm,
  // AssetEvent, ThreeSpaceSmple, Condition, AssetEvent

  class Observation;
  using ObservationPtr = std::shared_ptr<Observation>;
  using ObservationList = std::list<ObservationPtr>;

  class Observation : public entity::Entity
  {
  public:
    using super = entity::Entity;
    using entity::Entity::Entity;

    static entity::FactoryPtr getFactory();
    ~Observation() override = default;
    virtual ObservationPtr copy() const = 0;

    static ObservationPtr make(const DataItemPtr dataItem, const entity::Properties &props,
                               const Timestamp &timestamp, entity::ErrorList &errors);

    static void setProperties(const DataItemPtr dataItem, entity::Properties &props)
    {
      for (auto &prop : dataItem->getObservationProperties())
        props.emplace(prop);
    }

    void setDataItem(const DataItemPtr dataItem)
    {
      m_dataItem = dataItem;
      setProperties(dataItem, m_properties);
    }

    const auto getDataItem() const { return m_dataItem; }
    auto getSequence() const { return m_sequence; }

    void setTimestamp(const Timestamp &ts)
    {
      m_timestamp = ts;
      setProperty("timestamp", m_timestamp);
    }
    auto getTimestamp() const { return m_timestamp; }

    void setSequence(int64_t sequence)
    {
      m_sequence = sequence;
      setProperty("sequence", sequence);
    }

    virtual void makeUnavailable()
    {
      using namespace std::literals;
      m_unavailable = true;
      setProperty("VALUE", "UNAVAILABLE"s);
    }
    bool isUnavailable() const { return m_unavailable; }
    virtual void setEntityName() { Entity::setQName(m_dataItem->getObservationName()); }

    bool operator<(const Observation &another) const
    {
      if ((*m_dataItem) < (*another.m_dataItem))
        return true;
      else if (*m_dataItem == *another.m_dataItem)
        return m_sequence < another.m_sequence;
      else
        return false;
    }

    void clearResetTriggered() { m_properties.erase("resetTriggered"); }

  protected:
    Timestamp m_timestamp;
    bool m_unavailable {false};
    DataItemPtr m_dataItem {nullptr};
    uint64_t m_sequence {0};
  };

  class Sample : public Observation
  {
  public:
    using super = Observation;

    using Observation::Observation;
    static entity::FactoryPtr getFactory();
    ~Sample() override = default;

    ObservationPtr copy() const override { return std::make_shared<Sample>(*this); }
  };

  class ThreeSpaceSample : public Sample
  {
  public:
    using super = Sample;

    using Sample::Sample;
    static entity::FactoryPtr getFactory();
    ~ThreeSpaceSample() override = default;
  };

  class Timeseries : public Sample
  {
  public:
    using super = Sample;

    using Sample::Sample;
    static entity::FactoryPtr getFactory();
    ~Timeseries() override = default;

    ObservationPtr copy() const override { return std::make_shared<Timeseries>(*this); }
  };

  class Condition;
  using ConditionPtr = std::shared_ptr<Condition>;
  using ConditionList = std::list<ConditionPtr>;

  class Condition : public Observation
  {
  public:
    using super = Observation;

    enum Level
    {
      NORMAL,
      WARNING,
      FAULT,
      UNAVAILABLE
    };

    using Observation::Observation;
    static entity::FactoryPtr getFactory();
    ~Condition() override = default;
    ObservationPtr copy() const override { return std::make_shared<Condition>(*this); }

    ConditionPtr getptr() { return std::dynamic_pointer_cast<Condition>(Entity::getptr()); }

    void setLevel(Level level)
    {
      m_level = level;
      setEntityName();
    }

    void setLevel(const std::string &s)
    {
      if (iequals("normal", s))
        setLevel(NORMAL);
      else if (iequals("warning", s))
        setLevel(WARNING);
      else if (iequals("fault", s))
        setLevel(FAULT);
      else if (iequals("unavailable", s))
        setLevel(UNAVAILABLE);
      else
        throw entity::PropertyError("Invalid Condition Level: " + s);
    }

    void normal()
    {
      m_level = NORMAL;
      m_code.clear();
      m_properties.erase("nativeCode");
      m_properties.erase("nativeSeverity");
      m_properties.erase("qualifier");
      m_properties.erase("statistic");
      m_properties.erase("VALUE");
      setEntityName();
    }

    void makeUnavailable() override
    {
      m_unavailable = true;
      m_level = UNAVAILABLE;
      setEntityName();
    }

    void setEntityName() override
    {
      switch (m_level)
      {
        case NORMAL:
          setQName("Normal");
          break;

        case WARNING:
          setQName("Warning");
          break;

        case FAULT:
          setQName("Fault");
          break;

        case UNAVAILABLE:
          setQName("Unavailable");
          break;
      }
    }

    ConditionPtr getFirst()
    {
      if (m_prev)
        return m_prev->getFirst();

      return getptr();
    }

    void getConditionList(ConditionList &list)
    {
      if (m_prev)
        m_prev->getConditionList(list);

      list.emplace_back(getptr());
    }

    ConditionPtr find(const std::string &code)
    {
      if (m_code == code)
        return getptr();

      if (m_prev)
        return m_prev->find(code);

      return nullptr;
    }

    bool replace(ConditionPtr &old, ConditionPtr &_new);
    ConditionPtr deepCopy();
    ConditionPtr deepCopyAndRemove(ConditionPtr &old);

    const std::string &getCode() const { return m_code; }
    Level getLevel() const { return m_level; }
    ConditionPtr getPrev() const { return m_prev; }
    void appendTo(ConditionPtr cond) { m_prev = cond; }

  protected:
    std::string m_code;
    Level m_level {NORMAL};
    ConditionPtr m_prev;
  };

  class Event : public Observation
  {
  public:
    using super = Observation;

    using Observation::Observation;
    static entity::FactoryPtr getFactory();
    ~Event() override = default;
    ObservationPtr copy() const override { return std::make_shared<Event>(*this); }
  };

  class DataSetEvent : public Event
  {
  public:
    using super = Event;

    using Event::Event;
    static entity::FactoryPtr getFactory();
    ~DataSetEvent() override = default;
    ObservationPtr copy() const override { return std::make_shared<DataSetEvent>(*this); }

    const DataSet &getDataSet() const
    {
      const entity::Value &v = getValue();
      return std::get<DataSet>(v);
    }
    void setDataSet(const DataSet &set)
    {
      setValue(set);
      setProperty("count", int64_t(set.size()));
    }
  };

  using DataSetEventPtr = std::shared_ptr<DataSetEvent>;

  class TableEvent : public DataSetEvent
  {
  public:
    using DataSetEvent::DataSetEvent;
    static entity::FactoryPtr getFactory();
    ObservationPtr copy() const override { return std::make_shared<TableEvent>(*this); }
  };

  class AssetEvent : public Event
  {
  public:
    using super = Event;

    using Event::Event;
    static entity::FactoryPtr getFactory();
    ~AssetEvent() override = default;
    ObservationPtr copy() const override { return std::make_shared<AssetEvent>(*this); }

  protected:
  };

  class Message : public Event
  {
  public:
    using super = Event;

    using Event::Event;
    static entity::FactoryPtr getFactory();
    ~Message() override = default;
    ObservationPtr copy() const override { return std::make_shared<Message>(*this); }
  };

  class Alarm : public Event
  {
  public:
    using super = Event;

    using Event::Event;
    static entity::FactoryPtr getFactory();
    ~Alarm() override = default;
    ObservationPtr copy() const override { return std::make_shared<Alarm>(*this); }
  };

  using ObservationComparer = bool (*)(ObservationPtr &, ObservationPtr &);
  inline bool ObservationCompare(ObservationPtr &aE1, ObservationPtr &aE2) { return *aE1 < *aE2; }
}  // namespace mtconnect::observation
