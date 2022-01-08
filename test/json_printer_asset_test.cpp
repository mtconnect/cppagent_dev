//
//  json_printer_asset_test.cpp
//  agent_test
//
//  Created by William Sobel on 3/28/19.
//

#include <cstdio>

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

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "asset/cutting_tool.hpp"
#include "asset/file_asset.hpp"
#include "device_model/data_item/data_item.hpp"
#include "device_model/device.hpp"
#include "entity/xml_parser.hpp"
#include "json_helper.hpp"
#include "json_printer.hpp"
#include "observation/observation.hpp"
#include "rest_sink/checkpoint.hpp"
#include "test_utilities.hpp"
#include "utilities.hpp"
#include "xml_parser.hpp"
#include "xml_printer.hpp"

using json = nlohmann::json;
using namespace std;
using namespace mtconnect;
using namespace mtconnect::rest_sink;
using namespace mtconnect::asset;

class JsonPrinterAssetTest : public testing::Test
{
protected:
  void SetUp() override
  {
    CuttingToolArchetype::registerAsset();
    CuttingTool::registerAsset();
    FileArchetypeAsset::registerAsset();
    FileAsset::registerAsset();

    m_printer = std::make_unique<JsonPrinter>("1.5", true);
    m_parser = std::make_unique<entity::XmlParser>();
  }

  void TearDown() override
  {
    m_printer.reset();
    m_parser.reset();
  }

  AssetPtr parseAsset(const std::string &xml, entity::ErrorList &errors)
  {
    auto entity = m_parser->parse(Asset::getRoot(), xml, "1.7", errors);
    AssetPtr asset;
    for (auto &error : errors)
    {
      cout << error->what() << endl;
    }
    if (entity)
      asset = dynamic_pointer_cast<Asset>(entity);
    return asset;
  }

  std::unique_ptr<JsonPrinter> m_printer;
  std::unique_ptr<entity::XmlParser> m_parser;
};

TEST_F(JsonPrinterAssetTest, AssetHeader)
{
  AssetList asset;
  auto doc = m_printer->printAssets(123, 1024, 10, asset);
  auto jdoc = json::parse(doc);
  auto it = jdoc.begin();

  ASSERT_EQ(string("MTConnectAssets"), it.key());
  ASSERT_EQ(123, jdoc.at("/MTConnectAssets/Header/instanceId"_json_pointer).get<int32_t>());
  ASSERT_EQ(1024, jdoc.at("/MTConnectAssets/Header/assetBufferSize"_json_pointer).get<int32_t>());
  ASSERT_EQ(10, jdoc.at("/MTConnectAssets/Header/assetCount"_json_pointer).get<int32_t>());
}

TEST_F(JsonPrinterAssetTest, CuttingTool)
{
  auto xml = getFile("asset1.xml");
  entity::ErrorList errors;
  AssetPtr a = parseAsset(xml, errors);
  ASSERT_TRUE(a);
  AssetList assetList {a};
  auto doc = m_printer->printAssets(123, 1024, 10, assetList);

  auto jdoc = json::parse(doc);

  auto asset = jdoc.at("/MTConnectAssets/Assets"_json_pointer);
  ASSERT_TRUE(asset.is_array());
  ASSERT_EQ(1_S, asset.size());

  auto cuttingTool = asset.at(0);
  ASSERT_EQ(string("1"), cuttingTool.at("/CuttingTool/serialNumber"_json_pointer).get<string>());
  ASSERT_EQ(string("KSSP300R4SD43L240"),
            cuttingTool.at("/CuttingTool/toolId"_json_pointer).get<string>());
  ASSERT_EQ(string("KSSP300R4SD43L240.1"),
            cuttingTool.at("/CuttingTool/assetId"_json_pointer).get<string>());
  ASSERT_EQ(string("2011-05-11T13:55:22Z"),
            cuttingTool.at("/CuttingTool/timestamp"_json_pointer).get<string>());
  ASSERT_EQ(string("KMT,Parlec"),
            cuttingTool.at("/CuttingTool/manufacturers"_json_pointer).get<string>());
  ASSERT_EQ(string("Cutting tool ..."),
            cuttingTool.at("/CuttingTool/Description"_json_pointer).get<string>());
}

TEST_F(JsonPrinterAssetTest, CuttingToolLifeCycle)
{
  auto xml = getFile("asset1.xml");
  entity::ErrorList errors;
  AssetPtr a = parseAsset(xml, errors);
  ASSERT_TRUE(a);
  AssetList assetList {a};
  auto doc = m_printer->printAssets(123, 1024, 10, assetList);
  auto jdoc = json::parse(doc);

  auto asset = jdoc.at("/MTConnectAssets/Assets"_json_pointer);
  ASSERT_TRUE(asset.is_array());
  ASSERT_EQ(1_S, asset.size());

  auto cuttingTool = asset.at(0);
  auto lifeCycle = cuttingTool.at("/CuttingTool/CuttingToolLifeCycle"_json_pointer);
  ASSERT_TRUE(lifeCycle.is_object());

  auto status = lifeCycle.at("/CutterStatus/0/Status/value"_json_pointer);
  ASSERT_EQ(string("NEW"), status.get<string>());

  auto toolLife = lifeCycle.at("/ToolLife"_json_pointer);
  ASSERT_TRUE(toolLife.is_array());
  auto life = toolLife.at(0);
  ASSERT_TRUE(life.is_object());
  ASSERT_EQ(string("PART_COUNT"), life.at("/type"_json_pointer).get<string>());
  ASSERT_EQ(string("DOWN"), life.at("/countDirection"_json_pointer).get<string>());
  ASSERT_EQ(300.0, life.at("/limit"_json_pointer).get<double>());
  ASSERT_EQ(200.0, life.at("/value"_json_pointer).get<double>());

  auto speed = lifeCycle.at("/ProcessSpindleSpeed"_json_pointer);
  ASSERT_EQ(13300.0, speed.at("/maximum"_json_pointer).get<double>());
  ASSERT_EQ(605.0, speed.at("/nominal"_json_pointer).get<double>());
  ASSERT_EQ(10000.0, speed.at("/value"_json_pointer).get<double>());

  auto feed = lifeCycle.at("/ProcessFeedRate"_json_pointer);
  ASSERT_EQ(222.0, feed.at("/value"_json_pointer).get<double>());
}

TEST_F(JsonPrinterAssetTest, CuttingMeasurements)
{
  auto xml = getFile("asset1.xml");
  entity::ErrorList errors;
  AssetPtr asset = parseAsset(xml, errors);
  ASSERT_TRUE(asset);
  AssetList assetList {asset};
  auto doc = m_printer->printAssets(123, 1024, 10, assetList);
  auto jdoc = json::parse(doc);

  auto lifeCycle =
      jdoc.at("/MTConnectAssets/Assets/0/CuttingTool/CuttingToolLifeCycle"_json_pointer);
  ASSERT_TRUE(lifeCycle.is_object());

  auto measurements = lifeCycle.at("/Measurements"_json_pointer);
  ASSERT_TRUE(measurements.is_array());
  ASSERT_EQ(7_S, measurements.size());

  auto diameter = measurements.at(0);
  ASSERT_TRUE(diameter.is_object());
  ASSERT_EQ("BDX"_S, diameter.at("/BodyDiameterMax/code"_json_pointer).get<string>());
  ASSERT_EQ(73.25, diameter.at("/BodyDiameterMax/value"_json_pointer).get<double>());

  auto length = measurements.at(4);
  ASSERT_TRUE(length.is_object());
  ASSERT_EQ("LF"_S, length.at("/BodyLengthMax/code"_json_pointer).get<string>());
  ASSERT_EQ(120.65, length.at("/BodyLengthMax/nominal"_json_pointer).get<double>());
  ASSERT_EQ(120.404, length.at("/BodyLengthMax/minimum"_json_pointer).get<double>());
  ASSERT_EQ(120.904, length.at("/BodyLengthMax/maximum"_json_pointer).get<double>());
  ASSERT_EQ(120.65, length.at("/BodyLengthMax/value"_json_pointer).get<double>());
}

TEST_F(JsonPrinterAssetTest, CuttingItem)
{
  auto xml = getFile("asset1.xml");
  entity::ErrorList errors;
  AssetPtr asset = parseAsset(xml, errors);
  ASSERT_TRUE(asset);
  AssetList assetList {asset};
  auto doc = m_printer->printAssets(123, 1024, 10, assetList);
  auto jdoc = json::parse(doc);

  cout << doc;

  auto cuttingItems = jdoc.at(
      "/MTConnectAssets/Assets/0/"
      "CuttingTool/CuttingToolLifeCycle/"
      "CuttingItems"_json_pointer);
  ASSERT_EQ(24, cuttingItems["count"].get<int>());
  auto items = cuttingItems["list"];
  ASSERT_TRUE(items.is_array());
  ASSERT_EQ(6_S, items.size());

  auto item = items.at(0);
  ASSERT_TRUE(item.is_object());

  ASSERT_EQ("1-4"_S, item.at("/CuttingItem/indices"_json_pointer).get<string>());
  ASSERT_EQ(string("SDET43PDER8GB"), item.at("/CuttingItem/itemId"_json_pointer).get<string>());
  ASSERT_EQ(string("KC725M"), item.at("/CuttingItem/grade"_json_pointer).get<string>());
  ASSERT_EQ(string("KMT"), item.at("/CuttingItem/manufacturers"_json_pointer).get<string>());
  ASSERT_EQ(string("FLANGE: 1-4, ROW: 1"),
            item.at("/CuttingItem/Locus"_json_pointer).get<string>());

  auto measurements = item.at("/CuttingItem/Measurements"_json_pointer);
  ASSERT_TRUE(measurements.is_array());
  ASSERT_EQ(4_S, measurements.size());

  ASSERT_EQ("RE"_S, measurements.at("/3/CornerRadius/code"_json_pointer).get<string>());
  ASSERT_EQ(0.8, measurements.at("/3/CornerRadius/nominal"_json_pointer).get<double>());
  ASSERT_EQ(0.8, measurements.at("/3/CornerRadius/value"_json_pointer).get<double>());
}

TEST_F(JsonPrinterAssetTest, CuttingToolArchitype)
{
  auto xml = getFile("cutting_tool_archetype.xml");
  entity::ErrorList errors;
  AssetPtr asset = parseAsset(xml, errors);
  ASSERT_TRUE(asset);
  AssetList assetList {asset};
  auto doc = m_printer->printAssets(123, 1024, 10, assetList);
  auto jdoc = json::parse(doc);

  auto tool = jdoc.at(
      "/MTConnectAssets/Assets/0/"
      "CuttingToolArchetype"_json_pointer);
  ASSERT_TRUE(tool.is_object());
  auto def = tool.at("/CuttingToolDefinition"_json_pointer);
  ASSERT_TRUE(def.is_object());
  ASSERT_EQ("EXPRESS"_S, def.at("/format"_json_pointer).get<string>());
  ASSERT_EQ(string("Some Express..."), def.at("/value"_json_pointer).get<string>());
}

// TODO: Test extended asset suppot
#if 0
TEST_F(JsonPrinterAssetTest, UnknownAssetType)
{
  AssetPtr asset(new Asset("BLAH", "Bar", "Some Random Stuff"));
  asset->setTimestamp("2001-12-17T09:30:47Z");
  asset->setDeviceUuid("7800f530-34a9");

  vector<AssetPtr> assetList = {asset};
  auto doc = m_printer->printAssets(123, 1024, 10, assetList);
  auto jdoc = json::parse(doc);

  cout << jdoc.dump(2) << endl;
  auto bar = jdoc.at(
      "/MTConnectAssets/Assets/0/"
      "Bar"_json_pointer);
  ASSERT_TRUE(bar.is_object());
  ASSERT_EQ("BLAH"_S, bar.at("/assetId"_json_pointer).get<string>());
  ASSERT_EQ("2001-12-17T09:30:47Z"_S, bar.at("/timestamp"_json_pointer).get<string>());
  ASSERT_EQ("7800f530-34a9"_S, bar.at("/deviceUuid"_json_pointer).get<string>());
  ASSERT_EQ("Some Random Stuff"_S, bar.at("/text"_json_pointer).get<string>());
}
#endif
