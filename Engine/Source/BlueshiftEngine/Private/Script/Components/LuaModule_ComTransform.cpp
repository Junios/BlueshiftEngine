// Copyright(c) 2017 POLYGONTEK
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
// http ://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Precompiled.h"
#include "Script/LuaVM.h"
#include "Components/ComTransform.h"

BE_NAMESPACE_BEGIN

void LuaVM::RegisterTransformComponent(LuaCpp::Module &module) {
    LuaCpp::Selector _ComTransform = module["ComTransform"];

    // Component class should be registered first
    _ComTransform.SetClass<ComTransform>(module["Component"]);
    _ComTransform.AddClassMembers<ComTransform>(
        "parent", &ComTransform::GetParent,
        "local_origin", &ComTransform::GetLocalOrigin,
        "local_scale", &ComTransform::GetLocalScale,
        "local_axis", &ComTransform::GetLocalAxis,
        "local_angles", &ComTransform::GetLocalAngles,
        "set_local_origin", &ComTransform::SetLocalOrigin,
        "set_local_scale", &ComTransform::SetLocalScale,
        "set_local_axis", &ComTransform::SetLocalAxis,
        "set_local_angles", &ComTransform::SetLocalAngles,
        "set_local_transform", &ComTransform::SetLocalTransform,
        "origin", &ComTransform::GetOrigin,
        "scale", &ComTransform::GetScale,
        "axis", &ComTransform::GetAxis,
        "angles", &ComTransform::GetAngles,
        "set_origin", &ComTransform::SetOrigin,
        "set_axis", &ComTransform::SetAxis,
        "set_angles", &ComTransform::SetAngles,
        "local_matrix", &ComTransform::GetLocalMatrix,
        "world_matrix", &ComTransform::GetWorldMatrix,
        "translate", &ComTransform::Translate,
        "rotate", &ComTransform::Rotate);

    _ComTransform["meta_object"] = ComTransform::metaObject;
}

BE_NAMESPACE_END