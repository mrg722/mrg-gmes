#include "rendering/AssetManager.h"
#include <string>
#include <vector>

namespace district_fury {
namespace {
std::string ResolveAssetPath(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) if (FileExists(path.c_str())) return path;
    return candidates.empty()?std::string{}:candidates.front();
}
Texture2D LoadRequiredTexture(const char* key,const std::vector<std::string>& candidates,TextureFilter filter){
    const std::string path=ResolveAssetPath(candidates);
    if(path.empty()||!FileExists(path.c_str())){TraceLog(LOG_WARNING,"District Fury asset missing: %s",key);return Texture2D{0};}
    Texture2D texture=LoadTexture(path.c_str());
    if(texture.id==0){TraceLog(LOG_WARNING,"District Fury asset failed to load: %s (%s)",key,path.c_str());return Texture2D{0};}
    SetTextureFilter(texture,filter);return texture;
}
}
void AssetManager::LoadAll(){
    if(!textures.empty())return;
    textures["bg_industrial"]=LoadRequiredTexture("old_steel_yard_clean",{"assets/backgrounds/old_steel_yard_clean.png","../assets/backgrounds/old_steel_yard_clean.png","../../assets/backgrounds/old_steel_yard_clean.png"},TEXTURE_FILTER_BILINEAR);
    textures["bg_steel_deep"]=LoadRequiredTexture("old_steel_yard_deep_clean",{"assets/backgrounds/old_steel_yard_deep_clean.png","../assets/backgrounds/old_steel_yard_deep_clean.png","../../assets/backgrounds/old_steel_yard_deep_clean.png"},TEXTURE_FILTER_BILINEAR);
    textures["rayden_clean"]=LoadRequiredTexture("rayden_clean",{"assets/characters/rayden_clean.png","../assets/characters/rayden_clean.png","../../assets/characters/rayden_clean.png"},TEXTURE_FILTER_POINT);
    textures["punk_clean"]=LoadRequiredTexture("punk_clean",{"assets/enemies/punk_clean.png","../assets/enemies/punk_clean.png","../../assets/enemies/punk_clean.png"},TEXTURE_FILTER_POINT);
    textures["charger_clean"]=LoadRequiredTexture("charger_clean",{"assets/enemies/charger_clean.png","../assets/enemies/charger_clean.png","../../assets/enemies/charger_clean.png"},TEXTURE_FILTER_POINT);
    textures["brute_clean"]=LoadRequiredTexture("brute_clean",{"assets/enemies/brute_clean.png","../assets/enemies/brute_clean.png","../../assets/enemies/brute_clean.png"},TEXTURE_FILTER_POINT);
    textures["enforcer_clean"]=LoadRequiredTexture("enforcer_clean",{"assets/enemies/enforcer_clean.png","../assets/enemies/enforcer_clean.png","../../assets/enemies/enforcer_clean.png"},TEXTURE_FILTER_POINT);
    textures["chemical_soldier_clean"]=LoadRequiredTexture("chemical_soldier_clean",{"assets/enemies/chemical_soldier_clean.png","../assets/enemies/chemical_soldier_clean.png","../../assets/enemies/chemical_soldier_clean.png"},TEXTURE_FILTER_POINT);
    textures["urban_ninja_clean"]=LoadRequiredTexture("urban_ninja_clean",{"assets/enemies/urban_ninja_clean.png","../assets/enemies/urban_ninja_clean.png","../../assets/enemies/urban_ninja_clean.png"},TEXTURE_FILTER_POINT);
    textures["mutant_clean"]=LoadRequiredTexture("mutant_clean",{"assets/enemies/mutant_clean.png","../assets/enemies/mutant_clean.png","../../assets/enemies/mutant_clean.png"},TEXTURE_FILTER_POINT);
    textures["armored_guard_clean"]=LoadRequiredTexture("armored_guard_clean",{"assets/enemies/armored_guard_clean.png","../assets/enemies/armored_guard_clean.png","../../assets/enemies/armored_guard_clean.png"},TEXTURE_FILTER_POINT);
}
void AssetManager::UnloadAll(){for(auto&pair:textures)if(pair.second.id!=0)UnloadTexture(pair.second);textures.clear();}
Texture2D AssetManager::GetTexture(const std::string&name){const auto it=textures.find(name);return it!=textures.end()?it->second:Texture2D{0};}
}
