#include "rendering/AssetManager.h"
#include <algorithm>
#include <queue>
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
bool NearWhite(const Color& c){const int mx=std::max({(int)c.r,(int)c.g,(int)c.b});const int mn=std::min({(int)c.r,(int)c.g,(int)c.b});return c.a>0&&mn>=235&&(mx-mn)<=18;}

// DF-013: corrige el "aura blanca" / "mal cortados" reportado en los enemigos.
// Diagnostico (verificado sobre los PNG reales en assets/enemies/): los atlas
// YA vienen con canal alfa parcial en los bordes (anti-aliasing), pero el color
// RGB de esos pixeles de alfa bajo esta contaminado hacia blanco -- es decir,
// fueron generados/compuestos sobre un lienzo blanco y el color se guardo sin
// "des-mezclar" (straight alpha sin decontaminar). Al dibujarlos con blending
// normal sobre el fondo oscuro del juego, esa contaminacion se ve como un halo
// blanco alrededor de la silueta (y en huecos internos como axilas o entre
// piernas). El flood-fill de borde de mas abajo no lo arregla porque esos
// pixeles no son "blanco puro": son una mezcla progresiva.
//
// La correccion es la tecnica estandar de "unpremultiply contra un fondo
// conocido": para cada pixel con alfa parcial, se asume
//   color_guardado = color_real * a + blanco * (1 - a)
// y se despeja color_real = (color_guardado - blanco*(1-a)) / a.
// Los pixeles casi invisibles (alfa muy bajo) se descartan directamente:
// despejar la ecuacion ahi es numericamente inestable y no aportan nada visible.
void DecontaminateWhiteMatte(Image& image, int alphaFloor = 14) {
    const int w = image.width, h = image.height;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const Color c = GetImageColor(image, x, y);
            if (c.a == 0) continue;
            if (c.a < alphaFloor) { ImageDrawPixel(&image, x, y, {0, 0, 0, 0}); continue; }
            if (c.a >= 255) continue;  // opaco: nada que decontaminar
            const float alpha = static_cast<float>(c.a) / 255.0f;
            const float bleed = 255.0f * (1.0f - alpha);
            const auto fix = [&](unsigned char channel) -> unsigned char {
                const float trueValue = (static_cast<float>(channel) - bleed) / alpha;
                return static_cast<unsigned char>(std::clamp(trueValue, 0.0f, 255.0f));
            };
            ImageDrawPixel(&image, x, y, {fix(c.r), fix(c.g), fix(c.b), c.a});
        }
    }
}

Texture2D LoadEnemyTexture(const char* key,const std::vector<std::string>& candidates){
    const std::string path=ResolveAssetPath(candidates);
    if(path.empty()||!FileExists(path.c_str())){TraceLog(LOG_WARNING,"District Fury enemy asset missing: %s",key);return Texture2D{0};}
    Image image=LoadImage(path.c_str());
    if(image.data==nullptr){TraceLog(LOG_WARNING,"District Fury enemy asset failed to load: %s (%s)",key,path.c_str());return Texture2D{0};}
    ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if(image.data==nullptr){UnloadImage(image);return Texture2D{0};}
    const int w=image.width,h=image.height; std::vector<unsigned char> visited((size_t)w*h,0); std::queue<int> queue;
    const auto enqueue=[&](int x,int y){const int idx=y*w+x;if(!visited[(size_t)idx]&&NearWhite(GetImageColor(image,x,y))){visited[(size_t)idx]=1;queue.push(idx);}};
    // Paso 1: si existiera un fondo blanco solido (assets futuros sin alfa
    // previo), se sigue eliminando por flood-fill desde el borde.
    for(int x=0;x<w;++x){enqueue(x,0);enqueue(x,h-1);} for(int y=0;y<h;++y){enqueue(0,y);enqueue(w-1,y);}
    const int dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    while(!queue.empty()){const int idx=queue.front();queue.pop();const int x=idx%w,y=idx/w;ImageDrawPixel(&image,x,y,{0,0,0,0});for(int i=0;i<4;++i){const int nx=x+dx[i],ny=y+dy[i];if(nx>=0&&nx<w&&ny>=0&&ny<h)enqueue(nx,ny);}}
    // Paso 2: decontaminar el matte blanco de TODOS los bordes anti-aliasados
    // (el fix real para el halo reportado).
    DecontaminateWhiteMatte(image);
    // Paso 3: red de seguridad. Si tras decontaminar todavia queda algun
    // pixel casi blanco pegado a zona transparente (rincones muy comprimidos),
    // se recorta ese ultimo anillo.
    for(int y=0;y<h;++y)for(int x=0;x<w;++x){const Color c=GetImageColor(image,x,y);if(c.a==0||!NearWhite(c))continue;bool touchesTransparent=false;for(int i=0;i<4;++i){const int nx=x+dx[i],ny=y+dy[i];if(nx>=0&&nx<w&&ny>=0&&ny<h&&GetImageColor(image,nx,ny).a==0){touchesTransparent=true;break;}}if(touchesTransparent)ImageDrawPixel(&image,x,y,{c.r,c.g,c.b,0});}
    Texture2D texture=LoadTextureFromImage(image); UnloadImage(image); if(texture.id==0)return Texture2D{0}; SetTextureFilter(texture,TEXTURE_FILTER_POINT); return texture;
}
}
void AssetManager::LoadAll(){
    if(!textures.empty())return;
    textures["bg_industrial"]=LoadRequiredTexture("old_steel_yard_clean",{"assets/backgrounds/old_steel_yard_clean.png","../assets/backgrounds/old_steel_yard_clean.png","../../assets/backgrounds/old_steel_yard_clean.png"},TEXTURE_FILTER_BILINEAR);
    textures["bg_steel_deep"]=LoadRequiredTexture("old_steel_yard_deep_clean",{"assets/backgrounds/old_steel_yard_deep_clean.png","../assets/backgrounds/old_steel_yard_deep_clean.png","../../assets/backgrounds/old_steel_yard_deep_clean.png"},TEXTURE_FILTER_BILINEAR);
    textures["bg_mercado_antiguo"]=LoadRequiredTexture("mercado_antiguo_clean",{"assets/backgrounds/mercado_antiguo_clean.png","../assets/backgrounds/mercado_antiguo_clean.png","../../assets/backgrounds/mercado_antiguo_clean.png"},TEXTURE_FILTER_POINT);
    textures["bg_zona_quimica"]=LoadRequiredTexture("zona_quimica_clean",{"assets/backgrounds/zona_quimica_clean.png","../assets/backgrounds/zona_quimica_clean.png","../../assets/backgrounds/zona_quimica_clean.png"},TEXTURE_FILTER_POINT);
    textures["rayden_clean"]=LoadRequiredTexture("rayden_clean",{"assets/characters/rayden_clean.png","../assets/characters/rayden_clean.png","../../assets/characters/rayden_clean.png"},TEXTURE_FILTER_POINT);
    textures["punk_clean"]=LoadEnemyTexture("punk_clean",{"assets/enemies/punk_clean.png","../assets/enemies/punk_clean.png","../../assets/enemies/punk_clean.png"});
    textures["charger_clean"]=LoadEnemyTexture("charger_clean",{"assets/enemies/charger_clean.png","../assets/enemies/charger_clean.png","../../assets/enemies/charger_clean.png"});
    textures["brute_clean"]=LoadEnemyTexture("brute_clean",{"assets/enemies/brute_clean.png","../assets/enemies/brute_clean.png","../../assets/enemies/brute_clean.png"});
    textures["enforcer_clean"]=LoadEnemyTexture("enforcer_clean",{"assets/enemies/enforcer_clean.png","../assets/enemies/enforcer_clean.png","../../assets/enemies/enforcer_clean.png"});
    textures["chemical_soldier_clean"]=LoadEnemyTexture("chemical_soldier_clean",{"assets/enemies/chemical_soldier_clean.png","../assets/enemies/chemical_soldier_clean.png","../../assets/enemies/chemical_soldier_clean.png"});
    textures["urban_ninja_clean"]=LoadEnemyTexture("urban_ninja_clean",{"assets/enemies/urban_ninja_clean.png","../assets/enemies/urban_ninja_clean.png","../../assets/enemies/urban_ninja_clean.png"});
    textures["mutant_clean"]=LoadEnemyTexture("mutant_clean",{"assets/enemies/mutant_clean.png","../assets/enemies/mutant_clean.png","../../assets/enemies/mutant_clean.png"});
    textures["armored_guard_clean"]=LoadEnemyTexture("armored_guard_clean",{"assets/enemies/armored_guard_clean.png","../assets/enemies/armored_guard_clean.png","../../assets/enemies/armored_guard_clean.png"});
    textures["menu_main_art"]=LoadRequiredTexture("menu_main_art",{"assets/ui/menu_main_art.png","../assets/ui/menu_main_art.png","../../assets/ui/menu_main_art.png"},TEXTURE_FILTER_BILINEAR);
}
void AssetManager::UnloadAll(){for(auto&pair:textures)if(pair.second.id!=0)UnloadTexture(pair.second);textures.clear();}
Texture2D AssetManager::GetTexture(const std::string&name){const auto it=textures.find(name);return it!=textures.end()?it->second:Texture2D{0};}
}
