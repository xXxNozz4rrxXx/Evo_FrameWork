#pragma once

#include "../singleton.hpp"

class IMatRenderContext;
struct DrawModelState_t;
struct ModelRenderInfo_t;
class matrix3x4_t;
class IMaterial;
class Color;

class Chams
    : public Singleton<Chams>
{
    friend class Singleton<Chams>;

    Chams();
    ~Chams();

public:
    void OnDrawModelExecute(
        IMatRenderContext* ctx,
        const DrawModelState_t &state,
        const ModelRenderInfo_t &pInfo,
        matrix3x4_t *pCustomBoneToWorld);
	void OnSceneEnd();

private:
    void OverrideMaterial(bool ignoreZ, bool flat, bool wireframe,bool metallic, bool glass, const Color& rgba);

    IMaterial* materialRegular = nullptr;
    IMaterial* materialRegularIgnoreZ = nullptr;
    IMaterial* materialFlatIgnoreZ = nullptr;
    IMaterial* materialFlat = nullptr;
	IMaterial* materialMetalicIgnorez = nullptr;
	IMaterial* materialMetalic = nullptr;
};