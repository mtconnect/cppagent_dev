// Ensure that gtest is the first header otherwise Windows raises an error
#include <gtest/gtest.h>
// Keep this comment to keep gtest.h above. (clang-format off/on is not working here!)

#include "adapter.hpp"
#include "agent.hpp"
#include "agent_test_helper.hpp"
#include "json_helper.hpp"
#include "entity.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

using json = nlohmann::json;
using namespace std;
using namespace mtconnect;
using namespace mtconnect::entity;

class EntityTest : public testing::Test
{
 protected:
  void SetUp() override
  {  // Create an agent with only 16 slots and 8 data items.
    m_agent = make_unique<Agent>(PROJECT_ROOT_DIR "/samples/solid_model.xml", 4, 4, "1.7");
    m_agentId = int64ToString(getCurrentTimeInSec());

    m_agentTestHelper = make_unique<AgentTestHelper>();
    m_agentTestHelper->m_agent = m_agent.get();

    m_device = m_agent->getDeviceByName("LinuxCNC");
    
    
  }

  void TearDown() override
  {
    m_agent.reset();
    m_agentTestHelper.reset();
  }

  std::unique_ptr<Agent> m_agent;
  std::string m_agentId;
  Device *m_device{nullptr};

  std::unique_ptr<AgentTestHelper> m_agentTestHelper;
};


TEST_F(EntityTest, TestSimpleFactory)
{
  FactoryPtr root = make_shared<Factory>();
  FactoryPtr simpleFact = make_shared<Factory>(Requirements({
    Requirement("name", true ),
    Requirement("id", true),
    Requirement("size", false, Requirement::INTEGER) }));
  root->registerFactory("simple", simpleFact);

  Properties simple { { "id", "abc" }, { "name", "xxx" }, {"size", 10 }};
  
  auto entity = root->create("simple", simple);
  ASSERT_TRUE(entity);
  ASSERT_EQ(3, entity->getProperties().size());
  ASSERT_EQ("simple", entity->getName());
  ASSERT_EQ("abc", get<std::string>(entity->getProperty("id")));
  ASSERT_EQ("xxx", get<std::string>(entity->getProperty("name")));
  ASSERT_EQ(10, get<int>(entity->getProperty("size")));
}

TEST_F(EntityTest, TestSimpleTwoLevelFactory)
{
  auto root = make_shared<Factory>();
  
  auto second = make_shared<Factory>(Requirements({
    Requirement("key", true ),
    Requirement("value", true) }));
  
  auto simple = make_shared<Factory>(Requirements{
    Requirement("name", true ),
    Requirement("id", true),
    Requirement("size", false, Requirement::INTEGER),
    Requirement("second", Requirement::ENTITY, second)
  });
  root->registerFactory("simple",  simple);
  
  auto fact = root->factoryFor("simple");
  ASSERT_TRUE(fact);
  
  auto sfact = fact->factoryFor("second");
  ASSERT_TRUE(sfact);
  
  Properties sndp { {"key", "1" }, {"value", "arf"}};
  auto se = fact->create("second", sndp);
  ASSERT_TRUE(se);
  ASSERT_EQ(2, se->getProperties().size());
  ASSERT_EQ("1", get<std::string>(se->getProperty("key")));
  ASSERT_EQ("arf", get<std::string>(se->getProperty("value")));

  Properties simpp {
    { "id", "abc" }, { "name", "xxx" }, {"size", 10 },
    { "second", se }
  };
  
  auto entity = root->create("simple", simpp);
  ASSERT_TRUE(entity);
  ASSERT_EQ(4, entity->getProperties().size());
  ASSERT_EQ("simple", entity->getName());
  ASSERT_EQ("abc", get<std::string>(entity->getProperty("id")));
  ASSERT_EQ("xxx", get<std::string>(entity->getProperty("name")));
  ASSERT_EQ(10, get<int>(entity->getProperty("size")));
  
  auto v = get<EntityPtr>(entity->getProperty("second"));
  ASSERT_TRUE(v);
  ASSERT_EQ(2, v->getProperties().size());
  ASSERT_EQ("1", get<std::string>(v->getProperty("key")));
  ASSERT_EQ("arf", get<std::string>(v->getProperty("value")));

}

TEST_F(EntityTest, TestSimpleEntityList)
{
  auto root = make_shared<Factory>();
  
  auto second = make_shared<Factory>(Requirements({
    Requirement("key", true ),
    Requirement("value", true) }));
  
  auto seconds = make_shared<Factory>(Requirements({
    Requirement("second", Requirement::ENTITY, second,
                1, Requirement::Infinite) }));
  seconds->registerMatchers();

  auto simple = make_shared<Factory>(Requirements{
    Requirement("name", true ),
    Requirement("id", true),
    Requirement("size", false, Requirement::INTEGER),
    Requirement("seconds", Requirement::ENTITY_LIST, seconds)
  });
  root->registerFactory("simple",  simple);
  
  auto fact = root->factoryFor("simple");
  ASSERT_TRUE(fact);
  
  auto secondsFact = fact->factoryFor("seconds");
  ASSERT_TRUE(secondsFact);
  ASSERT_TRUE(secondsFact->isList());

  auto secondFact = secondsFact->factoryFor("second");
  ASSERT_TRUE(secondFact);
  
  Properties sndp1 { {"key", "1" }, {"value", "arf"}};
  auto se1 = secondsFact->create("second", sndp1);
  ASSERT_TRUE(se1);
  ASSERT_EQ(2, se1->getProperties().size());
  ASSERT_EQ("1", get<std::string>(se1->getProperty("key")));
  ASSERT_EQ("arf", get<std::string>(se1->getProperty("value")));
  
  Properties sndp2 { {"key", "2" }, {"value", "meow"}};
  auto se2 = secondsFact->create("second", sndp2);
  ASSERT_TRUE(se2);
  ASSERT_EQ(2, se2->getProperties().size());
  ASSERT_EQ("2", get<std::string>(se2->getProperty("key")));
  ASSERT_EQ("meow", get<std::string>(se2->getProperty("value")));

  ErrorList errors;
  EntityList list { se1, se2 };
  auto se3 = fact->create("seconds", list, errors);
  ASSERT_EQ(0, errors.size());
  ASSERT_TRUE(se3);
  
  Properties simpp {
    { "id", "abc" }, { "name", "xxx" }, {"size", 10 },
    { "seconds", se3 }
  };
  
  auto entity = root->create("simple", simpp);
  ASSERT_TRUE(entity);
  ASSERT_EQ(4, entity->getProperties().size());
  ASSERT_EQ("simple", entity->getName());
  ASSERT_EQ("abc", get<std::string>(entity->getProperty("id")));
  ASSERT_EQ("xxx", get<std::string>(entity->getProperty("name")));
  ASSERT_EQ(10, get<int>(entity->getProperty("size")));
  
  auto l = entity->getList("seconds");
  
  ASSERT_TRUE(l);
  ASSERT_EQ(2, l->size());

  auto it = l->begin();
  ASSERT_NE(l->end(), it);
  ASSERT_EQ(2, (*it)->getProperties().size());
  ASSERT_EQ("1", get<std::string>((*it)->getProperty("key")));
  ASSERT_EQ("arf", get<std::string>((*it)->getProperty("value")));
  
  it++;
  ASSERT_NE(l->end(), it);
  ASSERT_EQ(2, (*it)->getProperties().size());
  ASSERT_EQ("2", get<std::string>((*it)->getProperty("key")));
  ASSERT_EQ("meow", get<std::string>((*it)->getProperty("value")));
}

TEST_F(EntityTest, MissingProperty)
{
  FactoryPtr root = make_shared<Factory>();
  FactoryPtr simpleFact = make_shared<Factory>(Requirements({
    Requirement("name", true ),
    Requirement("id", true),
    Requirement("size", false, Requirement::INTEGER) }));
  root->registerFactory("simple", simpleFact);
  
  Properties simple { { "name", "xxx" }, {"size", 10 }};
  
  ErrorList errors;
  auto entity = root->create("simple", simple, errors);
  ASSERT_FALSE(entity);
  
  ASSERT_EQ(1, errors.size());
  ASSERT_EQ(string("Property id is required and not provided"),
            errors.begin()->what());
}

TEST_F(EntityTest, MissingOptionalProperty)
{
  FactoryPtr root = make_shared<Factory>();
  FactoryPtr simpleFact = make_shared<Factory>(Requirements({
    Requirement("name", true ),
    Requirement("id", true),
    Requirement("size", false, Requirement::INTEGER) }));
  root->registerFactory("simple", simpleFact);
  
  Properties simple { { "id", "abc" }, { "name", "xxx" }};
  
  ErrorList errors;
  auto entity = root->create("simple", simple, errors);
  ASSERT_TRUE(entity);
  ASSERT_EQ(2, entity->getProperties().size());
  ASSERT_EQ("simple", entity->getName());
  ASSERT_EQ("abc", get<std::string>(entity->getProperty("id")));
  ASSERT_EQ("xxx", get<std::string>(entity->getProperty("name")));
  
  ASSERT_EQ(0, errors.size());
}

TEST_F(EntityTest, UnexpectedProperty)
{
  FactoryPtr root = make_shared<Factory>();
  FactoryPtr simpleFact = make_shared<Factory>(Requirements({
    Requirement("name", true ),
    Requirement("id", true),
    Requirement("size", false, Requirement::INTEGER) }));
  root->registerFactory("simple", simpleFact);
  
  Properties simple { { "id", "abc" }, { "name", "xxx" }, { "junk", "junk" }};
  
  ErrorList errors;
  auto entity = root->create("simple", simple, errors);
  ASSERT_FALSE(entity);
  
  ASSERT_EQ(1, errors.size());
  ASSERT_EQ(string("The following keys were present and not expected: junk,"),
            errors.begin()->what());
}

TEST_F(EntityTest, EntityListAnyEntities)
{
  auto root = make_shared<Factory>();
  
  auto second = make_shared<Factory>(Requirements({
    Requirement("key", true ),
    Requirement("value", true) }));
  
  auto seconds = make_shared<Factory>(Requirements({
    Requirement("something", Requirement::ENTITY, second,
                1, Requirement::Infinite) }));
  seconds->registerFactory(regex(".+"), second);
  seconds->registerMatchers();

  auto simple = make_shared<Factory>(Requirements{
    Requirement("name", true ),
    Requirement("id", true),
    Requirement("size", false, Requirement::INTEGER),
    Requirement("seconds", Requirement::ENTITY_LIST, seconds)
  });
  root->registerFactory("simple",  simple);
  
  auto fact = root->factoryFor("simple");
  ASSERT_TRUE(fact);
  
  auto secondsFact = fact->factoryFor("seconds");
  ASSERT_TRUE(secondsFact);
  
  auto secondFact = secondsFact->factoryFor("dog");
  ASSERT_TRUE(secondFact);
  
  ErrorList errors;

  Properties sndp1 { {"key", "1" }, {"value", "arf"}};
  auto se1 = secondsFact->create("dog", sndp1, errors);
  ASSERT_EQ(0, errors.size());
  ASSERT_TRUE(se1);
  ASSERT_EQ(2, se1->getProperties().size());
  ASSERT_EQ("1", get<std::string>(se1->getProperty("key")));
  ASSERT_EQ("arf", get<std::string>(se1->getProperty("value")));
  
  Properties sndp2 { {"key", "2" }, {"value", "meow"}};
  auto se2 = secondsFact->create("cat", sndp2, errors);
  ASSERT_EQ(0, errors.size());
  ASSERT_TRUE(se2);
  ASSERT_EQ(2, se2->getProperties().size());
  ASSERT_EQ("2", get<std::string>(se2->getProperty("key")));
  ASSERT_EQ("meow", get<std::string>(se2->getProperty("value")));
  
  EntityList list { se1, se2 };
  auto se3 = fact->create("seconds", list, errors);
  ASSERT_EQ(0, errors.size());
  ASSERT_TRUE(se3);
  
  Properties simpp {
    { "id", "abc" }, { "name", "xxx" }, {"size", 10 },
    { "seconds", se3 }
  };
  
  auto entity = root->create("simple", simpp, errors);
  ASSERT_EQ(0, errors.size());
  ASSERT_TRUE(entity);
  ASSERT_EQ(4, entity->getProperties().size());
  ASSERT_EQ("simple", entity->getName());
  ASSERT_EQ("abc", get<std::string>(entity->getProperty("id")));
  ASSERT_EQ("xxx", get<std::string>(entity->getProperty("name")));
  ASSERT_EQ(10, get<int>(entity->getProperty("size")));
  
  auto l = entity->getList("seconds");
  ASSERT_EQ(2, l->size());
  
  auto it = l->begin();
  ASSERT_NE(l->end(), it);
  ASSERT_EQ(2, (*it)->getProperties().size());
  ASSERT_EQ("dog", (*it)->getName());
  ASSERT_EQ("1", get<std::string>((*it)->getProperty("key")));
  ASSERT_EQ("arf", get<std::string>((*it)->getProperty("value")));
  
  it++;
  ASSERT_NE(l->end(), it);
  ASSERT_EQ(2, (*it)->getProperties().size());
  ASSERT_EQ("cat", (*it)->getName());
  ASSERT_EQ("2", get<std::string>((*it)->getProperty("key")));
  ASSERT_EQ("meow", get<std::string>((*it)->getProperty("value")));

}
