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

#pragma once

#include "entity/entity.hpp"

namespace mtconnect
{
  namespace source
  {
    // A transform takes an entity and transforms it to another
    // entity. The transform is an object with the overloaded
    // operation () takes the object type and performs produces an
    // output. The entities are pass as shared pointers.
    //
    // Additiional parameters can be bound if additional context is
    // required.

    class Transform;
    using TransformPtr = std::shared_ptr<Transform>;
    using TransformMap = std::unordered_map<std::type_index, TransformPtr>;

    class Transform : public std::enable_shared_from_this<Transform>
    {
    public:
      virtual ~Transform() = default;
      Transform(const Transform &) = default;
      Transform() = default;

      virtual const entity::EntityPtr operator()(const entity::EntityPtr entity) = 0;
      TransformPtr getptr() { return shared_from_this(); }

      const entity::EntityPtr next(const entity::EntityPtr entity)
      {
        auto &r = *entity.get();
        auto next = m_next.find(std::type_index(typeid(r)));
        if (next != m_next.end())
          return (*next->second)(entity);
        else
          return entity;
      }

      template <typename T>
      void bind(TransformPtr trans)
      {
        m_next[std::type_index(typeid(T))] = trans;
      }

      std::string m_name;
      TransformMap m_next;
    };

  }  // namespace source
}  // namespace mtconnect
