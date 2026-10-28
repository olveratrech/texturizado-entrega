// Composición aprobada importada íntegramente desde Prueba_altar_OpenGL.
// Sólo cambia la raíz local; la posición global se aplica después de construirla.
#include "scene/Escena.h"
#include "scene/ApoyoDulces.h"
#include "Registro.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <random>
#include <stdexcept>

namespace {
// Izquierda/derecha siempre vistas de frente: -X/+X. Frente: +Z.
// Los límites incluyen el mantel. La franja frontal comienza en el piso superior.
struct Piso { float y, mitadAncho, frente, fondoFranja; };
constexpr Piso pisos[] = {
 {.454f, 1.8375f, 1.4725f, .8075f},
 {.904f, 1.4625f, .8075f, .1425f},
 {1.354f, 1.0875f, .1425f, -1.425f}
};
}

void Escena::construirAltarAprobado(Nodo* altarAprobado) {
 auto* altar = instanciar("altar_muertos", altarAprobado, "Mesa de tres pisos y petate", {0, 0, 0});
 altar->escala = {1.5f, 1.0f, 1.9f};
 // Restaurar el petate con el fondo original de la composición anterior.
 if (auto* petate = altar->buscar("petate")) petate->escala.z = 1.0f / 1.9f;

 const Modelo& jarron = *modelo("jarron_talavera");
 // El archivo actual incluye el ramo. El cuerpo mide 0.305; lo dejamos en 0.30.
 constexpr float escala = .30f / .305f;
 constexpr float margen = .06f;
 const glm::vec3 tam = (jarron.max - jarron.min) * escala;
 const float centroX = (jarron.min.x + jarron.max.x) * .5f;
 const float centroZ = (jarron.min.z + jarron.max.z) * .5f;
 for (int nivel = 0; nivel < 2; ++nivel) {
  const Piso& piso = pisos[nivel];
  for (int lado : {-1, 1}) {
   const float x = lado * (piso.mitadAncho - margen - tam.x * .5f);
   const float z = piso.frente - margen - tam.z * .5f;
   const std::string nombre = "Piso " + std::to_string(nivel + 1) +
       " · esquina delantera " + (lado < 0 ? "izquierda" : "derecha");
   auto* grupo = altarAprobado->agregar(nombre);
   grupo->posicion = {x, piso.y, z};
   instanciar("jarron_talavera", grupo, "Jarrón con ramo integrado",
       {-centroX * escala, -jarron.baseY * escala, -centroZ * escala}, 0, escala);
   // Comprobar toda la envolvente, incluido el ramo, dentro de la franja frontal.
   if (std::abs(x) + tam.x * .5f > piso.mitadAncho - margen + .0001f ||
       z + tam.z * .5f > piso.frente - margen + .0001f ||
       z - tam.z * .5f < piso.fondoFranja + margen)
    throw std::runtime_error("Jarrón fuera de su franja: " + nombre);
   registro::info("JARRON: %s, centro XZ=(%.4f, %.4f), apoyo Y=%.3f, altura total=%.3f",
       nombre.c_str(), x, z, piso.y, tam.y);
  }
 }

 // Retícula global A–T / 1–20. La fila fraccionaria es el punto medio entre filas.
 struct GrupoVelas { int piso; char columna; float fila; };
 const GrupoVelas ubicaciones[] = {
  {1,'Q',5}, {1,'Q',8}, {1,'Q',11}, {1,'D',5}, {1,'D',8}, {1,'D',11},
  {1,'F',13}, {1,'O',13}, {2,'E',6.5f}, {2,'P',6.5f}, {2,'G',10.5f}, {2,'N',10.5f}
 };
 // Radios horizontales medidos sobre los vértices actuales, centrados en su AABB.
 // Incluyen el contorno irregular del vaso de veladora_4.
 constexpr float radios[] = {.042001f, .035501f, .034001f, .056099f};
 std::array<Lote*,4> velas;
 for (int k=0; k<4; ++k) {
  const std::string clave = "veladora_" + std::to_string(k+1);
  velas[k] = nodoLote(clave, altarAprobado, "Veladoras compartidas · " + clave)->lote;
 }
 std::mt19937 azar(261026); // Selección aleatoria reproducible al abrir la escena.
 struct HuellaVela { glm::vec2 centro; float radio; int piso; };
 std::vector<HuellaVela> huellasVelas;
 int grupoIndice = 0;
 for (auto u : ubicaciones) {
  // Mantener la selección de variantes de los grupos originales, incluso al retirar dos.
  if (u.piso==2 && (u.columna=='E' || u.columna=='P')) u.fila=8;
  const Piso& piso = pisos[u.piso-1];
  const glm::vec2 centro(-2.5f + (u.columna-'A'+.5f)*.25f, -2.f+(u.fila-.5f)*.25f);
  std::array<int,4> variantes{0,1,2,3};
  std::shuffle(variantes.begin(), variantes.end(), azar);
  if (u.piso==1 && u.fila==8 && (u.columna=='Q' || u.columna=='D')) continue;
  // Las dos menores atrás (orejas), la más ancha delante (cabeza).
  std::sort(variantes.begin(), variantes.begin()+3, [&](int a,int b){return radios[a]<radios[b];});
  float r[3]; for (int k=0;k<3;++k) r[k]=radios[variantes[k]];
  constexpr float separacion=.002f; // 2 mm entre los círculos envolventes, antes de escalar.
  const float a=r[0]+r[1]+separacion, b=r[0]+r[2]+separacion, c=r[1]+r[2]+separacion;
  const float dx=(b*b+a*a-c*c)/(2*a);
  glm::vec2 puntos[3]={{0,0},{a,0},{dx,std::sqrt(b*b-dx*dx)}};
  glm::vec2 lo(1e6f),hi(-1e6f);
  for (int k=0;k<3;++k) {lo=glm::min(lo,puntos[k]-r[k]);hi=glm::max(hi,puntos[k]+r[k]);}
  const glm::vec2 mitad=(hi-lo)*.5f, medio=(lo+hi)*.5f;
  for (auto& punto:puntos) punto-=medio;
  // Mantener el centro exacto de la celda; adaptar todo el grupo a su franja.
  float xmin=-piso.mitadAncho, xmax=piso.mitadAncho;
  float zmin=piso.fondoFranja, zmax=piso.frente;
  if (centro.y<piso.fondoFranja) {
   zmin=-1.425f;
   if (centro.x<0) xmax=-pisos[u.piso].mitadAncho;
   else xmin=pisos[u.piso].mitadAncho;
  }
  constexpr float borde=.015f;
  const float s=std::min({1.f,(centro.x-xmin-borde)/mitad.x,(xmax-centro.x-borde)/mitad.x,
      (centro.y-zmin-borde)/mitad.y,(zmax-centro.y-borde)/mitad.y});
  if (s<=0) throw std::runtime_error("Celda de veladoras fuera de la superficie");
  ++grupoIndice;
  registro::info("GRUPO %02d: P%d %c%.1f, XZ=(%.4f,%.4f), escala=%.4f, variantes=%d/%d/%d",
      grupoIndice,u.piso,u.columna,u.fila,centro.x,centro.y,s,variantes[0]+1,variantes[1]+1,variantes[2]+1);
  for (int k=0;k<3;++k) {
   const glm::vec2 punto=centro+puntos[k]*s;
   const float radio=r[k]*s;
   if (punto.x-radio<xmin+borde-.0001f || punto.x+radio>xmax-borde+.0001f ||
       punto.y-radio<zmin+borde-.0001f || punto.y+radio>zmax-borde+.0001f)
    throw std::runtime_error("Veladora fuera de su franja");
   for (const auto& anterior:huellasVelas)
    if (anterior.piso==u.piso && glm::distance(anterior.centro,punto)<anterior.radio+radio)
     throw std::runtime_error("Intersección entre veladoras");
   // Comprobar también los jarrones existentes en este piso, incluido su ramo.
   for (int lado:{-1,1}) {
    const glm::vec2 j(lado*(piso.mitadAncho-margen-tam.x*.5f),piso.frente-margen-tam.z*.5f);
    if (std::abs(punto.x-j.x)<radio+tam.x*.5f && std::abs(punto.y-j.y)<radio+tam.z*.5f)
     throw std::runtime_error("Veladora invade la envolvente de un jarrón");
   }
   huellasVelas.push_back({punto,radio,u.piso});
   const int v=variantes[k];const std::string clave="veladora_"+std::to_string(v+1);
   const Modelo& m=*modelo(clave);
   const glm::vec3 offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f);
   const glm::vec3 pos=glm::vec3(punto.x,piso.y,punto.y)-offset*s;
   velas[v]->agregar(glm::translate(glm::mat4(1),pos)*glm::scale(glm::mat4(1),glm::vec3(s)));
   info(clave)->usos++;
  }
 }
 registro::info("VELADORAS: %d grupos, %zu piezas; apoyos, bordes y separaciones comprobados",grupoIndice,huellasVelas.size());

 // Líneas reducidas. Secuencias explícitas: mezcla fija sin variantes consecutivas iguales.
 struct VelaLinea { glm::vec2 punto; int variante; int piso; };
 std::vector<VelaLinea> nuevas;
 constexpr float hueco=.05f; // Mínimo de seguridad; las nuevas líneas quedan más abiertas.
 auto linea = [&](int nivel, float xmin, float xmax, const std::vector<int>& tipos) {
  // Centros alineados: el vaso más ancho queda a 6 cm del borde delantero;
  // los menores conservan entre 6 y 8.21 cm de margen.
  const float z=pisos[nivel-1].frente-.06f-radios[3];
  for(size_t i=0;i<tipos.size();++i) {
   if(i && tipos[i]==tipos[i-1])throw std::runtime_error("Variantes consecutivas repetidas");
   const float x=xmin+(xmax-xmin)*float(i)/float(tipos.size()-1);
   nuevas.push_back({{x,z},tipos[i]-1,nivel});
  }
  registro::info("LINEA P%d: %zu veladoras, X=%.4f a %.4f, Z=%.4f, paso uniforme=%.2f cm",
      nivel,tipos.size(),xmin,xmax,z,100*(xmax-xmin)/(tipos.size()-1));
 };
 linea(1,-.875f,.875f,{1,4,2,3,1,3,4});
 linea(2,-.625f,.625f,{3,1,4,2,1});
 // U completa, abierta atrás: 12 en lugar de 37, con paso uniforme sobre el recorrido.
 // Mantener los extremos posteriores; retirar laterales y frente a un mínimo de 8 cm.
 const float retiro=.08f+radios[3];
 const float xU=pisos[2].mitadAncho-retiro;
 const float zFrente=pisos[2].frente-retiro;
 const float zAtras=-1.425f+.025f+radios[3];
 const float lateral=zFrente-zAtras,frontal=2*xU,longitud=2*lateral+frontal;
 const std::array<int,12> tiposU{2,4,1,3,2,1,4,3,1,2,4,3};
 for(size_t i=0;i<tiposU.size();++i) {
  if(i && tiposU[i]==tiposU[i-1])throw std::runtime_error("Variantes consecutivas repetidas en U");
  const float distancia=longitud*float(i)/float(tiposU.size()-1);
  glm::vec2 punto;
  if(distancia<=lateral) punto={-xU,zAtras+distancia};
  else if(distancia<=lateral+frontal) punto={-xU+distancia-lateral,zFrente};
  else punto={xU,zFrente-(distancia-lateral-frontal)};
  nuevas.push_back({punto,tiposU[i]-1,3});
 }
 registro::info("U P3: 12 veladoras, paso sobre recorrido=%.2f cm, margen lateral/frontal >=8 cm",
     100*longitud/(tiposU.size()-1));
 std::array<int,3> cantidad{};
 for(const auto& vela:nuevas) {
  const int nivel=vela.piso,v=vela.variante;
  const auto& piso=pisos[nivel-1];const auto& punto=vela.punto;
  const float radio=radios[v];
  const std::string clave="veladora_"+std::to_string(v+1);
  const Modelo& m=*modelo(clave);
  if(std::abs(punto.x)+radio>piso.mitadAncho-.015f || punto.y-radio< -1.425f+.015f ||
     punto.y+radio>piso.frente-.015f)
   throw std::runtime_error("Veladora de línea fuera del mantel");
  if(nivel<3 && std::abs(punto.x)-radio<pisos[nivel].mitadAncho+.015f &&
     punto.y-radio<piso.fondoFranja+.015f)
   throw std::runtime_error("Veladora de línea invade el siguiente piso");
  for(const auto& otra:huellasVelas)
   if(otra.piso==nivel && glm::distance(otra.centro,punto)<otra.radio+radio+hueco-.0001f)
    throw std::runtime_error("Veladora de línea sin los 5 cm de separación");
  // Revisar todos los jarrones cuyos intervalos verticales se solapan.
  for(int jNivel=0;jNivel<2;++jNivel) {
   const auto& jp=pisos[jNivel];
   if(piso.y>=jp.y+tam.y || piso.y+(m.max.y-m.baseY)<=jp.y)continue;
   for(int lado:{-1,1}) {
    const glm::vec2 j(lado*(jp.mitadAncho-margen-tam.x*.5f),jp.frente-margen-tam.z*.5f);
    if(std::abs(punto.x-j.x)<radio+tam.x*.5f && std::abs(punto.y-j.y)<radio+tam.z*.5f)
     throw std::runtime_error("Línea de veladoras invade un jarrón");
   }
  }
  huellasVelas.push_back({punto,radio,nivel});
  const glm::vec3 offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f);
  velas[v]->agregar(glm::translate(glm::mat4(1),glm::vec3(punto.x,piso.y,punto.y)-offset));
  info(clave)->usos++;cantidad[nivel-1]++;
 }
 registro::info("LINEAS: P1=%d, P2=%d, P3=%d; %zu nuevas; total veladoras=%zu; sin choques y huecos >=5 cm",
     cantidad[0],cantidad[1],cantidad[2],nuevas.size(),huellasVelas.size());


 // Veladoras aprobadas: toda colocación nueva se adapta a ellas, nunca las mueve.
 struct PlatoPedido { int piso; char columna; float fila; const char* clave; float giro; float entreColumnas=0; };
 // Orientaciones fijas alrededor de Y (vertical), sin inclinar los recipientes.
 const PlatoPedido pedidos[] = {
  {1,'D',8,"plato_mole",12}, {1,'Q',8,"plato_carnitas",-9},
  {1,'G',13,"plato_tamales",-14}, {1,'N',13,"plato_carnitas",17},
  {2,'H',10,"plato_tamales",11}, {2,'M',10,"plato_mole",-16},
  {2,'E',5,"frutero",18}, {2,'P',5,"frutero",-27},
  {3,'G',6,"plato_tacos_dorados",-12}, {3,'N',6,"plato_tamales",16},
  {2,'J',10.5f,"frutero",43,.5f} // Centro de J–K / 10–11: X=0, Z=0.5.
 };
 struct HuellaPlato {glm::vec2 lo,hi; int piso;};
 std::vector<HuellaPlato> platosPuestos;
 std::map<std::string,Lote*> platosLotes;
 for(const std::string clave:{"plato_tamales","plato_mole","plato_carnitas","plato_tacos_dorados","frutero"})
  platosLotes[clave]=nodoLote(clave,altarAprobado,"Platos compartidos · "+clave)->lote;
 // Buscar dentro de la celda indicada (hasta 10 cm desde su centro), primero
 // a 28 cm de ancho, reduciendo solo el plato cuando el espacio lo exige.
 std::vector<glm::vec2> ajustes;
 for(int z=-4;z<=4;++z)for(int x=-4;x<=4;++x)ajustes.push_back({x*.025f,z*.025f});
 std::stable_sort(ajustes.begin(),ajustes.end(),[](auto a,auto b){return glm::dot(a,a)<glm::dot(b,b);});
 constexpr float margenPlato=.015f;
 for(const auto& pedido:pedidos) {
  const auto& piso=pisos[pedido.piso-1];
  const Modelo& m=*modelo(pedido.clave);
  const glm::vec3 medidas=m.max-m.min;
  const float angulo=glm::radians(pedido.giro);
  const glm::vec2 huellaRotada(std::abs(std::cos(angulo))*medidas.x+std::abs(std::sin(angulo))*medidas.z,
      std::abs(std::sin(angulo))*medidas.x+std::abs(std::cos(angulo))*medidas.z);
  const glm::mat4 rotacion=glm::rotate(glm::mat4(1),angulo,glm::vec3(0,1,0));
  const glm::vec2 referencia(-2.5f+(pedido.columna-'A'+.5f+pedido.entreColumnas)*.25f,-2.f+(pedido.fila-.5f)*.25f);
  auto cabe=[&](glm::vec2 centro,float escalaPlato) {
   const glm::vec2 mitad=huellaRotada*escalaPlato*.5f;
   const glm::vec2 lo=centro-mitad,hi=centro+mitad;
   if(lo.x< -piso.mitadAncho+margenPlato||hi.x>piso.mitadAncho-margenPlato||
      lo.y< -1.425f+margenPlato||hi.y>piso.frente-margenPlato)return false;
   if(pedido.piso<3 && hi.x> -pisos[pedido.piso].mitadAncho-margenPlato &&
      lo.x<pisos[pedido.piso].mitadAncho+margenPlato && lo.y<piso.fondoFranja+margenPlato)return false;
   for(const auto& v:huellasVelas) if(v.piso==pedido.piso) {
    const glm::vec2 cercano=glm::clamp(v.centro,lo,hi);
    if(glm::distance(cercano,v.centro)<v.radio+margenPlato)return false;
   }
   for(const auto& otro:platosPuestos) if(otro.piso==pedido.piso &&
       hi.x+margenPlato>otro.lo.x&&lo.x-margenPlato<otro.hi.x&&
       hi.y+margenPlato>otro.lo.y&&lo.y-margenPlato<otro.hi.y)return false;
   for(int jNivel=0;jNivel<2;++jNivel) {
    const auto& jp=pisos[jNivel];
    if(piso.y>=jp.y+tam.y || piso.y+(m.max.y-m.baseY)*escalaPlato<=jp.y)continue;
    for(int lado:{-1,1}) {
     const glm::vec2 j(lado*(jp.mitadAncho-margen-tam.x*.5f),jp.frente-margen-tam.z*.5f);
     if(hi.x+margenPlato>j.x-tam.x*.5f && lo.x-margenPlato<j.x+tam.x*.5f &&
        hi.y+margenPlato>j.y-tam.z*.5f && lo.y-margenPlato<j.y+tam.z*.5f)return false;
    }
   }
   return true;
  };
  bool colocado=false;float escalaPlato=0;glm::vec2 centro;
  for(int paso=0;paso<=32 && !colocado;++paso) {
   escalaPlato=(.28f-paso*.005f)/medidas.x;
   for(const auto& ajuste:ajustes)if(cabe(referencia+ajuste,escalaPlato)) {
    centro=referencia+ajuste;colocado=true;break;
   }
  }
  if(!colocado)throw std::runtime_error("No cabe un plato en la celda solicitada sin mover veladoras");
  const glm::vec2 mitad=huellaRotada*escalaPlato*.5f;
  platosPuestos.push_back({centro-mitad,centro+mitad,pedido.piso});
  const glm::vec3 offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f);
  const glm::vec3 pos(centro.x,piso.y,centro.y);
  platosLotes[pedido.clave]->agregar(glm::translate(glm::mat4(1),pos)*rotacion*
      glm::scale(glm::mat4(1),glm::vec3(escalaPlato))*glm::translate(glm::mat4(1),-offset));
  info(pedido.clave)->usos++;
  registro::info("RECIPIENTE: P%d %c%.1f (+%.1f columna), %s, XZ=(%.4f,%.4f), ancho local=%.1f cm, apoyo Y=%.3f, giro Y=%.1f",
    pedido.piso,pedido.columna,pedido.fila,pedido.entreColumnas,pedido.clave,centro.x,centro.y,100*medidas.x*escalaPlato,piso.y,pedido.giro);
 }
 registro::info("COMIDA: %zu platos comprobados; 54 veladoras aprobadas sin cambios",platosPuestos.size());


 // Media luna opuesta a la U: arco posterior abierto hacia +Z (el frente).
 // Una instancia de cada modelo. Centro retrasado; laterales adelantados e inclinados en planta.
 struct Retrato {const char* clave; float x,z,altura,giro;};
 const Retrato retratos[] = {
  {"portarretrato_1",-.42f,-.99f,.36f,14},
  {"portarretrato_3",0,-1.20f,.44f,0},
  {"portarretrato_2",.42f,-.99f,.36f,-14}
 };
 std::vector<HuellaPlato> huellasRetratos;
 constexpr float margenRetrato=.02f;
 for(const auto& r:retratos) {
  const Modelo& m=*modelo(r.clave);
  const glm::vec3 dimensiones=m.max-m.min;
  const float s=r.altura/(m.max.y-m.baseY),a=glm::radians(r.giro);
  const glm::vec2 mitad(
      (std::abs(std::cos(a))*dimensiones.x+std::abs(std::sin(a))*dimensiones.z)*s*.5f,
      (std::abs(std::sin(a))*dimensiones.x+std::abs(std::cos(a))*dimensiones.z)*s*.5f);
  const glm::vec2 centro(r.x,r.z),lo=centro-mitad,hi=centro+mitad;
  if(lo.x< -pisos[2].mitadAncho+margenRetrato||hi.x>pisos[2].mitadAncho-margenRetrato||
     lo.y< -1.425f+margenRetrato||hi.y>pisos[2].frente-margenRetrato)
   throw std::runtime_error("Portarretrato fuera del piso 3");
  for(const auto& v:huellasVelas)if(v.piso==3 &&
      glm::distance(v.centro,glm::clamp(v.centro,lo,hi))<v.radio+margenRetrato)
   throw std::runtime_error("Portarretrato invade una veladora aprobada");
  auto interseca=[&](const HuellaPlato& h) {
   return hi.x+margenRetrato>h.lo.x && lo.x-margenRetrato<h.hi.x &&
          hi.y+margenRetrato>h.lo.y && lo.y-margenRetrato<h.hi.y;
  };
  for(const auto& h:platosPuestos)if(h.piso==3 && interseca(h))
   throw std::runtime_error("Portarretrato invade un plato");
  for(const auto& h:huellasRetratos)if(interseca(h))
   throw std::runtime_error("Portarretratos superpuestos");
  for(int nivel=0;nivel<2;++nivel) {
   const auto& jp=pisos[nivel];
   if(jp.y+tam.y<=pisos[2].y)continue;
   for(int lado:{-1,1}) {
    const glm::vec2 j(lado*(jp.mitadAncho-margen-tam.x*.5f),jp.frente-margen-tam.z*.5f);
    const glm::vec2 radio(tam.x*.5f,tam.z*.5f);
    if(interseca({j-radio,j+radio,nivel+1}))throw std::runtime_error("Portarretrato invade un ramo");
   }
  }
  huellasRetratos.push_back({lo,hi,3});
  auto* grupo=altarAprobado->agregar(std::string("Media luna · ")+r.clave);
  grupo->colocar({r.x,pisos[2].y,r.z},{0,r.giro,0},glm::vec3(s));
  const glm::vec3 offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f);
  instanciar(r.clave,grupo,r.clave,-offset);
  registro::info("RETRATO: %s, XZ=(%.3f,%.3f), altura=%.2f m, giro Y=%.1f, apoyo Y=%.3f",
      r.clave,r.x,r.z,r.altura,r.giro,pisos[2].y);
 }
 registro::info("RETRATOS: 3 modelos, una instancia de cada uno; arco posterior sin colisiones");


 // Complementos: comprobar su volumen contra todas las piezas ya colocadas.
 // Incluir las transformaciones internas glTF, los lotes y los soportes de marcos.
 struct Volumen {glm::vec3 lo,hi;};
 std::vector<Volumen> ocupados;
 auto envolver=[&](glm::vec3 lo,glm::vec3 hi,const glm::mat4& matriz) {
  Volumen b{glm::vec3(1e20f),glm::vec3(-1e20f)};
  for(int k=0;k<8;++k) {
   const auto v=glm::vec3(matriz*glm::vec4((k&1)?hi.x:lo.x,(k&2)?hi.y:lo.y,(k&4)?hi.z:lo.z,1));
   b.lo=glm::min(b.lo,v);b.hi=glm::max(b.hi,v);
  }
  return b;
 };
 altarAprobado->actualizar(glm::mat4(1));
 std::function<void(const Nodo*)> recoger=[&](const Nodo* n) {
  if(!n->visible)return;
  if(n->modelo && n->malla>=0 && n->modelo->nombre!="altar_muertos")
   for(const auto& prim:n->modelo->mallas[n->malla].primitivas)
    ocupados.push_back(envolver(prim.min,prim.max,n->mundo));
  if(n->lote)for(const auto& instancia:n->lote->transformaciones)
   for(const auto& parte:n->lote->modelo->plano)
    for(const auto& prim:n->lote->modelo->mallas[parte.malla].primitivas)
     ocupados.push_back(envolver(prim.min,prim.max,n->mundo*instancia*parte.matriz));
  for(const auto& hijo:n->hijos)recoger(hijo.get());
 };
 recoger(altarAprobado);
 struct Complemento {
  const char* clave;const char* nombre;int piso;float x,z,altura,giro;
  float ajuste; // desplazamiento máximo por eje; nunca modifica piezas anteriores.
 };
 const Complemento complementos[] = {
  {"portarretrato_1","Marco detrás del jarrón P1 izquierdo",1,-1.43f,.98f,.42f,22,.10f},
  {"portarretrato_2","Marco detrás del jarrón P1 derecho",1,1.43f,.98f,.42f,-22,.10f},
  {"portarretrato_2","Marco detrás del jarrón P2 izquierdo",2,-1.20f,.30f,.40f,22,.10f},
  {"portarretrato_1","Marco detrás del jarrón P2 derecho",2,1.20f,.30f,.40f,-22,.10f},
  {"botella_victoria","Victoria detrás del plato frontal izquierdo",1,-.85f,.89f,.272f,12,.025f},
  {"botella_corona","Corona detrás del plato frontal derecho",1,.85f,.89f,.272f,-13,.025f},
  {"botella_corona","Corona a la derecha del plato lateral izquierdo",1,-1.575f,-.38f,.272f,17,.025f},
  {"botella_victoria","Victoria a la derecha del plato lateral derecho",1,1.725f,-.38f,.272f,-9,.025f},
  {"calaverita_azucar","Calaverita normal P2 IJ10",2,-.25f,.375f,.15f,6,.10f},
  {"calaverita_azucar","Calaverita normal P2 KL10",2,.25f,.375f,.15f,-6,.10f},
  {"calaverita_azucar","Calaverita normal P3 H8",3,-.70f,-.225f,.15f,8,0},
  {"calaverita_azucar","Calaverita normal P3 M8",3,.70f,-.225f,.15f,-8,0},
  {"calaverita_azucar","Calaverita grande P3 JK8",3,0,-.125f,.20f,0,.075f},
  {"calaverita_azucar","Calaverita grande P1 J13 hacia su pareja",1,-.20f,1.125f,.20f,22,0},
  {"calaverita_azucar","Calaverita grande P1 K13 hacia su pareja",1,.25f,1.125f,.20f,-22,0},
  {"jarra_agua","Jarra P2 F11 con asa hacia fuera",2,-1.125f,.625f,.20f,180,.10f},
  {"jarra_agua","Jarra P2 O11 con asa hacia fuera",2,1.125f,.625f,.20f,0,.10f}
 };
 constexpr float separacionObjeto=.012f;
 std::map<std::string,Lote*> complementosLotes;
 for(const auto& c:complementos) {
  const Modelo& m=*modelo(c.clave);const auto& piso=pisos[c.piso-1];
  const glm::vec3 offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f);
  const glm::mat4 giro=glm::rotate(glm::mat4(1),glm::radians(c.giro),{0,1,0});
  // Todas las piezas nuevas conservan su altura prevista; resolver únicamente su posición.
  const float escalaObjeto=c.altura/(m.max.y-m.baseY);
  std::vector<glm::vec2> candidatos;
  const int pasos=int(std::round(c.ajuste/.025f));
  for(int z=-pasos;z<=pasos;++z)for(int x=-pasos;x<=pasos;++x)candidatos.push_back({x*.025f,z*.025f});
  std::stable_sort(candidatos.begin(),candidatos.end(),[](auto a,auto b){return glm::dot(a,a)<glm::dot(b,b);});
  bool puesto=false;glm::vec3 pos;Volumen elegido;
  for(const auto& delta:candidatos) {
   pos={c.x+delta.x,piso.y,c.z+delta.y};
   const glm::mat4 matriz=glm::translate(glm::mat4(1),pos)*giro*
       glm::scale(glm::mat4(1),glm::vec3(escalaObjeto))*glm::translate(glm::mat4(1),-offset);
   glm::vec3 base=m.min;base.y=m.baseY;
   const Volumen b=envolver(base,m.max,matriz);
   if(b.lo.x< -piso.mitadAncho+separacionObjeto||b.hi.x>piso.mitadAncho-separacionObjeto||
      b.lo.z< -1.425f+separacionObjeto||b.hi.z>piso.frente-separacionObjeto)continue;
   if(c.piso<3 && b.hi.x> -pisos[c.piso].mitadAncho-separacionObjeto &&
      b.lo.x<pisos[c.piso].mitadAncho+separacionObjeto && b.lo.z<piso.fondoFranja+separacionObjeto)continue;
   bool choque=false;
   for(const auto& o:ocupados) {
    if(b.hi.y<=o.lo.y+.0001f||b.lo.y>=o.hi.y-.0001f)continue;
    if(b.hi.x+separacionObjeto>o.lo.x&&b.lo.x-separacionObjeto<o.hi.x&&
       b.hi.z+separacionObjeto>o.lo.z&&b.lo.z-separacionObjeto<o.hi.z) {choque=true;break;}
   }
   if(choque)continue;
   elegido=b;puesto=true;break;
  }
  if(!puesto)throw std::runtime_error(std::string("Sin espacio para: ")+c.nombre);
  if(!complementosLotes.count(c.clave))
   complementosLotes[c.clave]=nodoLote(c.clave,altarAprobado,std::string("Complementos compartidos · ")+c.clave)->lote;
  complementosLotes[c.clave]->agregar(glm::translate(glm::mat4(1),pos)*giro*
      glm::scale(glm::mat4(1),glm::vec3(escalaObjeto))*glm::translate(glm::mat4(1),-offset));
  info(c.clave)->usos++;
  ocupados.push_back(elegido);
  registro::info("COMPLEMENTO: %s, XZ=(%.3f,%.3f), altura=%.3f, giro Y=%.1f",c.nombre,pos.x,pos.z,c.altura,c.giro);
 }
 registro::info("COMPLEMENTOS: 4 marcos ligeros, 4 cervezas, 7 calaveritas y 2 jarras; sin intersecciones");


 // NUEVO: montoncitos por tipo, añadidos después de la composición aprobada.
 struct Monton {const char* clave;const char* zona;int piso,cantidad;float x,z,giro,ancho,dx,dz;};
 const Monton montones[] = {
  {"dulce_palanqueta","DE14",1,4,-1.50f,1.375f,-12,.22f,.225f,.10f},
  {"dulce_limon_relleno","N12-13 (hueco contiguo)",1,5,.875f,1.00f,16,.20f,.25f,.20f},
  {"dulce_leche_estriado","I6",3,5,-.375f,-.525f,-18,.18f,0,0},
  {"dulce_jamoncillo","L7",3,5,.375f,-.375f,23,.20f,.10f,.10f},
  {"dulce_alegria","P4",2,4,1.375f,-1.125f,-8,.20f,.10f,.10f},
  {"dulce_tamarindo","D10",1,5,-1.625f,.375f,13,.21f,.10f,.10f},
  {"dulce_cocada","Q3",1,5,1.625f,-1.375f,-21,.20f,.10f,.10f}
 };
 std::map<std::string,std::unique_ptr<ApoyoDulce>> perfilesDulce;
 std::map<std::string,Lote*> lotesDulce;
 int dulcesTotales=0,superiores=0;
 for(const auto& g:montones) {
  const Modelo& m=*modelo(g.clave);const auto& piso=pisos[g.piso-1];
  auto& perfil=perfilesDulce[g.clave];if(!perfil)perfil=std::make_unique<ApoyoDulce>(m);
  const auto& p=*perfil;
  // Cinco piezas: cuatro abajo + una arriba. Cuatro: tres abajo + una arriba.
  std::vector<glm::vec3> piezas;
  if(g.cantidad==5) {
   for(int z:{-1,1})for(int x:{-1,1})piezas.push_back({x*p.tam.x*.515f,0,z*p.tam.y*.515f});
  } else {
   piezas={{-p.tam.x*.515f,0,-p.tam.y*.343f},{p.tam.x*.515f,0,-p.tam.y*.343f},{0,0,p.tam.y*.687f}};
  }
  float apoyo=-1e20f;int basesConHuella=0;
  for(const auto& base:piezas) {
   const float contacto=p.alturaSobre({-base.x,-base.z});
   if(contacto>-1e10f){apoyo=std::max(apoyo,contacto);++basesConHuella;}
  }
  if(basesConHuella<2 || !std::isfinite(apoyo) || apoyo<=0)
   throw std::runtime_error(std::string("Montón sin apoyo geométrico: ")+g.clave);
  // El contacto utiliza los triángulos de la geometría actual, no su altura total.
  piezas.push_back({0,apoyo+.0001f,0});
  Volumen local{glm::vec3(1e20f),glm::vec3(-1e20f)};
  glm::vec3 loModelo=m.min-p.offset,hiModelo=m.max-p.offset;loModelo.y=0;
  for(const auto& pieza:piezas){local.lo=glm::min(local.lo,loModelo+pieza);local.hi=glm::max(local.hi,hiModelo+pieza);}
  const glm::vec3 centroLocal((local.lo.x+local.hi.x)*.5f,0,(local.lo.z+local.hi.z)*.5f);
  const glm::mat4 giro=glm::rotate(glm::mat4(1),glm::radians(g.giro),{0,1,0});
  std::vector<glm::vec2> opciones;
  for(int z=-int(std::round(g.dz/.025f));z<=int(std::round(g.dz/.025f));++z)
   for(int x=-int(std::round(g.dx/.025f));x<=int(std::round(g.dx/.025f));++x)opciones.push_back({x*.025f,z*.025f});
  std::stable_sort(opciones.begin(),opciones.end(),[](auto a,auto b){return glm::dot(a,a)<glm::dot(b,b);});
  bool listo=false;float escalaD=0;glm::vec3 posicion;glm::mat4 matrizGrupo(1);Volumen cajaGrupo;
  // Máximo 22 intentos de escala; la composición aprobada no participa en la búsqueda.
  for(int intento=0;intento<=22 && !listo;++intento) {
   const float ancho=g.ancho-intento*.005f;if(ancho<.10f)break;
   // Las piezas estriadas son largas sobre Z: limitar su dimensión mayor,
   // no su ancho X, evita agrandarlas artificialmente hasta superar los platos.
   const bool estriado=std::string(g.clave)=="dulce_leche_estriado";
   const float referenciaTam=estriado?std::max(local.hi.x-local.lo.x,local.hi.z-local.lo.z):(local.hi.x-local.lo.x);
   escalaD=ancho/referenciaTam;
   for(const auto& opcion:opciones) {
    posicion={g.x+opcion.x,piso.y,g.z+opcion.y};
    const glm::mat4 matriz=glm::translate(glm::mat4(1),posicion)*giro*
        glm::scale(glm::mat4(1),glm::vec3(escalaD))*glm::translate(glm::mat4(1),-centroLocal);
    const Volumen b=envolver(local.lo,local.hi,matriz);
    constexpr float margenD=.012f;
    if(b.lo.x< -piso.mitadAncho+margenD||b.hi.x>piso.mitadAncho-margenD||
       b.lo.z< -1.425f+margenD||b.hi.z>piso.frente-margenD)continue;
    if(g.piso<3 && b.hi.x> -pisos[g.piso].mitadAncho-margenD &&
       b.lo.x<pisos[g.piso].mitadAncho+margenD && b.lo.z<piso.fondoFranja+margenD)continue;
    bool choque=false;
    for(const auto& o:ocupados) {
     if(b.hi.y<=o.lo.y+.0001f||b.lo.y>=o.hi.y-.0001f)continue;
     if(b.hi.x+margenD>o.lo.x&&b.lo.x-margenD<o.hi.x&&
        b.hi.z+margenD>o.lo.z&&b.lo.z-margenD<o.hi.z){choque=true;break;}
    }
    if(choque)continue;
    matrizGrupo=matriz;cajaGrupo=b;listo=true;break;
   }
  }
  if(!listo)throw std::runtime_error(std::string("No cabe el montoncito sin mover la composición: ")+g.zona);
  if(!lotesDulce.count(g.clave))lotesDulce[g.clave]=nodoLote(g.clave,altarAprobado,std::string("Dulces compartidos · ")+g.clave)->lote;
  for(const auto& pieza:piezas) {
   lotesDulce[g.clave]->agregar(matrizGrupo*glm::translate(glm::mat4(1),pieza-p.offset));
   info(g.clave)->usos++;dulcesTotales++;
  }
  ocupados.push_back(cajaGrupo);superiores++;
  registro::info("MONTON: P%d %s, %s, %d piezas (%d+1), XZ=(%.3f,%.3f), ancho local=%.1f cm, giro=%.1f, altura=%.1f cm, apoyos proyectados=%d",
      g.piso,g.zona,g.clave,g.cantidad,g.cantidad-1,posicion.x,posicion.z,
      100*(local.hi.x-local.lo.x)*escalaD,g.giro,100*(cajaGrupo.hi.y-cajaGrupo.lo.y),basesConHuella);
 }
 registro::info("DULCES: 7 montoncitos, %d piezas, %d piezas superiores apoyadas sobre perfiles de triángulos; composición aprobada intacta",dulcesTotales,superiores);


 // Vajilla adicional: solo se ajustan estas ocho piezas, tras validar los dulces.
 const Complemento vajillaNueva[] = {
  {"jarro_cafe","Café P1 entre carnitas derechas y veladoras",1,1.08f,1.28f,.10f,-18,.15f},
  {"jarro_cafe","Café P2 entre frutero izquierdo y veladoras",2,-1.30f,-.50f,.11f,165,.075f},
  {"olla_barro","Olla P2 entre frutero derecho y veladoras",2,1.275f,-.50f,.1725f,12,0},
  {"olla_barro","Olla P1 delante del marco junto al jarrón derecho",1,1.37f,1.25f,.1725f,-12,0},
  {"vaso_agua","Agua P1 entre cerveza frontal derecha y veladoras",1,.99f,.88f,.11f,0,.05f},
  {"vaso_agua","Agua P2 a la izquierda del frutero izquierdo",2,-1.40f,-.65f,.11f,0,.075f},
  {"vaso_agua","Agua P3 entre calaverita izquierda y central",3,-.35f,-.175f,.11f,0,.05f},
  {"vaso_agua","Agua P3 entre calaverita central y derecha",3,.35f,-.175f,.11f,0,.05f}
 };
 for(const auto& c:vajillaNueva) {
  const Modelo& m=*modelo(c.clave);const auto& piso=pisos[c.piso-1];
  const glm::vec3 offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f);
  const glm::mat4 giro=glm::rotate(glm::mat4(1),glm::radians(c.giro),{0,1,0});
  // Todas las piezas nuevas conservan su altura prevista; resolver únicamente su posición.
  const float escalaObjeto=c.altura/(m.max.y-m.baseY);
  std::vector<glm::vec2> candidatos;
  const int pasos=int(std::round(c.ajuste/.025f));
  for(int z=-pasos;z<=pasos;++z)for(int x=-pasos;x<=pasos;++x)candidatos.push_back({x*.025f,z*.025f});
  std::stable_sort(candidatos.begin(),candidatos.end(),[](auto a,auto b){return glm::dot(a,a)<glm::dot(b,b);});
  bool puesto=false;glm::vec3 pos;Volumen elegido;
  for(const auto& delta:candidatos) {
   pos={c.x+delta.x,piso.y,c.z+delta.y};
   const glm::mat4 matriz=glm::translate(glm::mat4(1),pos)*giro*
       glm::scale(glm::mat4(1),glm::vec3(escalaObjeto))*glm::translate(glm::mat4(1),-offset);
   glm::vec3 base=m.min;base.y=m.baseY;
   const Volumen b=envolver(base,m.max,matriz);
   if(b.lo.x< -piso.mitadAncho+separacionObjeto||b.hi.x>piso.mitadAncho-separacionObjeto||
      b.lo.z< -1.425f+separacionObjeto||b.hi.z>piso.frente-separacionObjeto)continue;
   if(c.piso<3 && b.hi.x> -pisos[c.piso].mitadAncho-separacionObjeto &&
      b.lo.x<pisos[c.piso].mitadAncho+separacionObjeto && b.lo.z<piso.fondoFranja+separacionObjeto)continue;
   bool choque=false;
   for(const auto& o:ocupados) {
    if(b.hi.y<=o.lo.y+.0001f||b.lo.y>=o.hi.y-.0001f)continue;
    if(b.hi.x+separacionObjeto>o.lo.x&&b.lo.x-separacionObjeto<o.hi.x&&
       b.hi.z+separacionObjeto>o.lo.z&&b.lo.z-separacionObjeto<o.hi.z) {choque=true;break;}
   }
   if(choque)continue;
   elegido=b;puesto=true;break;
  }
  if(!puesto)throw std::runtime_error(std::string("Sin espacio para: ")+c.nombre);
  if(!complementosLotes.count(c.clave))
   complementosLotes[c.clave]=nodoLote(c.clave,altarAprobado,std::string("Complementos compartidos · ")+c.clave)->lote;
  complementosLotes[c.clave]->agregar(glm::translate(glm::mat4(1),pos)*giro*
      glm::scale(glm::mat4(1),glm::vec3(escalaObjeto))*glm::translate(glm::mat4(1),-offset));
  info(c.clave)->usos++;
  ocupados.push_back(elegido);
  registro::info("VAJILLA: %s, XZ=(%.3f,%.3f), altura=%.3f, giro Y=%.1f",c.nombre,pos.x,pos.z,c.altura,c.giro);
 }

 registro::info("VAJILLA: 2 jarros, 2 ollas y 4 vasos; composición aprobada intacta");


 // Reservar el centro del piso superior antes de distribuir el pan.
 auto libreEnPiso=[&](const Volumen& b,int nivel,float margenLibre) {
  const auto& piso=pisos[nivel-1];
  if(b.lo.x< -piso.mitadAncho+margenLibre || b.hi.x>piso.mitadAncho-margenLibre ||
     b.lo.z< -1.425f+margenLibre || b.hi.z>piso.frente-margenLibre)return false;
  if(nivel<3 && b.hi.x> -pisos[nivel].mitadAncho-margenLibre &&
     b.lo.x<pisos[nivel].mitadAncho+margenLibre && b.lo.z<piso.fondoFranja+margenLibre)return false;
  for(const auto& o:ocupados) {
   if(b.hi.y<=o.lo.y+.0001f || b.lo.y>=o.hi.y-.0001f)continue;
   if(b.hi.x+margenLibre>o.lo.x && b.lo.x-margenLibre<o.hi.x &&
      b.hi.z+margenLibre>o.lo.z && b.lo.z-margenLibre<o.hi.z)return false;
  }
  return true;
 };
 const Modelo& copal=*modelo("sahumerio");
 const glm::vec3 offsetCopal((copal.min.x+copal.max.x)*.5f,copal.baseY,(copal.min.z+copal.max.z)*.5f);
 const glm::vec3 centroCopal(0,pisos[2].y,(-1.425f+pisos[2].frente)*.5f);
 const glm::mat4 matrizCopal=glm::translate(glm::mat4(1),centroCopal-offsetCopal);
 auto baseCopal=copal.min;baseCopal.y=copal.baseY;
 const Volumen cajaCopal=envolver(baseCopal,copal.max,matrizCopal);
 if(!libreEnPiso(cajaCopal,3,.015f))throw std::runtime_error("El sahumerio no cabe en el centro del piso 3");
 instanciar("sahumerio",altarAprobado,"Sahumerio central del piso 3",centroCopal-offsetCopal);
 ocupados.push_back(cajaCopal);
 registro::info("SAHUMERIO: centro P3 XZ=(%.4f,%.4f), altura=%.3f",centroCopal.x,centroCopal.z,copal.max.y-copal.baseY);


 // Pan: tamaño único aprobado de 12 cm. Prioridad 3+1; huecos restantes 2+1.
 const Modelo& pan=*modelo("pan_de_muerto");
 const ApoyoDulce perfilPan(pan);
 constexpr float anchoPan=.12f,margenPan=.015f;
 const float escalaPan=anchoPan/(pan.max.x-pan.min.x);
 float radioPan=0;
 for(const auto& parte:pan.plano)for(const auto& vertice:pan.mallas[parte.malla].superficie) {
  const glm::vec3 v=glm::vec3(parte.matriz*glm::vec4(vertice,1))-perfilPan.offset;
  radioPan=std::max(radioPan,glm::length(glm::vec2(v.x,v.z)));
 }
 struct FormaPan {std::vector<glm::vec3> piezas;Volumen caja;glm::vec3 centro;};
 auto formarPan=[&](int cantidad) {
  FormaPan f;
  const float distancia=2*radioPan+.003f/escalaPan;
  if(cantidad==4) {
   const float h=distancia*std::sqrt(3.f)*.5f;
   f.piezas={{-distancia*.5f,0,-h/3},{distancia*.5f,0,-h/3},{0,0,2*h/3}};
  } else f.piezas={{-distancia*.5f,0,0},{distancia*.5f,0,0}};
  // Buscar un contacto equilibrado sobre todas las bases, sin alterar su tamaño.
  float mejor=1e20f;glm::vec3 superior;float holguraMax=0;
  for(int z=-2;z<=2;++z)for(int x=-2;x<=2;++x) {
   const glm::vec2 desplazamiento(x*.005f,z*.005f);
   float bajo=1e20f,alto=-1e20f;bool valido=true;
   for(const auto& base:f.piezas) {
    const float apoyo=perfilPan.alturaSobre(desplazamiento-glm::vec2(base.x,base.z));
    if(apoyo< -1e10f){valido=false;break;}
    bajo=std::min(bajo,apoyo);alto=std::max(alto,apoyo);
   }
   if(!valido)continue;
   const float error=alto-bajo+.001f*glm::length(desplazamiento);
   if(error<mejor){mejor=error;superior={desplazamiento.x,alto+.0001f,desplazamiento.y};holguraMax=(alto-bajo)*escalaPan;}
  }
  if(mejor>1e10f)throw std::runtime_error("Pan superior sin apoyo sobre las bases");
  f.piezas.push_back(superior);
  f.caja={glm::vec3(1e20f),glm::vec3(-1e20f)};
  auto lo=pan.min-perfilPan.offset;lo.y=0;
  const auto hi=pan.max-perfilPan.offset;
  for(const auto& pieza:f.piezas){f.caja.lo=glm::min(f.caja.lo,lo+pieza);f.caja.hi=glm::max(f.caja.hi,hi+pieza);}
  f.centro={(f.caja.lo.x+f.caja.hi.x)*.5f,0,(f.caja.lo.z+f.caja.hi.z)*.5f};
  registro::info("APOYO PAN %d+1: holgura máxima entre bases=%.3f mm",cantidad-1,holguraMax*1000);
  return f;
 };
 const FormaPan forma4=formarPan(4),forma3=formarPan(3);
 // Diez grupos fijos: siete 3+1 y tres 2+1. M12 sustituye N12–13 por autorización del usuario.
 struct SitioPan {int piso,cantidad;float x,z,giro;};
 const SitioPan sitiosPan[] = {
  {1,4,-.550f,1.100f,90}, {1,4,1.650f,-.650f,0},
  {2,4,-1.275f,-1.250f,0},
  {3,4,.375f,-.650f,0}, {3,4,-.150f,-.375f,90},
  {1,3,-1.650f,.150f,0}, {1,3,.025f,1.125f,90},
  {3,3,-.675f,-.475f,0}, {1,4,1.625f,.250f,0},
  {1,4,.550f,.950f,90}
 };
 auto* lotePan=nodoLote("pan_de_muerto",altarAprobado,"Pan de muerto compartido · grupos 3+1 y 2+1")->lote;
 std::array<int,3> gruposPorPiso{};int totalPan=0,totalGrupos4=0,totalGrupos3=0;
 for(const auto& sitio:sitiosPan) {
  const auto& forma=sitio.cantidad==4?forma4:forma3;
  // La malla texturizada tiene una envolvente distinta a la versión gris. Se
  // conserva el tamaño de 12 cm y se busca el desplazamiento libre más cercano
  // a cada posición aprobada, en pasos de 2.5 cm y hasta 35 cm.
  std::vector<glm::vec2> ajustesPan;
  for(int z=-14;z<=14;++z)for(int x=-14;x<=14;++x)ajustesPan.push_back({x*.025f,z*.025f});
  std::stable_sort(ajustesPan.begin(),ajustesPan.end(),[](auto a,auto b){return glm::dot(a,a)<glm::dot(b,b);});
  glm::mat4 matriz(1);Volumen b;glm::vec2 posicionFinal;bool colocado=false;
  auto basePan=pan.min;basePan.y=pan.baseY;
  for(const auto& ajuste:ajustesPan) {
   posicionFinal={sitio.x+ajuste.x,sitio.z+ajuste.y};
   const glm::mat4 prueba=glm::translate(glm::mat4(1),glm::vec3(posicionFinal.x,pisos[sitio.piso-1].y,posicionFinal.y))*
       glm::rotate(glm::mat4(1),glm::radians(sitio.giro),{0,1,0})*
       glm::scale(glm::mat4(1),glm::vec3(escalaPan))*glm::translate(glm::mat4(1),-forma.centro);
   bool libre=true;
   for(const auto& pieza:forma.piezas) {
    const auto caja=envolver(basePan,pan.max,prueba*glm::translate(glm::mat4(1),pieza-perfilPan.offset));
    if(!libreEnPiso(caja,sitio.piso,margenPan)){libre=false;break;}
   }
   if(libre){matriz=prueba;b=envolver(forma.caja.lo,forma.caja.hi,matriz);colocado=true;break;}
  }
  if(!colocado)throw std::runtime_error("No se encontró espacio cercano para un grupo de pan texturizado");
  for(const auto& pieza:forma.piezas) {
   lotePan->agregar(matriz*glm::translate(glm::mat4(1),pieza-perfilPan.offset));
   info("pan_de_muerto")->usos++;totalPan++;
  }
  ocupados.push_back(b);gruposPorPiso[sitio.piso-1]++;
  if(sitio.cantidad==4)totalGrupos4++;else totalGrupos3++;
  registro::info("GRUPO PAN: P%d, %d+1, XZ=(%.3f,%.3f), ajuste=(%.3f,%.3f), giro=%.0f, pan=12 cm",
      sitio.piso,sitio.cantidad-1,posicionFinal.x,posicionFinal.y,posicionFinal.x-sitio.x,posicionFinal.y-sitio.z,
      sitio.giro);
 }
 if(totalGrupos4!=7 || totalGrupos3!=3 || totalPan!=37)throw std::runtime_error("Cantidad de pan distinta de la solicitada");
 registro::info("PAN: %d grupos 3+1, %d grupos 2+1, %d panes; grupos por piso=%d/%d/%d; tamaño único y sin mover modelos previos",
     totalGrupos4,totalGrupos3,totalPan,gruposPorPiso[0],gruposPorPiso[1],gruposPorPiso[2]);

 // Papel picado pequeño, colgado del borde exterior del mantel, nunca del tablero retranqueado.
 // Todo el bloque anterior permanece aprobado e inmutable.
 constexpr float margenEsquinaPapel=.015f,separacionPapel=.012f,despeguePapel=.001f;
 constexpr float fondoAltar=-1.425f;
 std::array<Modelo*,6> papeles;
 std::array<Lote*,6> lotesPapel;
 float anchoMinimo=1e20f;
 for(int i=0;i<6;++i) {
  const std::string clave="papel_"+std::to_string(i+1);
  papeles[i]=modelo(clave);
  lotesPapel[i]=nodoLote(clave,altarAprobado,"Papel picado compartido · "+clave)->lote;
  anchoMinimo=std::min(anchoMinimo,papeles[i]->max.x-papeles[i]->min.x);
 }
 std::mt19937 azarPapel(27102026u);
 std::mt19937 azarColorPapel(27102029u);
 const std::array<glm::vec4,7> coloresPapel{{
  {0.82f,0.05f,0.34f,1}, // rosa mexicano
  {0.42f,0.08f,0.72f,1}, // morado
  {1.00f,0.28f,0.03f,1}, // naranja
  {0.03f,0.38f,0.88f,1}, // azul
  {0.08f,0.62f,0.24f,1}, // verde
  {0.96f,0.70f,0.03f,1}, // amarillo
  {0.88f,0.06f,0.08f,1}  // rojo
 }};
 int colorPapelAnterior=-1;
 auto anchoPapel=[&](int i){return papeles[i]->max.x-papeles[i]->min.x;};
 // Elegir el máximo número que cabe, reservando espacio para los pendientes.
 // En el frente, las variantes se eligen por parejas reflejadas; los costados comparten fila.
 auto filaPapel=[&](float largo,bool reflejada) {
  const int n=int(std::floor((largo-2*margenEsquinaPapel+separacionPapel)/(anchoMinimo+separacionPapel)));
  if(n<1)throw std::runtime_error("Borde sin espacio para papel pequeño");
  std::vector<int> fila(n,-1);
  float disponible=largo-2*margenEsquinaPapel-(n-1)*separacionPapel;
  int pendientes=n,anterior=-1;
  for(int i=0;i<(reflejada?(n+1)/2:n);++i) {
   const int multiplicidad=reflejada && i!=n-1-i?2:1;
   std::vector<int> opciones;
   for(int k=0;k<6;++k)
    if(multiplicidad*anchoPapel(k)+(pendientes-multiplicidad)*anchoMinimo<=disponible+.000001f)
     opciones.push_back(k);
   if(opciones.empty())throw std::runtime_error("Papel excede longitud del borde");
   if(opciones.size()>1)opciones.erase(std::remove(opciones.begin(),opciones.end(),anterior),opciones.end());
   const int elegido=opciones[azarPapel()%opciones.size()];
   fila[i]=elegido;if(reflejada)fila[n-1-i]=elegido;
   disponible-=multiplicidad*anchoPapel(elegido);pendientes-=multiplicidad;anterior=elegido;
  }
  return fila;
 };
 int totalPapeles=0;
 auto colgarFila=[&](int nivel,int lado,const std::vector<int>& fila) {
  const auto& piso=pisos[nivel];
  const float largo=lado==0?2*piso.mitadAncho:piso.frente-fondoAltar;
  float anchoTotal=0;for(int variante:fila)anchoTotal+=anchoPapel(variante);
  const float hueco=fila.size()>1?(largo-2*margenEsquinaPapel-anchoTotal)/(fila.size()-1):0;
  float cursor=fila.size()>1?margenEsquinaPapel:(largo-anchoTotal)*.5f;
  if(fila.size()>1 && hueco<separacionPapel-.00001f)throw std::runtime_error("Papeles demasiado juntos");
  for(int variante:fila) {
   const auto& m=*papeles[variante];const float ancho=anchoPapel(variante);
   const float centro=cursor+ancho*.5f;
   // El papel se curva hacia -Z local: orientarlo hacia fuera y sujetar su parte superior.
   const glm::vec3 anclaje=lado==0?glm::vec3(-piso.mitadAncho+centro,piso.y,piso.frente+despeguePapel):
       glm::vec3(lado*(piso.mitadAncho+despeguePapel),piso.y,fondoAltar+centro);
   const float giro=lado==0?180.f:(lado<0?90.f:-90.f);
   const glm::vec3 origen((m.min.x+m.max.x)*.5f,m.max.y,m.max.z);
   const glm::mat4 matriz=glm::translate(glm::mat4(1),anclaje)*
       glm::rotate(glm::mat4(1),glm::radians(giro),{0,1,0})*glm::translate(glm::mat4(1),-origen);
   const Volumen b=envolver(m.min,m.max,matriz);
   if(std::abs(b.hi.y-piso.y)>.00001f || b.lo.y<piso.y-.45f)
    throw std::runtime_error("Altura incorrecta del papel colgante");
   if((lado==0 && b.lo.z<piso.frente+.0005f) ||
      (lado==1 && b.lo.x<piso.mitadAncho+.0005f) ||
      (lado==-1 && b.hi.x> -piso.mitadAncho-.0005f))
    throw std::runtime_error("Papel dentro del borde exterior");
   for(const auto& o:ocupados)
    if(b.hi.x>o.lo.x+.0001f && b.lo.x<o.hi.x-.0001f &&
       b.hi.y>o.lo.y+.0001f && b.lo.y<o.hi.y-.0001f &&
       b.hi.z>o.lo.z+.0001f && b.lo.z<o.hi.z-.0001f)
     throw std::runtime_error("Papel picado invade una pieza existente");
   int color=int(azarColorPapel()%coloresPapel.size());
   if(color==colorPapelAnterior)color=(color+1+int(azarColorPapel()%(coloresPapel.size()-1)))%coloresPapel.size();
   colorPapelAnterior=color;
   lotesPapel[variante]->agregar(matriz,coloresPapel[static_cast<size_t>(color)]);
   info("papel_"+std::to_string(variante+1))->usos++;totalPapeles++;
   ocupados.push_back(b);
   registro::info("PAPEL: P%d, lado=%d, variante=%d, anclaje=(%.4f,%.4f,%.4f), ancho=%.3f",
       nivel+1,lado,variante+1,anclaje.x,anclaje.y,anclaje.z,ancho);
   cursor+=ancho+hueco;
  }
  registro::info("FILA PAPEL: P%d, lado=%d, cantidad=%zu, separación=%.3f m",nivel+1,lado,fila.size(),hueco);
 };
 for(int nivel=0;nivel<3;++nivel) {
  const auto frontal=filaPapel(2*pisos[nivel].mitadAncho,true);
  const auto lateral=filaPapel(pisos[nivel].frente-fondoAltar,false);
  colgarFila(nivel,0,frontal);colgarFila(nivel,-1,lateral);colgarFila(nivel,1,lateral);
 }
 registro::info("PAPEL: %d pequeños; frente y costados simétricos, variantes fijas, escala original",totalPapeles);

 // Espolvoreado fijo e irregular: sólo se adapta a los huecos de la composición aprobada.
 std::mt19937 azarFlores(27102027u);
 auto aleatorioFlor=[&](){return float(azarFlores()>>8)/16777216.f;};
 auto entreFlores=[&](float a,float b){return a+(b-a)*aleatorioFlor();};
 const Modelo& cabeza=*modelo("cabeza_cempasuchil");
 const Modelo& petalo=*modelo("petalo_cempasuchil");
 auto* loteCabezas=nodoLote("cabeza_cempasuchil",altarAprobado,"Cabezas de cempasúchil sueltas")->lote;
 auto* lotePetalos=nodoLote("petalo_cempasuchil",altarAprobado,"Pétalos esparcidos sobre los tres pisos")->lote;
 auto apoyarFlor=[&](const Modelo& m,float x,float z,float y,float giro,float escalaObjeto) {
  const glm::vec3 offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f);
  return glm::translate(glm::mat4(1),glm::vec3(x,y+.0002f,z))*
      glm::rotate(glm::mat4(1),giro,{0,1,0})*glm::scale(glm::mat4(1),glm::vec3(escalaObjeto))*
      glm::translate(glm::mat4(1),-offset);
 };
 auto cajaFlor=[&](const Modelo& m,const glm::mat4& matriz) {
  auto lo=m.min;lo.y=m.baseY;return envolver(lo,m.max,matriz);
 };
 const int cabezasDeseadas[]={4,3,3};
 int totalCabezas=0,totalPetalos=0;
 for(int nivel=1;nivel<=3;++nivel) {
  const auto& piso=pisos[nivel-1];
  std::vector<glm::vec2> centrosCabezas;
  for(int intento=0;intento<5000 && int(centrosCabezas.size())<cabezasDeseadas[nivel-1];++intento) {
   const float x=entreFlores(-piso.mitadAncho,piso.mitadAncho),z=entreFlores(fondoAltar,piso.frente);
   bool cerca=false;for(const auto& c:centrosCabezas)if(glm::length(c-glm::vec2(x,z))<.38f)cerca=true;
   if(cerca)continue;
   const auto matriz=apoyarFlor(cabeza,x,z,piso.y,entreFlores(0,6.2831853f),entreFlores(.85f,1.12f));
   const auto b=cajaFlor(cabeza,matriz);
   if(!libreEnPiso(b,nivel,.035f))continue;
   loteCabezas->agregar(matriz);info("cabeza_cempasuchil")->usos++;
   ocupados.push_back(b);centrosCabezas.push_back({x,z});totalCabezas++;
   registro::info("FLOR SUELTA: P%d XZ=(%.4f,%.4f), cabeza sin tallo",nivel,x,z);
  }
  // Medir área libre; esta retícula sólo mide, nunca determina posiciones de pétalos.
  float areaLibre=0;
  constexpr float pasoArea=.025f;
  for(float x=-piso.mitadAncho+pasoArea*.5f;x<piso.mitadAncho;x+=pasoArea)
   for(float z=fondoAltar+pasoArea*.5f;z<piso.frente;z+=pasoArea) {
    const Volumen prueba{{x-.012f,piso.y+.0002f,z-.012f},{x+.012f,piso.y+.008f,z+.012f}};
    if(libreEnPiso(prueba,nivel,.003f))areaLibre+=pasoArea*pasoArea;
   }
  // Aproximadamente 190 pétalos/m² libre: abundantes puntos sueltos, no una alfombra.
  const int objetivo=int(std::round(areaLibre*190.f));
  std::vector<glm::vec2> manchas;
  for(int i=0;i<9;++i)manchas.push_back({entreFlores(-piso.mitadAncho,piso.mitadAncho),entreFlores(fondoAltar,piso.frente)});
  struct PetaloPuesto {glm::vec2 centro;Volumen caja;int capa;};
  std::vector<PetaloPuesto> puestos;
  int solapados=0;
  for(int intento=0;intento<objetivo*300 && int(puestos.size())<objetivo;++intento) {
   float x=entreFlores(-piso.mitadAncho,piso.mitadAncho),z=entreFlores(fondoAltar,piso.frente);
   if(!puestos.empty() && aleatorioFlor()<.12f) {
    const auto& vecino=puestos[azarFlores()%puestos.size()];
    const float angulo=entreFlores(0,6.2831853f),radio=entreFlores(.014f,.026f);
    x=vecino.centro.x+std::cos(angulo)*radio;z=vecino.centro.y+std::sin(angulo)*radio;
   } else {
    float densidad=.38f;
    for(const auto& c:manchas) {const auto delta=c-glm::vec2(x,z);densidad+=.50f*std::exp(-glm::dot(delta,delta)/.035f);}
    if(aleatorioFlor()>std::min(.95f,densidad))continue;
   }
   auto matriz=apoyarFlor(petalo,x,z,piso.y,entreFlores(0,6.2831853f),entreFlores(.85f,1.2f));
   auto b=cajaFlor(petalo,matriz);
   if(!libreEnPiso(b,nivel,.003f))continue;
   float apoyo=piso.y+.0002f;int capa=0;
   for(const auto& otro:puestos)
    if(b.hi.x>otro.caja.lo.x && b.lo.x<otro.caja.hi.x && b.hi.z>otro.caja.lo.z && b.lo.z<otro.caja.hi.z) {
     apoyo=std::max(apoyo,otro.caja.hi.y+.0002f);capa=std::max(capa,otro.capa+1);
    }
   if(capa>1)continue; // Sólo solapamientos de dos piezas, sin montículos.
   const float subir=apoyo-(piso.y+.0002f);
   matriz=glm::translate(glm::mat4(1),glm::vec3(0,subir,0))*matriz;
   b=cajaFlor(petalo,matriz);
   if(!libreEnPiso(b,nivel,.003f))continue;
   lotePetalos->agregar(matriz);info("petalo_cempasuchil")->usos++;
   puestos.push_back({{x,z},b,capa});totalPetalos++;if(capa)solapados++;
  }
  if(int(puestos.size())!=objetivo)throw std::runtime_error("No se pudo completar el espolvoreado de pétalos");
  for(const auto& p:puestos)ocupados.push_back(p.caja);
  registro::info("ESPOLVOREADO: P%d, área libre=%.3f m2, pétalos=%zu, solapados=%d, cabezas=%zu",
      nivel,areaLibre,puestos.size(),solapados,centrosCabezas.size());
 }
 registro::info("CEMPASUCHIL: %d pétalos y %d cabezas; distribución fija, sin mover las 217 piezas aprobadas",totalPetalos,totalCabezas);

 // Ampliación solicitada: conservar todos los pétalos/cabezas previos y añadir al final.
 const int petalosPrevios=totalPetalos,cabezasPrevias=totalCabezas;
 struct PetaloExtra {Volumen caja;int piso;};
 std::vector<PetaloExtra> petalosExistentes;
 for(const auto& m:lotePetalos->transformaciones) {
  const auto b=cajaFlor(petalo,m);int piso=1;
  if(b.lo.y>1.3f)piso=3;else if(b.lo.y>.85f)piso=2;
  petalosExistentes.push_back({b,piso});
 }
 // Recuperar los volúmenes de todas las piezas excepto pétalos: las flores nuevas pueden apoyarse sobre éstos.
 ocupados.clear();altarAprobado->actualizar(glm::mat4(1));
 std::function<void(const Nodo*)> recogerSinPetalos=[&](const Nodo* n) {
  if(!n->visible)return;
  if(n->modelo && n->malla>=0 && n->modelo->nombre!="altar_muertos")
   for(const auto& prim:n->modelo->mallas[n->malla].primitivas)
    ocupados.push_back(envolver(prim.min,prim.max,n->mundo));
  if(n->lote && n->lote!=lotePetalos)
   for(const auto& instancia:n->lote->transformaciones)
    for(const auto& parte:n->lote->modelo->plano)
     for(const auto& prim:n->lote->modelo->mallas[parte.malla].primitivas)
      ocupados.push_back(envolver(prim.min,prim.max,n->mundo*instancia*parte.matriz));
  for(const auto& hijo:n->hijos)recogerSinPetalos(hijo.get());
 };
 recogerSinPetalos(altarAprobado);
 auto seCruzanXZ=[](const Volumen& a,const Volumen& b) {
  return a.hi.x>b.lo.x && a.lo.x<b.hi.x && a.hi.z>b.lo.z && a.lo.z<b.hi.z;
 };
 azarFlores.seed(27102028u);
 const int floresAdicionales[]={14,11,15},petalosAdicionales[]={588,466,790};
 for(int nivel=1;nivel<=3;++nivel) {
  const auto& piso=pisos[nivel-1];int floresAgregadas=0,petalosAgregados=0;
  for(int intento=0;intento<30000 && floresAgregadas<floresAdicionales[nivel-1];++intento) {
   const float x=entreFlores(-piso.mitadAncho,piso.mitadAncho),z=entreFlores(fondoAltar,piso.frente);
   auto m=apoyarFlor(cabeza,x,z,piso.y,entreFlores(0,6.2831853f),entreFlores(.85f,1.12f));
   auto b=cajaFlor(cabeza,m);
   if(!libreEnPiso(b,nivel,.015f))continue;
   float altura=piso.y+.0002f;
   for(const auto& p:petalosExistentes)if(p.piso==nivel && seCruzanXZ(b,p.caja))altura=std::max(altura,p.caja.hi.y+.0002f);
   m=glm::translate(glm::mat4(1),glm::vec3(0,altura-piso.y-.0002f,0))*m;b=cajaFlor(cabeza,m);
   if(!libreEnPiso(b,nivel,.015f))continue;
   loteCabezas->agregar(m);info("cabeza_cempasuchil")->usos++;ocupados.push_back(b);
   floresAgregadas++;totalCabezas++;
  }
  std::vector<glm::vec2> manchasExtra;
  for(int i=0;i<9;++i)manchasExtra.push_back({entreFlores(-piso.mitadAncho,piso.mitadAncho),entreFlores(fondoAltar,piso.frente)});
  for(int intento=0;intento<500000 && petalosAgregados<petalosAdicionales[nivel-1];++intento) {
   const float x=entreFlores(-piso.mitadAncho,piso.mitadAncho),z=entreFlores(fondoAltar,piso.frente);
   float densidad=.45f;
   for(const auto& c:manchasExtra){auto d=c-glm::vec2(x,z);densidad+=.4f*std::exp(-glm::dot(d,d)/.035f);}
   if(aleatorioFlor()>std::min(.95f,densidad))continue;
   auto m=apoyarFlor(petalo,x,z,piso.y,entreFlores(0,6.2831853f),entreFlores(.85f,1.2f));
   auto b=cajaFlor(petalo,m);
   if(!libreEnPiso(b,nivel,.003f))continue;
   float altura=piso.y+.0002f;
   for(const auto& p:petalosExistentes)if(p.piso==nivel && seCruzanXZ(b,p.caja))altura=std::max(altura,p.caja.hi.y+.0002f);
   if(altura-piso.y>.018f)continue; // Solapamientos bajos, nunca pilas altas.
   m=glm::translate(glm::mat4(1),glm::vec3(0,altura-piso.y-.0002f,0))*m;b=cajaFlor(petalo,m);
   if(!libreEnPiso(b,nivel,.003f))continue;
   lotePetalos->agregar(m);info("petalo_cempasuchil")->usos++;
   petalosExistentes.push_back({b,nivel});petalosAgregados++;totalPetalos++;
  }
  if(floresAgregadas!=floresAdicionales[nivel-1] || petalosAgregados!=petalosAdicionales[nivel-1])
   throw std::runtime_error("Falta espacio para completar la ampliación floral");
  registro::info("AMPLIACION FLORAL: P%d, +%d cabezas, +%d pétalos",nivel,floresAgregadas,petalosAgregados);
 }
 const int cabezasEsperadas=cabezasPrevias+floresAdicionales[0]+floresAdicionales[1]+floresAdicionales[2];
 const int petalosEsperados=petalosPrevios+petalosAdicionales[0]+petalosAdicionales[1]+petalosAdicionales[2];
 if(totalCabezas!=cabezasEsperadas || totalPetalos!=petalosEsperados)
  throw std::runtime_error("Recuento floral incorrecto");
 registro::info("CEMPASUCHIL FINAL: %d pétalos y %d cabezas; %d colocaciones anteriores intactas",
     totalPetalos,totalCabezas,petalosPrevios+cabezasPrevias);

 // Cubiertas colocadas al final para que no intervengan en las comprobaciones de
 // espacio de las ofrendas y flores. El leve desplazamiento evita parpadeo Z.
 constexpr std::array<glm::vec4,3> coloresPiso = {{
     {0.32f, 0.07f, 0.48f, 1.0f},
     {0.94f, 0.30f, 0.035f, 1.0f},
     {0.32f, 0.07f, 0.48f, 1.0f}
 }};
 constexpr float fondoAltarCubierta = -1.425f;
 for (int nivel=0;nivel<3;++nivel) {
  const auto& piso=pisos[nivel];
  const float profundidad=piso.frente-fondoAltarCubierta;
  auto* cubierta=instanciar("cubierta_piso",altarAprobado,
      "Cubierta piso "+std::to_string(nivel+1),
      {0,piso.y+.00015f,(piso.frente+fondoAltarCubierta)*.5f});
  cubierta->escala={2.0f*piso.mitadAncho,1.0f,profundidad};
  cubierta->tinte=coloresPiso[nivel];
 }

}
