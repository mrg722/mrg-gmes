#pragma once
#include "raylib.h"
#include <string>
#include <utility>

namespace district_fury {
inline std::string TranslateGameText(const char* text){
    if(!text)return{};std::string s(text);
    const std::pair<const char*,const char*> r[]={
        {"OLD STEEL YARD // DEEP LINE","VIEJO ASTILLERO // LINEA PROFUNDA"},{"OLD STEEL YARD","VIEJO ASTILLERO"},{"DEEP LINE","LINEA PROFUNDA"},
        {"ASTRA TOWER // PUBLIC ATRIUM","TORRE ASTRA // ATRIO PUBLICO"},{"ASTRA TOWER // RESEARCH FLOOR","TORRE ASTRA // PISO DE INVESTIGACION"},{"ASTRA TOWER // EXECUTIVE CORE","TORRE ASTRA // NUCLEO EJECUTIVO"},{"ASTRA TOWER","TORRE ASTRA"},{"PUBLIC ATRIUM","ATRIO PUBLICO"},{"RESEARCH FLOOR","PISO DE INVESTIGACION"},{"EXECUTIVE CORE","NUCLEO EJECUTIVO"},{"STEEL YARD","ASTILLERO DE ACERO"},
        {"STAGE 3 CLEAR","ESCENARIO 3 COMPLETADO"},{"STAGE 2 CLEAR","ESCENARIO 2 COMPLETADO"},{"STAGE 1 CLEAR","ESCENARIO 1 COMPLETADO"},{"STAGE CLEAR","ESCENARIO COMPLETADO"},{"GAME OVER","HAS CAIDO"},{"PAUSED","PAUSA"},{"RESUME","REANUDAR"},{"RESTART","REINICIAR"},{"CONTROLS","CONTROLES"},{"DIFFICULTY","DIFICULTAD"},
        {"SCORE","PUNTOS"},{"TIME","TIEMPO"},{"DAMAGE TAKEN","DANO RECIBIDO"},{"DAMAGE","DANO RECIBIDO"},{"MAX COMBO","COMBO MAXIMO"},{"WAVE","OLEADA"},{"PHASE","FASE"},{"HP","VIDA"},{"SP","ENERGIA"},{"RAGE","FURIA"},{"ENERGY","PODER"},{"SHIELD","ESCUDO"},{"GUARD","BLOQUEO"},{"BLOCK","BLOQUEO"},{"AREA LOCKED","AREA BLOQUEADA"},{"DEFEAT THE GATEKEEPER","DERROTA AL GUARDIAN"},
        {"NEXT","SIGUIENTE"},{"CLEAR","COMPLETADO"},{"THE CHAIN ROUTE","LA RUTA DE LAS CADENAS"},{"CHAIN ROUTE","RUTA DE LAS CADENAS"},{"TITAN-X has been shut down.","TITAN-X ha sido derrotado."},
        {"ENTER / J — START","ENTER / J — COMENZAR"},{"ENTER / J — START STORY","ENTER / J — COMENZAR HISTORIA"},{"ENTER / J — START","ENTER / J — COMENZAR"},{"ENTER / R — CONTINUE","ENTER / R — CONTINUAR"},{"R — RESET SAVE","R — BORRAR GUARDADO"},{"ESC — EXIT","ESC — SALIR"},{"ESC  EXIT","ESC  SALIR"},
        {"ENTER / J  —  START","ENTER / J  —  COMENZAR"},{"ENTER / J — START","ENTER / J — COMENZAR"},{"ENTER / R — CONTINUE","ENTER / R — CONTINUAR"},{"R  RESTART","R  REINICIAR"},{"R — RESTART","R — REINICIAR"},{"Q  MENU","Q  MENU"},{"Q — MENU","Q — MENU"},{"ESC  RESUME","ESC  REANUDAR"},
        {"Break the security cordon.","Rompe el cordon de seguridad."},{"Cross the restricted research floor.","Cruza el piso de investigacion restringido."},{"Reach the executive core and expose Titan-X.","Llega al nucleo ejecutivo y enfrenta a Titan-X."},{"RAYDEN CRUZ // VIEJO ASTILLERO","RAYDEN CRUZ // VIEJO ASTILLERO"},{"GRINDER // EL SEGADOR","GRINDER // EL SEGADOR"},
        {"A STORY BEAT-EM-UP","UN BEAT-EM-UP DE HISTORIA"},{"BEAT-EM-UP DE HISTORIA","BEAT-EM-UP DE HISTORIA"},{"START","COMENZAR"},{"EXIT","SALIR"},{"ENEMIES","ENEMIGOS"},{"BOSS","JEFE"},{"RANK","RANGO"},{"NUEVA PARTIDA","NUEVA PARTIDA"}
    };for(const auto&x:r){std::string from(x.first),to(x.second);std::size_t p=0;while((p=s.find(from,p))!=std::string::npos){s.replace(p,from.size(),to);p+=to.size();}}return s;
}
inline void DrawTextTranslated(const char* text,int posX,int posY,int fontSize,Color color){const std::string translated=TranslateGameText(text);::DrawText(translated.c_str(),posX,posY,fontSize,color);}
}
#define DrawText district_fury::DrawTextTranslated
