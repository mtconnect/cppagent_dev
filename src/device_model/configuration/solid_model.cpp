//
// Copyright Copyright 2009-2021, AMT � The Association For Manufacturing Technology (�AMT�)
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

#include "solid_model.hpp"

using namespace std;

namespace mtconnect::device_model::configuration {
  using namespace entity;
  FactoryPtr SolidModel::getFactory()
  {
    static FactoryPtr solidModel;
    if (!solidModel)
    {
      auto transformation = make_shared<Factory>(Requirements {
          Requirement("Translation", VECTOR, 3, false), Requirement("Rotation", VECTOR, 3, false)});

      solidModel = make_shared<Factory>(
          Requirements {Requirement("id", true), Requirement("solidModelIdRef", false),
                        Requirement("href", false), Requirement("itemRef", false),
                        Requirement("mediaType",
                                    ControlledVocab {"STEP", "STL", "GDML", "OBJ", "COLLADA",
                                                     "IGES", "3DS", "ACIS", "X_T"},
                                    true),
                        Requirement("coordinateSystemIdRef", false),
                        Requirement("Transformation", ENTITY, transformation, false),
                        Requirement("Scale", VECTOR, 3, false)});

      solidModel->registerMatchers();
    }
    return solidModel;
  }
}  // namespace mtconnect::device_model::configuration
