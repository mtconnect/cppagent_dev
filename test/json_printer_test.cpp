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

// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <libxml/xmlwriter.h>

#include <nlohmann/json.hpp>

#include "adapter/adapter.hpp"
#include "agent.hpp"
#include "configuration/config_options.hpp"
#include "entity.hpp"
#include "entity/json_printer.cpp"
#include "entity/json_printer.hpp"
#include "entity/xml_parser.hpp"
#include "entity/xml_printer.hpp"
#include "xml_printer_helper.hpp"

using json = nlohmann::json;
using namespace std;
using namespace mtconnect;
using namespace mtconnect::entity;

class JsonPrinterTest : public testing::Test
{
protected:
  void SetUp() override {}

  void TearDown() override {}
  FactoryPtr createFileArchetypeFactory()
  {
    auto header = make_shared<Factory>(Requirements {
        Requirement("creationTime", true), Requirement("version", true),
        Requirement("testIndicator", false), Requirement("instanceId", INTEGER, true),
        Requirement("sender", true), Requirement("bufferSize", INTEGER, true),
        Requirement("assetBufferSize", INTEGER, true), Requirement("assetCount", INTEGER, true),
        Requirement("deviceModelChangeTime", true)});

    auto description = make_shared<Factory>(
        Requirements {Requirement("manufacturer", false), Requirement("model", false),
                      Requirement("serialNumber", false), Requirement("station", false),
                      Requirement("VALUE", false)});

    auto dataitem = make_shared<Factory>(Requirements {
        Requirement("name", false),
        Requirement("id", true),
        Requirement("type", true),
        Requirement("subType", false),
        Requirement("statistic", false),
        Requirement("units", false),
        Requirement("nativeUnits", false),
        Requirement("category", true),
        Requirement("coordinateSystem", false),
        Requirement("coordinateSystemId", false),
        Requirement("compositionId", false),
        Requirement("sampleRate", false),
        Requirement("representation", false),
        Requirement("significantDigits", false),
        Requirement("discrete", false),
    });

    auto dataitems = make_shared<Factory>(
        Requirements {Requirement("DataItem", ENTITY, dataitem, 1, Requirement::Infinite)});

    auto component = make_shared<Factory>(Requirements {
        Requirement("id", true),
        Requirement("name", false),
        Requirement("uuid", false),
    });

    auto components = make_shared<Factory>(
        Requirements({Requirement("Component", ENTITY, component, 1, Requirement::Infinite)}));
    components->registerMatchers();
    components->registerFactory(regex(".+"), component);

    component->addRequirements({Requirement("Components", ENTITY_LIST, components, false)});
    component->addRequirements({Requirement("Description", ENTITY, description, false)});
    component->addRequirements({Requirement("DataItems", ENTITY_LIST, dataitems, false)});

    auto device = make_shared<Factory>(*component);
    device->addRequirements(Requirements {
        Requirement("name", true),
        Requirement("uuid", true),
    });

    auto devices = make_shared<Factory>(
        Requirements {Requirement("Device", ENTITY, device, 1, Requirement::Infinite)});
    devices->registerMatchers();

    auto mtconnectDevices = make_shared<Factory>(Requirements {
        Requirement("Header", ENTITY, header, true),
        Requirement("Devices", ENTITY_LIST, devices, true),
    });

    auto root = make_shared<Factory>(
        Requirements {Requirement("MTConnectDevices", ENTITY, mtconnectDevices)});

    return root;
  }

  auto deviceModel()
  {
    auto doc = string {
        "<MTConnectDevices>\n"
        "  <Header creationTime=\"2021-01-07T18:34:15Z\" sender=\"DMZ-MTCNCT\" "
        "instanceId=\"1609418103\" version=\"1.6.0.6\" assetBufferSize=\"8096\" assetCount=\"60\" "
        "bufferSize=\"131072\" deviceModelChangeTime=\"2021-01-07T18:34:15Z\"/>\n"
        "  <Devices>\n"
        "    <Device id=\"d1\" name=\"foo\" uuid=\"xxx\">\n"
        "      <DataItems>\n"
        "        <DataItem category=\"EVENT\" id=\"avail\" name=\"avail\" type=\"AVAILABILITY\"/>\n"
        "        <DataItem category=\"EVENT\" id=\"d1_asset_chg\" type=\"ASSET_CHANGED\"/>\n"
        "        <DataItem category=\"EVENT\" id=\"d1_asset_rem\" type=\"ASSET_REMOVED\"/>\n"
        "      </DataItems\n>"
        "      <Components>\n"
        "        <Systems id=\"s1\">\n"
        "          <Description model=\"abc\">Hey Will</Description>\n"
        "          <Components>\n"
        "            <Electric id=\"e1\"/>\n"
        "            <Heating id=\"h1\"/>\n"
        "          </Components>\n"
        "        </Systems>\n"
        "      </Components>\n"
        "    </Device>\n"
        "  </Devices>\n"
        "</MTConnectDevices>\n"};
    return doc;
  }
};

TEST_F(JsonPrinterTest, Header)
{
  auto root = createFileArchetypeFactory();

  auto doc = deviceModel();

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::JsonPrinter jprinter;

  json jdoc;
  jdoc = jprinter.print(entity);

  auto header = jdoc.at("/MTConnectDevices/Header"_json_pointer);

  ASSERT_EQ("DMZ-MTCNCT", header.at("/sender"_json_pointer).get<string>());
  ASSERT_EQ(8096, header.at("/assetBufferSize"_json_pointer).get<int64_t>());
}

TEST_F(JsonPrinterTest, Devices)
{
  auto root = createFileArchetypeFactory();

  auto doc = deviceModel();

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::JsonPrinter jprinter;

  json jdoc;
  jdoc = jprinter.print(entity);

  auto devices = jdoc.at("/MTConnectDevices/Devices"_json_pointer);

  ASSERT_EQ(1, devices.size());
  ASSERT_EQ("foo", devices.at("/0/Device/name"_json_pointer).get<string>());
}

TEST_F(JsonPrinterTest, Components)
{
  auto root = createFileArchetypeFactory();

  auto doc = deviceModel();

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::JsonPrinter jprinter;

  json jdoc;
  jdoc = jprinter.print(entity);

  auto components = jdoc.at("/MTConnectDevices/Devices/0/Device/Components"_json_pointer);

  ASSERT_EQ(1, components.size());

  auto systems = components.at("/0/Systems"_json_pointer);

  ASSERT_EQ("s1", systems.at("/id"_json_pointer).get<string>());

  ASSERT_EQ("abc", systems.at("/Description/model"_json_pointer).get<string>());
  ASSERT_EQ("Hey Will", systems.at("/Description/value"_json_pointer).get<string>());

  ASSERT_EQ(2, systems.at("/Components"_json_pointer).size());
  ASSERT_EQ("h1", systems.at("/Components/1/Heating/id"_json_pointer).get<string>());
}

TEST_F(JsonPrinterTest, TopLevelDataItems)
{
  auto root = createFileArchetypeFactory();

  auto doc = deviceModel();

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::JsonPrinter jprinter;

  json jdoc;
  jdoc = jprinter.print(entity);

  auto dataitems = jdoc.at("/MTConnectDevices/Devices/0/Device/DataItems"_json_pointer);
  ASSERT_EQ("AVAILABILITY", dataitems.at("/0/DataItem/type"_json_pointer).get<string>());
  ASSERT_EQ("ASSET_CHANGED", dataitems.at("/1/DataItem/type"_json_pointer).get<string>());
  ASSERT_EQ("ASSET_REMOVED", dataitems.at("/2/DataItem/type"_json_pointer).get<string>());
}

TEST_F(JsonPrinterTest, ElementListWithProperty)
{
  auto item = make_shared<Factory>(Requirements {{"itemId", true}});

  auto items = make_shared<Factory>(Requirements {
      {"count", INTEGER, true}, {"CuttingItem", ENTITY, item, 1, Requirement::Infinite}});

  auto lifeCycle = make_shared<Factory>(Requirements {{"CuttingItems", ENTITY_LIST, items, true}});

  auto root = make_shared<Factory>(Requirements {{"Root", ENTITY, lifeCycle, true}});

  string doc = R"DOC(
<Root>
  <CuttingItems count="2">
    <CuttingItem itemId="1"/>
    <CuttingItem itemId="2"/>
  </CuttingItems>
</Root>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::JsonPrinter jprinter;

  json jdoc;
  jdoc = jprinter.print(entity);

  ASSERT_EQ(2, jdoc.at("/Root/CuttingItems/count"_json_pointer).get<int>());
  ASSERT_EQ("1",
            jdoc.at("/Root/CuttingItems/list/0/CuttingItem/itemId"_json_pointer).get<string>());
  ASSERT_EQ("2",
            jdoc.at("/Root/CuttingItems/list/1/CuttingItem/itemId"_json_pointer).get<string>());
}
