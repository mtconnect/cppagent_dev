// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "adapter/adapter.hpp"
#include "agent.hpp"
#include "entity.hpp"
#include "entity/xml_parser.hpp"
#include "json_helper.hpp"

using json = nlohmann::json;
using namespace std;
using namespace mtconnect;
using namespace mtconnect::entity;

class EntityParserTest : public testing::Test
{
protected:
  void SetUp() override
  {  // Create an agent with only 16 slots and 8 data items.
  }

  void TearDown() override {}

  FactoryPtr components()
  {
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

    auto device = make_shared<Factory>(*component);
    device->addRequirements(Requirements {
        Requirement("name", true),
        Requirement("uuid", true),
    });

    auto root = make_shared<Factory>(Requirements {Requirement("Device", ENTITY, device)});

    return root;
  }
};

TEST_F(EntityParserTest, TestParseSimpleDocument)
{
  auto fileProperty =
      make_shared<Factory>(Requirements({Requirement("name", true), Requirement("VALUE", true)}));

  auto fileProperties = make_shared<Factory>(
      Requirements({Requirement("FileProperty", ENTITY, fileProperty, 1, Requirement::Infinite)}));
  fileProperties->registerMatchers();

  auto fileComment = make_shared<Factory>(
      Requirements({Requirement("timestamp", true), Requirement("VALUE", true)}));

  auto fileComments = make_shared<Factory>(
      Requirements({Requirement("FileComment", ENTITY, fileComment, 1, Requirement::Infinite)}));
  fileComments->registerMatchers();

  auto fileArchetype = make_shared<Factory>(Requirements {
      Requirement("assetId", true), Requirement("deviceUuid", true), Requirement("timestamp", true),
      Requirement("removed", false), Requirement("name", true), Requirement("mediaType", true),
      Requirement("applicationCategory", true), Requirement("applicationType", true),
      Requirement("FileComments", ENTITY_LIST, fileComments, false),
      Requirement("FileProperties", ENTITY_LIST, fileProperties, false)});

  auto root =
      make_shared<Factory>(Requirements {Requirement("FileArchetype", ENTITY, fileArchetype)});

  auto doc = string {
      "<FileArchetype name='xxxx' assetId='uuid' deviceUuid='duid' timestamp='2020-12-01T10:00Z' \n"
      "     mediaType='json' applicationCategory='ASSEMBLY' applicationType='DATA' >\n"
      "  <FileProperties>\n"
      "    <FileProperty name='one'>Round</FileProperty>\n"
      "    <FileProperty name='two'>Flat</FileProperty>\n"
      "  </FileProperties>\n"
      "</FileArchetype>"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  ASSERT_EQ("FileArchetype", entity->getName());
  ASSERT_EQ("xxxx", get<string>(entity->getProperty("name")));
  ASSERT_EQ("uuid", get<string>(entity->getProperty("assetId")));
  ASSERT_EQ("2020-12-01T10:00Z", get<string>(entity->getProperty("timestamp")));
  ASSERT_EQ("json", get<string>(entity->getProperty("mediaType")));
  ASSERT_EQ("ASSEMBLY", get<string>(entity->getProperty("applicationCategory")));
  ASSERT_EQ("DATA", get<string>(entity->getProperty("applicationType")));

  auto fps = entity->getList("FileProperties");
  ASSERT_TRUE(fps);
  ASSERT_EQ(2, fps->size());

  auto it = fps->begin();
  ASSERT_EQ("FileProperty", (*it)->getName());
  ASSERT_EQ("one", get<string>((*it)->getProperty("name")));
  ASSERT_EQ("Round", get<string>((*it)->getProperty("VALUE")));

  it++;
  ASSERT_EQ("FileProperty", (*it)->getName());
  ASSERT_EQ("two", get<string>((*it)->getProperty("name")));
  ASSERT_EQ("Flat", get<string>((*it)->getProperty("VALUE")));
}

TEST_F(EntityParserTest, TestRecursiveEntityLists)
{
  auto root = components();

  auto doc = string {
      "<Device id='d1' name='foo' uuid='xxx'>\n"
      "  <Components>\n"
      "    <Systems id='s1'>\n"
      "       <Components>\n"
      "         <Electric id='e1'/>\n"
      "         <Heating id='h1'/>\n"
      "       </Components>\n"
      "    </Systems>\n"
      "  </Components>\n"
      "</Device>"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  ASSERT_EQ("Device", entity->getName());
  ASSERT_EQ("d1", get<string>(entity->getProperty("id")));
  ASSERT_EQ("foo", get<string>(entity->getProperty("name")));
  ASSERT_EQ("xxx", get<string>(entity->getProperty("uuid")));

  auto l = entity->getList("Components");
  ASSERT_TRUE(l);
  ASSERT_EQ(1, l->size());

  auto systems = l->front();
  ASSERT_EQ("Systems", systems->getName());
  ASSERT_EQ("s1", get<string>(systems->getProperty("id")));

  auto sl = systems->getList("Components");
  ASSERT_TRUE(sl);
  ASSERT_EQ(2, sl->size());

  auto sli = sl->begin();

  ASSERT_EQ("Electric", (*sli)->getName());
  ASSERT_EQ("e1", get<string>((*sli)->getProperty("id")));

  sli++;
  ASSERT_EQ("Heating", (*sli)->getName());
  ASSERT_EQ("h1", get<string>((*sli)->getProperty("id")));
}

TEST_F(EntityParserTest, TestRecursiveEntityListFailure)
{
  auto root = components();

  auto doc = string {
      "<Device id='d1' name='foo'>\n"
      "  <Components>\n"
      "    <Systems id='s1'>\n"
      "       <Components>\n"
      "         <Electric id='e1'/>\n"
      "         <Heating id='h1'/>\n"
      "       </Components>\n"
      "    </Systems>\n"
      "  </Components>\n"
      "</Device>"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(1, errors.size());
  ASSERT_FALSE(entity);
  ASSERT_EQ(string("Device(uuid): Property uuid is required and not provided"),
            errors.front()->what());
}

TEST_F(EntityParserTest, TestRecursiveEntityListMissingComponents)
{
  auto root = components();

  auto doc = string {
      "<Device id='d1' uuid='xxx' name='foo'>\n"
      "  <Components>\n"
      "    <Systems id='s1'>\n"
      "       <Components>\n"
      "       </Components>\n"
      "    </Systems>\n"
      "  </Components>\n"
      "</Device>"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(2, errors.size());
  ASSERT_TRUE(entity);
  ASSERT_EQ(string("Components(Component): Entity list requirement Component must have at least 1 "
                   "entries, 0 found"),
            errors.front()->what());
  ASSERT_EQ("Device", entity->getName());
  ASSERT_EQ("d1", get<string>(entity->getProperty("id")));
  ASSERT_EQ("foo", get<string>(entity->getProperty("name")));
  ASSERT_EQ("xxx", get<string>(entity->getProperty("uuid")));

  auto l = entity->getList("Components");
  ASSERT_TRUE(l);
  ASSERT_EQ(1, l->size());

  auto systems = l->front();
  ASSERT_EQ("Systems", systems->getName());
  ASSERT_EQ("s1", get<string>(systems->getProperty("id")));

  auto sl = systems->getList("Components");
  ASSERT_FALSE(sl);
}

TEST_F(EntityParserTest, TestRawContent)
{
  auto definition =
      make_shared<Factory>(Requirements({Requirement("format", false), Requirement("RAW", true)}));

  auto root =
      make_shared<Factory>(Requirements({Requirement("Definition", ENTITY, definition, true)}));

  auto doc = R"DOC(
<Definition format="XML">
  <SomeContent with="stuff">
    And some text
  </SomeContent>
  <AndMoreContent/>
  And random text as well.
</Definition>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);

  auto expected = R"DOC(<SomeContent with="stuff">
    And some text
  </SomeContent><AndMoreContent/>
  And random text as well.
)DOC";

  ASSERT_EQ("XML", get<string>(entity->getProperty("format")));
  ASSERT_EQ(expected, get<string>(entity->getProperty("RAW")));
}

TEST_F(EntityParserTest, check_proper_line_truncation)
{
  auto description = make_shared<Factory>(
      Requirements {Requirement("manufacturer", false), Requirement("model", false),
                    Requirement("serialNumber", false), Requirement("station", false),
                    Requirement("VALUE", false)});

  auto root = make_shared<Factory>(Requirements {{{"Description", ENTITY, description, false}}});

  auto doc = R"DOC(
  <Description>
      And some text
  </Description>
)DOC";

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ("Description", entity->getName());
  ASSERT_EQ("And some text", entity->getValue<string>());
}
