#pragma once
#include "raylib.h"
#include <string>
#include <utility>

namespace district_fury {
inline std::string TranslateGameText(const char* text){
    if(!text)return{};std::string s(text);
    const std::pair<const char*,const char*> r[]={
        {"OLD STEEL YARD","VIEJO ASTILLERO"},{"DEEP LINE","LINEA PROFUNDA"},{"ASTRA TOWER","TORRE ASTRA"},{"PUBLIC ATRIUM","ATRIUM PUBLICO"},{"RESEARCH FLOOR","PISO DE INVESTIGACION"},{"EXECUTIVE CORE","NUCLEO EJECUTIVO"},{"STEEL YARD","ASTILLERO DE ACERO"},{"STAGE CLEAR","ETAPA COMPLETADA"},{"GAME OVER","HAS CAIDO"},{"PAUSED","PAUSA"},{"RESUME","REANUDAR"},{"RESTART","REINICIAR"},{"CONTROLS","CONTROLES"},{"DIFFICULTY","DIFICULTAD"},{"SCORE","PUNTOS"},{"TIME","TIEMPO"},{"DAMAGE","DANO"},{"MAX COMBO","COMBO MAXIMO"},{"WAVE","OLEADA"},{"PHASE","FASE"},{"HP","VIDA"},{"SP","ENERGIA"},{"RAGE","FURIA"},{"ENERGY","PODER"},{"GUARD","BLOQUEO"},{"BLOCK","BLOQUEO"},{"AREA LOCKED","AREA BLOQUEADA"},{"DEFEAT THE GATEKEEPER","DERROTA AL GUARDIAN"},{"NEXT","SIGUIENTE"},{"CLEAR","COMPLETADO"},{"THE CHAIN ROUTE","LA RUTA DE LAS CADENAS"},{"CHAIN ROUTE","RUTA DE LAS CADENAS"},{"TITAN-X has been shut down.","TITAN-X ha sido derrotado."}
    };for(const auto&x:r){std::string from(x.first),to(x.second);std::size_t p=0;while((p=s.find(from,p))!=std::string::npos){s.replace(p,from.size(),to);p+=to.size();}}return s;
}
inline void DrawText(const char* text,int posX,int posY,int fontSize,Color color){const std::string translated=TranslateGameText(text);::DrawText(translated.c_str(),posX,posY,fontSize,color);}
}
