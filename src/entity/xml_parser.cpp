//
// Copyright Copyright 2009-2019, AMT – The Association For Manufacturing Technology (“AMT”)
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

#include "entity/xml_parser.hpp"
#include "xml_helper.hpp"

#include <dlib/logger.h>

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

using namespace std;

namespace mtconnect
{
  namespace entity
  {
    static dlib::logger g_logger("entity.xml_parser");
    
    extern "C" void XMLCDECL entityXMLErrorFunc(void *ctx ATTRIBUTE_UNUSED, const char *msg, ...)
    {
      va_list args;
      
      char buffer[2048] = {0};
      va_start(args, msg);
      vsnprintf(buffer, 2046u, msg, args);
      buffer[2047] = '0';
      va_end(args);
      
      g_logger << dlib::LERROR << "XML: " << buffer;
    }
    
    string nodeQName(xmlNodePtr node)
    {
      string qname((const char*) node->name);
      
      if (node->ns && node->ns->prefix &&
          strncmp((const char *) node->ns->href, "urn:mtconnect.org:MTConnectDevices", 34u))
      {
        qname.insert(0, (const char* ) node->ns->prefix);
      }
      
      return qname;
    }
    
    inline void trim(string &s)
    {
      auto beg = s.find_first_not_of(" \t\n");
      if (beg > 0)
        s.erase(0, beg);
      if (!s.empty())
      {
        auto end = s.find_last_not_of(" \t\n");
        if (end < s.size() - 1)
          s.erase(end);
      }
    }
        
    static EntityPtr parseXmlNode(FactoryPtr factory, xmlNodePtr node,
                            ErrorList &errors)
    {
      auto qname = nodeQName(node);
      auto ef = factory->factoryFor(qname);
      if (ef)
      {
        Properties properties;
        EntityList *l { nullptr };
        if (ef->isList())
        {
          l = &properties["list"].emplace<EntityList>();
        }
        
        for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
        {
          if (attr->type == XML_ATTRIBUTE_NODE)
          {
            properties.insert({ (const char *) attr->name,
              string((const char *) attr->children->content) });
          }
        }
              
        for (xmlNodePtr child = node->children; child; child = child->next)
        {
          if (child->type == XML_ELEMENT_NODE)
          {
            auto ent = parseXmlNode(ef, child, errors);
            if (ent)
            {
              if (ef->isList())
              {
                l->emplace_back(ent);
              }
              else
              {
                properties.insert({ ent->getName(), ent });
              }
            }
            else if (child->children->type == XML_TEXT_NODE && child->children->content &&
                     *child->children->content != '\0' && child->properties == nullptr)
            {
              // TODO: Fix when the child is an element that failed because of an
              // error instead of not being associated with simple content entity.
              string s((const char *) child->children->content);
              trim(s);
              if (!s.empty())
                properties.insert({  nodeQName(child), s });
            }
            else
            {
              g_logger << dlib::LWARN << "Unexpected element: " << nodeQName(child);
            }
          }
          else if (child->type == XML_TEXT_NODE)
          {
            string s((const char *) child->content);
            trim(s);
            if (!s.empty())
              properties.insert({ "value", s });
          }
        }
        
        auto entity = ef->make(qname, properties, errors);
        return entity;
      }
      
      return nullptr;
    }
    
    
    EntityPtr XmlParser::parse(FactoryPtr factory,
                                const string &document,
                                const string &version,
                                ErrorList &errors)
    {
      EntityPtr entity;
      try
      {
        xmlInitParser();
        xmlXPathInit();
        xmlSetGenericErrorFunc(nullptr, entityXMLErrorFunc);
        
        unique_ptr<xmlDoc,function<void(xmlDocPtr)>> doc(xmlReadMemory(document.c_str(),
                                                               document.length(),
                                                               "document.xml", nullptr,
                                                               XML_PARSE_NOBLANKS),
                                                        [](xmlDocPtr d){ xmlFreeDoc(d); });
        xmlNodePtr root = xmlDocGetRootElement(doc.get());
        entity = parseXmlNode(factory, root, errors);
      }
      
      catch (PropertyError e)
      {
        g_logger << dlib::LERROR << "Cannot parse XML document: " << e.what();
        errors.emplace_back(e);
        throw;
      }
      
      catch (XmlError e)
      {
        g_logger << dlib::LERROR << "Cannot parse XML document: " << e.what();
        throw;
      }
      
      return entity;
    }    
  }
}