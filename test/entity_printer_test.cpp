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

#include "adapter/adapter.hpp"
#include "agent.hpp"
#include "entity.hpp"
#include "entity/xml_parser.hpp"
#include "entity/xml_printer.hpp"
#include "xml_printer_helper.hpp"

using namespace std;
using namespace mtconnect;
using namespace mtconnect::entity;
using namespace std::literals;

class EntityPrinterTest : public testing::Test
{
protected:
  void SetUp() override { m_writer = make_unique<XmlWriter>(true); }

  void TearDown() override { m_writer.reset(); }

  FactoryPtr createFileArchetypeFactory()
  {
    auto fileProperty =
        make_shared<Factory>(Requirements({Requirement("name", true), Requirement("VALUE", true)}));

    auto fileProperties = make_shared<Factory>(Requirements(
        {Requirement("FileProperty", ENTITY, fileProperty, 1, Requirement::Infinite)}));
    fileProperties->registerMatchers();

    auto fileComment = make_shared<Factory>(
        Requirements({Requirement("timestamp", true), Requirement("VALUE", true)}));

    auto fileComments = make_shared<Factory>(
        Requirements({Requirement("FileComment", ENTITY, fileComment, 1, Requirement::Infinite)}));
    fileComments->registerMatchers();

    auto fileArchetype = make_shared<Factory>(Requirements {
        Requirement("assetId", true), Requirement("deviceUuid", true),
        Requirement("timestamp", true), Requirement("removed", false), Requirement("name", true),
        Requirement("mediaType", true), Requirement("applicationCategory", true),
        Requirement("applicationType", true), Requirement("Description", false),
        Requirement("FileComments", ENTITY_LIST, fileComments, false),
        Requirement("FileProperties", ENTITY_LIST, fileProperties, false)});

    auto root =
        make_shared<Factory>(Requirements {Requirement("FileArchetype", ENTITY, fileArchetype)});

    return root;
  }

  std::unique_ptr<XmlWriter> m_writer;
};

TEST_F(EntityPrinterTest, TestParseSimpleDocument)
{
  auto root = createFileArchetypeFactory();

  auto doc = string {
      "<FileArchetype applicationCategory=\"ASSEMBLY\" applicationType=\"DATA\" assetId=\"uuid\" "
      "deviceUuid=\"duid\" mediaType=\"json\" name=\"xxxx\" timestamp=\"2020-12-01T10:00Z\">\n"
      "  <FileProperties>\n"
      "    <FileProperty name=\"one\">Round</FileProperty>\n"
      "    <FileProperty name=\"two\">Flat</FileProperty>\n"
      "  </FileProperties>\n"
      "</FileArchetype>\n"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::XmlPrinter printer;

  printer.print(*m_writer, entity, {});
  ASSERT_EQ(doc, m_writer->getContent());
}

TEST_F(EntityPrinterTest, TestFileArchetypeWithDescription)
{
  auto root = createFileArchetypeFactory();

  auto doc = string {
      "<FileArchetype applicationCategory=\"ASSEMBLY\" applicationType=\"DATA\" assetId=\"uuid\" "
      "deviceUuid=\"duid\" mediaType=\"json\" name=\"xxxx\" timestamp=\"2020-12-01T10:00Z\">\n"
      "  <Description>Hello there Shaurabh</Description>\n"
      "  <FileProperties>\n"
      "    <FileProperty name=\"one\">Round</FileProperty>\n"
      "    <FileProperty name=\"two\">Flat</FileProperty>\n"
      "  </FileProperties>\n"
      "</FileArchetype>\n"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::XmlPrinter printer;

  printer.print(*m_writer, entity, {});
  ASSERT_EQ(doc, m_writer->getContent());
}

TEST_F(EntityPrinterTest, TestRecursiveEntityLists)
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

  auto doc = string {
      "<Device id=\"d1\" name=\"foo\" uuid=\"xxx\">\n"
      "  <Components>\n"
      "    <Systems id=\"s1\">\n"
      "      <Components>\n"
      "        <Electric id=\"e1\"/>\n"
      "        <Heating id=\"h1\"/>\n"
      "      </Components>\n"
      "    </Systems>\n"
      "  </Components>\n"
      "</Device>\n"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::XmlPrinter printer;

  printer.print(*m_writer, entity, {});
  ASSERT_EQ(doc, m_writer->getContent());
}

TEST_F(EntityPrinterTest, TestEntityOrder)
{
  auto component = make_shared<Factory>(Requirements {
      Requirement("id", true),
      Requirement("VALUE", false),
  });

  auto components = make_shared<Factory>(
      Requirements({Requirement("ZFirstValue", ENTITY, component),
                    Requirement("HSecondValue", ENTITY, component),
                    Requirement("AThirdValue", ENTITY, component),
                    Requirement("GFourthValue", ENTITY, component), Requirement("Simple", false),
                    Requirement("Unordered", false)}));
  components->setOrder({"ZFirstValue", "Simple", "HSecondValue", "AThirdValue", "GFourthValue"});

  auto device = make_shared<Factory>(Requirements({
      Requirement("name", true),
      Requirement("Components", ENTITY, components),
  }));

  auto root = make_shared<Factory>(Requirements {Requirement("Device", ENTITY, device)});

  auto doc = string {
      "<Device name=\"foo\">\n"
      "  <Components>\n"
      "    <Unordered>Last</Unordered>\n"
      "    <HSecondValue id=\"a\">First</HSecondValue>\n"
      "    <GFourthValue id=\"b\">Second</GFourthValue>\n"
      "    <ZFirstValue id=\"c\">Third</ZFirstValue>\n"
      "    <Simple>Fourth</Simple>\n"
      "    <AThirdValue id=\"d\">Fifth</AThirdValue>\n"
      "  </Components>\n"
      "</Device>\n"};

  ErrorList errors;
  entity::XmlParser parser;

  auto entity = parser.parse(root, doc, "1.7", errors);
  ASSERT_EQ(0, errors.size());

  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});

  auto expected = string {
      "<Device name=\"foo\">\n"
      "  <Components>\n"
      "    <ZFirstValue id=\"c\">Third</ZFirstValue>\n"
      "    <Simple>Fourth</Simple>\n"
      "    <HSecondValue id=\"a\">First</HSecondValue>\n"
      "    <AThirdValue id=\"d\">Fifth</AThirdValue>\n"
      "    <GFourthValue id=\"b\">Second</GFourthValue>\n"
      "    <Unordered>Last</Unordered>\n"
      "  </Components>\n"
      "</Device>\n"};

  ASSERT_EQ(expected, m_writer->getContent());
}

TEST_F(EntityPrinterTest, TestRawContent)
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
  ASSERT_EQ(0, errors.size());

  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});

  auto expected = R"DOC(<Definition format="XML"><SomeContent with="stuff">
    And some text
  </SomeContent><AndMoreContent/>
  And random text as well.
</Definition>
)DOC";

  ASSERT_EQ(expected, m_writer->getContent());
}

class EntityPrinterNamespaceTest : public EntityPrinterTest
{
protected:
  EntityPtr createDevice()
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

    ErrorList errors;
    auto s1 = components->create("System", Properties {{"id", "s1"s}}, errors);
    EXPECT_EQ(0, errors.size());
    EXPECT_TRUE(s1);
    auto s2 = components->create("x:FlizGuard", Properties {{"id", "s2"s}}, errors);
    EXPECT_EQ(0, errors.size());
    EXPECT_TRUE(s2);

    EntityList list {s1, s2};
    auto c1 = device->create("Components", list, errors);
    EXPECT_TRUE(c1);
    EXPECT_EQ(0, errors.size());

    auto entity = root->create(
        "Device",
        Properties {{"id", "d1"s}, {"uuid", "xxx"s}, {"name", "foo"s}, {"Components", c1}}, errors);
    EXPECT_EQ(0, errors.size());
    EXPECT_TRUE(entity);

    return entity;
  }
};

TEST_F(EntityPrinterNamespaceTest, test_namespace_removal_when_no_namespaces)
{
  auto entity = createDevice();

  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {});

  auto expected = string {
      R"DOC(<Device id="d1" name="foo" uuid="xxx">
  <Components>
    <System id="s1"/>
    <FlizGuard id="s2"/>
  </Components>
</Device>
)DOC"};

  ASSERT_EQ(expected, m_writer->getContent());
}

TEST_F(EntityPrinterNamespaceTest, test_namespace_removal_with_namespaces)
{
  auto entity = createDevice();

  entity::XmlPrinter printer;
  printer.print(*m_writer, entity, {"x"});

  auto expected = string {
      R"DOC(<Device id="d1" name="foo" uuid="xxx">
  <Components>
    <System id="s1"/>
    <x:FlizGuard id="s2"/>
  </Components>
</Device>
)DOC"};

  ASSERT_EQ(expected, m_writer->getContent());
}
