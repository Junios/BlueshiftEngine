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
#include "Render/Render.h"
#include "RenderInternal.h"
#include "Core/JointPose.h"

BE_NAMESPACE_BEGIN

void RBSurf::DrawPrimitives() const {
    glr.BindBuffer(Renderer::IndexBuffer, ibHandle);
    
    if (numInstances > 1) {
        glr.DrawElementsInstanced(Renderer::TrianglesPrim, startIndex, numIndexes, sizeof(TriIndex), 0, numInstances);
    } else {
        glr.DrawElements(Renderer::TrianglesPrim, startIndex, numIndexes, sizeof(TriIndex), 0);
    }

    if (flushType == ShadowFlush) {
        backEnd.ctx->renderCounter.shadowDrawCalls++;
        backEnd.ctx->renderCounter.shadowDrawIndexes += numIndexes;
        backEnd.ctx->renderCounter.shadowDrawVerts += numVerts;
    } else {
        backEnd.ctx->renderCounter.drawCalls++;
        backEnd.ctx->renderCounter.drawIndexes += numIndexes;
        backEnd.ctx->renderCounter.drawVerts += numVerts;
    }
}

void RBSurf::SetShaderProperties(const Shader *shader, const StrHashMap<Shader::Property> &shaderProperties) const {
    const auto &specHashMap = shader->GetSpecHashMap();

    for (int i = 0; i < specHashMap.Count(); i++) {
        const auto *entry = specHashMap.GetByIndex(i);
        const auto &key = entry->first;
        const auto &spec = entry->second;

        if (spec.GetFlags() & PropertySpec::ShaderDefine) {
            continue;
        }

        const auto *propEntry = shaderProperties.Get(key);
        if (!propEntry) {
            continue;
        }

        const Shader::Property &prop = propEntry->second;

        switch (spec.GetType()) {
        case PropertySpec::FloatType:
            shader->SetConstant1f(key, prop.data.As<float>());
            break;
        case PropertySpec::Vec2Type:
            shader->SetConstant2f(key, prop.data.As<Vec2>());
            break;
        case PropertySpec::Vec3Type:
            shader->SetConstant3f(key, prop.data.As<Vec3>());
            break;
        case PropertySpec::Vec4Type:
            shader->SetConstant4f(key, prop.data.As<Vec4>());
            break;
        case PropertySpec::PointType:
            shader->SetConstant2i(key, prop.data.As<Point>());
            break;
        case PropertySpec::RectType:
            shader->SetConstant4i(key, prop.data.As<Rect>());
            break;
        case PropertySpec::Mat3Type:
            shader->SetConstant3x3f(key, true, prop.data.As<Mat3>());
            break;
        case PropertySpec::ObjectType: // 
            shader->SetTexture(key, prop.texture);
            break;
        default:
            assert(0);
            break;
        }
    }

    //shader->SetConstant1f("currentRenderWidthRatio", (float)GL_GetVidWidth() / g_rsd.screenWidth);
    //shader->SetConstant1f("currentRenderHeightRatio", (float)GL_GetVidHeight() /g_rsd.screenHeight);
}

const Texture *RBSurf::TextureFromShaderProperties(const Material::Pass *mtrlPass, const Str &textureName) const {
    const auto *entry = mtrlPass->shader->GetSpecHashMap().Get(textureName);
    if (!entry) {
        return nullptr;
    }

    const auto &spec = entry->second;
    if ((spec.GetFlags() & PropertySpec::ShaderDefine) || (spec.GetType() != PropertySpec::ObjectType)) {
        return nullptr;
    }

    const Shader::Property &prop = mtrlPass->shaderProperties.Get(textureName)->second;
    return prop.texture;
}

void RBSurf::SetMatrixConstants(const Shader *shader) const {
    if (shader->builtInConstantLocations[Shader::ModelViewMatrixConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewMatrixConst], true, backEnd.modelViewMatrix);
    }
    
    if (shader->builtInConstantLocations[Shader::ProjectionMatrixConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ProjectionMatrixConst], true, backEnd.projMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixConst], true, backEnd.modelViewProjMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ModelViewMatrixTransposeConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewMatrixTransposeConst], false, backEnd.modelViewMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ProjectionMatrixTransposeConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ProjectionMatrixTransposeConst], false, backEnd.projMatrix);
    }

    if (shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixTransposeConst] >= 0) {
        shader->SetConstant4x4f(shader->builtInConstantLocations[Shader::ModelViewProjectionMatrixTransposeConst], false, backEnd.modelViewProjMatrix);
    }
}

void RBSurf::SetVertexColorConstants(const Shader *shader, const Material::VertexColorMode &vertexColor) const {
    Vec4 vertexColorScale;
    Vec4 vertexColorAdd;

    if (vertexColor == Material::ModulateVertexColor) {
        vertexColorScale.Set(1.0f, 1.0f, 1.0f, 1.0f);
        vertexColorAdd.Set(0.0f, 0.0f, 0.0f, 0.0f);
    } else if (vertexColor == Material::InverseModulateVertexColor) {
        vertexColorScale.Set(-1.0f, -1.0f, -1.0f, 1.0f);
        vertexColorAdd.Set(1.0f, 1.0f, 1.0f, 0.0f);
    } else {
        vertexColorScale.Set(0.0f, 0.0f, 0.0f, 0.0f);
        vertexColorAdd.Set(1.0f, 1.0f, 1.0f, 1.0f);
    }
    
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::VertexColorScaleConst], vertexColorScale);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::VertexColorAddConst], vertexColorAdd);
}

void RBSurf::SetSkinningConstants(const Shader *shader, const SkinningJointCache *cache) const {
    if (renderGlobal.skinningMethod == Mesh::CpuSkinning) {
        return;
    }

    if (renderGlobal.skinningMethod == Mesh::VertexShaderSkinning) {
        shader->SetConstantArray4f("joints", cache->numJoints * 3, cache->skinningJoints[0].Ptr());
    } else if (renderGlobal.skinningMethod == Mesh::VtfSkinning) {
        const Texture *jointsMapTexture = cache->bufferCache.texture;
        shader->SetTexture("jointsMap", jointsMapTexture);

        if (renderGlobal.vtUpdateMethod == Mesh::TboUpdate) {
            shader->SetConstant1i("tcBase", cache->bufferCache.tcBase[0]);
        } else {
            shader->SetConstant2f("invJointsMapSize", Vec2(1.0f / jointsMapTexture->GetWidth(), 1.0f / jointsMapTexture->GetHeight()));
            shader->SetConstant2f("tcBase", Vec2(cache->bufferCache.tcBase[0], cache->bufferCache.tcBase[1]));
        }

        if (r_usePostProcessing.GetBool() && (r_motionBlur.GetInteger() & 2)) {
            shader->SetConstant1i("jointIndexOffsetCurr", cache->jointIndexOffsetCurr);
            shader->SetConstant1i("jointIndexOffsetPrev", cache->jointIndexOffsetPrev);
        }
    }
}

void RBSurf::RenderColor(const Color4 &color) const {
    const Shader *shader = ShaderManager::constantColorShader;

    if (subMesh && subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    if (subMesh && subMesh->useGpuSkinning) {
        const Mesh *mesh = surfEntity->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    shader->SetConstant4f("color", color);

    DrawPrimitives();
}

void RBSurf::RenderSelection(const Material::Pass *mtrlPass, const Vec3 &vec3_id) const {
    const Shader *shader = ShaderManager::selectionIdShader;
    if (mtrlPass->stateBits & Renderer::MaskAF && shader->perforatedVersion) {
        shader = shader->perforatedVersion;
    }

    if (subMesh && subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    if (subMesh && subMesh->useGpuSkinning) {
        const Mesh *mesh = surfEntity->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

        Color4 color;
        if (mtrlPass->useOwnerColor) {
            color = Color4(&surfEntity->def->parms.materialParms[SceneEntity::RedParm]);
        } else {
            color = mtrlPass->constantColor;
        }

        shader->SetConstant4f("constantColor", color);

        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "diffuseMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], baseTexture);
    }

    shader->SetConstant3f("id", vec3_id);

    DrawPrimitives();
}

void RBSurf::RenderDepth(const Material::Pass *mtrlPass) const {
    const Shader *shader = ShaderManager::depthShader;
    if (mtrlPass->stateBits & Renderer::MaskAF && shader->perforatedVersion) {
        shader = shader->perforatedVersion;
    }

    if (subMesh && subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    if (subMesh && subMesh->useGpuSkinning) {
        const Mesh *mesh = surfEntity->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

        Color4 color;
        if (mtrlPass->useOwnerColor) {
            color = Color4(&surfEntity->def->parms.materialParms[SceneEntity::RedParm]);
        } else {
            color = mtrlPass->constantColor;
        }

        shader->SetConstant4f("constantColor", color);

        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "diffuseMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], baseTexture);
    }	

    DrawPrimitives();
}

void RBSurf::RenderVelocity(const Material::Pass *mtrlPass) const {
    const Shader *shader = ShaderManager::objectMotionBlurShader;
    if (mtrlPass->stateBits & Renderer::MaskAF && shader->perforatedVersion) {
        shader = shader->perforatedVersion;
    }

    if (subMesh && subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    shader->Bind();

    SetMatrixConstants(shader);

    Mat4 prevModelViewMatrix = backEnd.view->def->viewMatrix * surfEntity->def->motionBlurModelMatrix[1];
    //shader->SetConstantMatrix4fv("prevModelViewMatrix", 1, true, prevModelViewMatrix);

    Mat4 prevModelViewProjMatrix = backEnd.view->def->projMatrix * prevModelViewMatrix;
    shader->SetConstant4x4f("prevModelViewProjectionMatrix", true, prevModelViewProjMatrix);

    shader->SetConstant1f("shutterSpeed", r_motionBlur_ShutterSpeed.GetFloat() / backEnd.ctx->frameTime);
    //shader->SetConstant1f("motionBlurID", (float)surfEntity->id);

    shader->SetTexture("depthMap", backEnd.ctx->screenDepthTexture);

    if (mtrlPass->stateBits & Renderer::MaskAF) {
        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "diffuseMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], baseTexture);
        
        Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
        Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);
    }

    if (subMesh && subMesh->useGpuSkinning) {
        const Mesh *mesh = surfEntity->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    DrawPrimitives();
}

void RBSurf::RenderAmbient(const Material::Pass *mtrlPass, float ambientScale) const {
    const Shader *shader = nullptr;

    if (r_ambientLit.GetBool()) {
        shader = !mtrlPass->shader || !mtrlPass->shader->ambientLitVersion ? ShaderManager::amblitNoBumpShader : mtrlPass->shader->ambientLitVersion;
        if (!shader) {
            shader = ShaderManager::amblitNoBumpShader;
        }
        
        if (!r_useDepthPrePass.GetBool()) {
            if (mtrlPass->stateBits & Renderer::MaskAF && shader->perforatedVersion) {
                shader = shader->perforatedVersion;
            }
        }

        if (subMesh && subMesh->useGpuSkinning) {
            if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
                shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
            }
        }

        if (!mtrlPass->shader) {
            shader->Bind();
            shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], mtrlPass->texture);
        } else {
            if (mtrlPass->shader->ambientLitVersion) {
                shader->Bind();
                SetShaderProperties(shader, mtrlPass->shaderProperties);
            } else {
                shader->Bind();
                const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "diffuseMap") : mtrlPass->texture;
                shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], baseTexture);
            }
        }
        
        SetMatrixConstants(shader);

        if (subMesh && subMesh->useGpuSkinning) {
            const Mesh *mesh = surfEntity->def->parms.mesh;
            SetSkinningConstants(shader, mesh->skinningJointCache);
        }

        // TODO:
        shader->SetTexture("ambientCubeMap0", backEnd.irradianceCubeMapTexture);
        shader->SetTexture("ambientCubeMap1", backEnd.irradianceCubeMapTexture);
        shader->SetConstant1f("ambientLerp", 0.0f);

        // view vector: world -> to mesh coordinates
        Vec3 localViewOrigin = surfEntity->def->parms.axis.TransposedMulVec(backEnd.view->def->parms.origin - surfEntity->def->parms.origin) / surfEntity->def->parms.scale;
        //Vec3 localViewOrigin = (backEnd.view->def->parms.origin - surfEntity->def->parms.origin) * surfEntity->def->parms.axis;
        shader->SetConstant3f(shader->builtInConstantLocations[Shader::LocalViewOriginConst], localViewOrigin);

        const Mat4 &worldMatrix = surfEntity->def->GetModelMatrix();
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixSConst], worldMatrix[0]);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixTConst], worldMatrix[1]);
        shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixRConst], worldMatrix[2]);			
    } else {
        shader = ShaderManager::amblitNoAmbientCubeMapShader;
        
        if (!r_useDepthPrePass.GetBool()) {
            if (mtrlPass->stateBits & Renderer::MaskAF && shader->perforatedVersion) {
                shader = shader->perforatedVersion;
            }
        }
        
        if (subMesh && subMesh->useGpuSkinning) {
            if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
                shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
            }
        }

        shader->Bind();

        SetMatrixConstants(shader);

        if (subMesh && subMesh->useGpuSkinning) {
            const Mesh *mesh = surfEntity->def->parms.mesh;
            SetSkinningConstants(shader, mesh->skinningJointCache);
        }

        const Texture *baseTexture = mtrlPass->shader ? TextureFromShaderProperties(mtrlPass, "diffuseMap") : mtrlPass->texture;
        shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], baseTexture);
        
        ambientScale *= 0.5f;
    }

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfEntity->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }
        
    if (ambientScale != 1.0f) {
        color[0] *= ambientScale;
        color[1] *= ambientScale;
        color[2] *= ambientScale;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);
    
    DrawPrimitives();
}

void RBSurf::RenderGeneric(const Material::Pass *mtrlPass) const {
    const Shader *shader;

    if (mtrlPass->shader) {
        shader = mtrlPass->shader;

        if (subMesh && subMesh->useGpuSkinning) {
            if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
                shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
            }
        }

        shader->Bind();
        SetShaderProperties(shader, mtrlPass->shaderProperties);
    } else {
        shader = ShaderManager::amblitNoAmbientCubeMapShader;

        if (subMesh && subMesh->useGpuSkinning) {
            if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
                shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
            }
        }

        shader->Bind();
        shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], mtrlPass->texture);
    }
    
    SetMatrixConstants(shader);

    if (subMesh && subMesh->useGpuSkinning) {
        const Mesh *mesh = surfEntity->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }

    Vec3 localViewOrigin = surfEntity->def->parms.axis.TransposedMulVec(backEnd.view->def->parms.origin - surfEntity->def->parms.origin) / surfEntity->def->parms.scale;
    //Vec3 localViewOrigin = (backEnd.view->def->parms.origin - surfEntity->def->parms.origin) * surfEntity->def->parms.axis;
    shader->SetConstant3f(shader->builtInConstantLocations[Shader::LocalViewOriginConst], localViewOrigin);

    const Mat4 &worldMatrix = surfEntity->def->GetModelMatrix();
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixSConst], worldMatrix[0]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixTConst], worldMatrix[1]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixRConst], worldMatrix[2]);

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfEntity->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);

    DrawPrimitives();
}

void RBSurf::RenderLightInteraction(const Material::Pass *mtrlPass) const {
    bool useShadowMap = (r_shadows.GetInteger() == 0) || (!surfLight->def->parms.castShadows || !surfEntity->def->parms.receiveShadows) ? false : true;
    const Shader *shader;

    if (mtrlPass->shader) {
        shader = mtrlPass->shader;
        if (useShadowMap) {
            if (surfLight->def->parms.type == SceneLight::PointLight) {
                shader = shader->pointShadowVersion;
            } else if (surfLight->def->parms.type == SceneLight::SpotLight) {
                shader = shader->spotShadowVersion;
            } else if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
                shader = shader->parallelShadowVersion;
            }
        }
            
        if (subMesh && subMesh->useGpuSkinning) {
            if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
                shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
            }
        }

        shader->Bind();
        SetShaderProperties(shader, mtrlPass->shaderProperties);
    } else {
        shader = ShaderManager::lightingDefaultShader;
        if (useShadowMap) {
            if (surfLight->def->parms.type == SceneLight::PointLight) {
                shader = shader->pointShadowVersion;
            } else if (surfLight->def->parms.type == SceneLight::SpotLight) {
                shader = shader->spotShadowVersion;
            } else if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
                shader = shader->parallelShadowVersion;
            }
        }

        if (subMesh && subMesh->useGpuSkinning) {
            if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
                shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
            }
        }

        shader->Bind();
        shader->SetTexture(shader->builtInSamplerUnits[Shader::DiffuseMapSampler], mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    const Mat4 &worldMatrix = surfEntity->def->GetModelMatrix();
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixSConst], worldMatrix[0]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixTConst], worldMatrix[1]);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::WorldMatrixRConst], worldMatrix[2]);

    // world coordinates -> entity's local coordinates
    Vec3 localViewOrigin = surfEntity->def->parms.axis.TransposedMulVec(backEnd.view->def->parms.origin - surfEntity->def->parms.origin) / surfEntity->def->parms.scale;
    Vec4 localLightOrigin;
    Vec3 lightInvRadius;

    if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
        localLightOrigin = Vec4(surfEntity->def->parms.axis.TransposedMulVec(-surfLight->def->parms.axis[0]), 1.0f);
        lightInvRadius.SetFromScalar(0);
    } else {
        localLightOrigin = Vec4(surfEntity->def->parms.axis.TransposedMulVec(surfLight->def->parms.origin - surfEntity->def->parms.origin) / surfEntity->def->parms.scale, 0.0f);
        lightInvRadius = 1.0f / surfLight->def->GetRadius();
    }

    Mat3 localLightAxis = surfEntity->def->parms.axis.TransposedMul(surfLight->def->parms.axis);
    localLightAxis[0] *= surfEntity->def->parms.scale;
    localLightAxis[1] *= surfEntity->def->parms.scale;
    localLightAxis[2] *= surfEntity->def->parms.scale;

    shader->SetConstant3f(shader->builtInConstantLocations[Shader::LocalViewOriginConst], localViewOrigin);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::LocalLightOriginConst], localLightOrigin);
    shader->SetConstant3x3f(shader->builtInConstantLocations[Shader::LocalLightAxisConst], false, localLightAxis);	

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, mtrlPass->vertexColorMode);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfEntity->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);

    if (subMesh && subMesh->useGpuSkinning) {
        const Mesh *mesh = surfEntity->def->parms.mesh;
        SetSkinningConstants(shader, mesh->skinningJointCache);
    }
        
    shader->SetConstant3f("lightInvRadius", lightInvRadius);
    shader->SetConstant1f("lightFallOffExponent", surfLight->def->parms.fallOffExponent);
    shader->SetConstant1i("removeBackProjection", surfLight->def->parms.type == SceneLight::SpotLight ? 1 : 0);
        
    if (useShadowMap) {
        if (surfLight->def->parms.type == SceneLight::PointLight) {
            shader->SetConstant2f("shadowProjectionDepth", backEnd.shadowProjectionDepth);
            shader->SetConstant1f("vscmBiasedScale", backEnd.ctx->vscmBiasedScale);
            shader->SetTexture("cubicNormalCubeMap", textureManager.cubicNormalCubeMapTexture);
            shader->SetTexture("indirectionCubeMap", backEnd.ctx->indirectionCubeMapTexture);
            shader->SetTexture("shadowMap", backEnd.ctx->vscmRT->DepthStencilTexture());
        } else if (surfLight->def->parms.type == SceneLight::SpotLight) {
            shader->SetConstant4x4f("shadowProjMatrix", true, backEnd.shadowViewProjectionScaleBiasMatrix[0]);
            shader->SetTexture("shadowArrayMap", backEnd.ctx->shadowMapRT->DepthStencilTexture());
        } else if (surfLight->def->parms.type == SceneLight::DirectionalLight) {
            shader->SetConstantArray4x4f("shadowCascadeProjMatrix", true, backEnd.csmCount, backEnd.shadowViewProjectionScaleBiasMatrix);

            if (r_CSM_selectionMethod.GetInteger() == 0) {
                // z-based selection shader needs shadowSplitFar value
                float sFar[4];
                for (int i = 0; i < backEnd.csmCount; i++) {
                    float dFar = backEnd.csmDistances[i + 1];
                    sFar[i] = (backEnd.projMatrix[2][2] * -dFar + backEnd.projMatrix[2][3]) / dFar;
                    sFar[i] = sFar[i] * 0.5f + 0.5f;
                }
                shader->SetConstant4f("shadowSplitFar", sFar);
            }
            shader->SetConstant1f("cascadeBlendSize", r_CSM_blendSize.GetFloat());
            shader->SetConstantArray1f("shadowMapFilterSize", r_CSM_count.GetInteger(), backEnd.shadowMapFilterSize);
            shader->SetTexture("shadowArrayMap", backEnd.ctx->shadowMapRT->DepthStencilTexture());
        }

        if (r_shadowMapQuality.GetInteger() == 3) {
            shader->SetTexture("randomRotMatMap", textureManager.randomRotMatTexture);
        }

        Vec2 shadowMapTexelSize;

        if (surfLight->def->parms.type == SceneLight::PointLight) {
            shadowMapTexelSize.x = 1.0f / backEnd.ctx->vscmRT->GetWidth();
            shadowMapTexelSize.y = 1.0f / backEnd.ctx->vscmRT->GetHeight();
        } else {
            shadowMapTexelSize.x = 1.0f / backEnd.ctx->shadowMapRT->GetWidth();
            shadowMapTexelSize.y = 1.0f / backEnd.ctx->shadowMapRT->GetHeight();
        }

        shader->SetConstant2f("shadowMapTexelSize", shadowMapTexelSize);
    } else {
        /*
        // WARNING: for the nvidia's stupid dynamic branching... 
        if (r_shadowMapQuality.GetInteger() == 3) {
            shader->SetTexture("randomRotMatMap", textureManager.randomRotMatTexture);
        }

        // WARNING: for the nvidia's stupid dynamic branching... 
        if (r_shadows.GetInteger() == 1) {
            shader->SetTexture("shadowArrayMap", backEnd.ctx->shadowMapRT->DepthStencilTexture());
        }*/
    }

    const Material *lightMaterial = surfLight->def->GetMaterial();

    shader->SetTexture("lightProjectionMap", lightMaterial->GetPass()->texture);
    shader->SetConstant4x4f("lightTextureMatrix", true, surfLight->viewProjTexMatrix);

    Color4 lightColor = surfLight->lightColor * surfLight->def->parms.intensity * r_lightScale.GetFloat();
    shader->SetConstant4f("lightColor", lightColor);

    //bool useLightCube = lightStage->textureStage.texture->GetType() == TextureCubeMap ? true : false;
    //shader->SetConstant1i("useLightCube", useLightCube);
    
/*	if (useLightCube) {
        shader->SetTexture("lightCubeMap", lightStage->textureStage.texture);
    } else {
        //shader->SetTexture("lightCubeMap", textureManager.m_defaultCubeMapTexture);
    }*/

    DrawPrimitives();
}

void RBSurf::RenderFogLightInteraction(const Material::Pass *mtrlPass) const {	
    const Shader *shader = ShaderManager::fogLightShader;

    if (subMesh && subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    shader->Bind();

    // light texture transform matrix
    Mat4 viewProjScaleBiasMat = surfLight->def->GetViewProjScaleBiasMatrix() * surfEntity->def->GetModelMatrix();	
    shader->SetConstant4x4f("lightTextureMatrix", true, viewProjScaleBiasMat);
    shader->SetConstant3f("fogColor", &surfLight->def->parms.materialParms[SceneEntity::RedParm]);

    Vec3 vec = surfLight->def->parms.origin - backEnd.view->def->parms.origin;
    bool fogEnter = vec.Dot(surfLight->def->parms.axis[0]) < 0.0f ? true : false;

    if (fogEnter) {
        shader->SetTexture("fogMap", textureManager.fogTexture);
        shader->SetTexture("fogEnterMap", textureManager.whiteTexture);
    } else {
        shader->SetTexture("fogMap", textureManager.whiteTexture);
        shader->SetTexture("fogEnterMap", textureManager.fogEnterTexture);
    }

    const Material *lightMaterial = surfLight->def->parms.material;
    shader->SetTexture("fogProjectionMap", lightMaterial->GetPass()->texture);

    DrawPrimitives();
}

void RBSurf::RenderBlendLightInteraction(const Material::Pass *mtrlPass) const {
    const Shader *shader = ShaderManager::blendLightShader;

    if (subMesh && subMesh->useGpuSkinning) {
        if (shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex]) {
            shader = shader->gpuSkinningVersion[subMesh->gpuSkinningVersionIndex];
        }
    }

    Color3 blendColor(&surfLight->def->parms.materialParms[SceneEntity::RedParm]);

    if (cvarSystem.GetCVarBool(L"gl_sRGB")) {
        blendColor = blendColor.SRGBtoLinear();
    }

    shader->Bind();

    // light texture transform matrix
    Mat4 viewProjScaleBiasMat = surfLight->def->GetViewProjScaleBiasMatrix() * surfEntity->def->GetModelMatrix();
    shader->SetConstant4x4f("lightTextureMatrix", true, viewProjScaleBiasMat);
    shader->SetConstant3f("blendColor", blendColor);

    const Material *lightMaterial = surfLight->def->parms.material;
    shader->SetTexture("blendProjectionMap", lightMaterial->GetPass()->texture);	

    DrawPrimitives();
}

void RBSurf::RenderGui(const Material::Pass *mtrlPass) const {
    const Shader *shader;

    if (mtrlPass->shader) {
        shader = mtrlPass->shader;
        shader->Bind();

        SetShaderProperties(shader, mtrlPass->shaderProperties);
    } else {
        shader = ShaderManager::guiShader;
        shader->Bind();

        shader->SetTexture("map", mtrlPass->texture);
    }

    SetMatrixConstants(shader);

    Vec4 textureMatrixS = Vec4(mtrlPass->tcScale[0], 0.0f, 0.0f, mtrlPass->tcTranslation[0]);
    Vec4 textureMatrixT = Vec4(0.0f, mtrlPass->tcScale[1], 0.0f, mtrlPass->tcTranslation[1]);

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixSConst], textureMatrixS);
    shader->SetConstant4f(shader->builtInConstantLocations[Shader::TextureMatrixTConst], textureMatrixT);

    SetVertexColorConstants(shader, Material::ModulateVertexColor);

    Color4 color;
    if (mtrlPass->useOwnerColor) {
        color = Color4(&surfEntity->def->parms.materialParms[SceneEntity::RedParm]);
    } else {
        color = mtrlPass->constantColor;
    }

    shader->SetConstant4f(shader->builtInConstantLocations[Shader::ConstantColorConst], color);
    
    DrawPrimitives();
}

BE_NAMESPACE_END
