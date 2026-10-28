#pragma once
#include "render/Model.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

// Envolventes conservadoras por celda. Recorta cada triángulo real contra
// la celda: las alturas extremas no proceden de una caja global del dulce.
// El contacto puede tener una holgura submilimétrica por discretización.
struct ApoyoDulce {
 static constexpr int N=256;
 glm::vec3 offset;
 glm::vec2 tam,paso;
 std::vector<float> inf,sup;
 explicit ApoyoDulce(const Modelo& m):
  offset((m.min.x+m.max.x)*.5f,m.baseY,(m.min.z+m.max.z)*.5f),
  tam(m.max.x-m.min.x,m.max.z-m.min.z),paso(tam/float(N)),
  inf(N*N,1e20f),sup(N*N,-1e20f) {
  for(const auto& parte:m.plano) {
   const auto& vs=m.mallas[parte.malla].superficie;
   for(size_t k=0;k+2<vs.size();k+=3) {
    glm::vec3 v[3];for(int j=0;j<3;++j)v[j]=glm::vec3(parte.matriz*glm::vec4(vs[k+j],1))-offset;
    const glm::vec3 lo=glm::min(v[0],glm::min(v[1],v[2])),hi=glm::max(v[0],glm::max(v[1],v[2]));
    auto ix=[&](float x){return std::clamp(int(std::floor((x+tam.x*.5f)/paso.x)),0,N-1);};
    auto iz=[&](float z){return std::clamp(int(std::floor((z+tam.y*.5f)/paso.y)),0,N-1);};
    for(int z=iz(lo.z);z<=iz(hi.z);++z)for(int x=ix(lo.x);x<=ix(hi.x);++x) {
     std::vector<glm::vec3> pol{v[0],v[1],v[2]};
     auto cortar=[&](int eje,float limite,bool mayor) {
      if(pol.empty())return;
      std::vector<glm::vec3> siguiente;
      auto a=pol.back();bool dentroA=mayor?a[eje]>=limite:a[eje]<=limite;
      for(auto b:pol) {
       const bool dentroB=mayor?b[eje]>=limite:b[eje]<=limite;
       if(dentroA!=dentroB)siguiente.push_back(a+(b-a)*((limite-a[eje])/(b[eje]-a[eje])));
       if(dentroB)siguiente.push_back(b);
       a=b;dentroA=dentroB;
      }
      pol=std::move(siguiente);
     };
     const float xmin=-tam.x*.5f+x*paso.x,zmin=-tam.y*.5f+z*paso.y;
     cortar(0,xmin,true);cortar(0,xmin+paso.x,false);
     cortar(2,zmin,true);cortar(2,zmin+paso.y,false);
     for(const auto& q:pol) {inf[z*N+x]=std::min(inf[z*N+x],q.y);sup[z*N+x]=std::max(sup[z*N+x],q.y);}
    }
   }
  }
 }
 // Altura relativa de una copia superior para no penetrar otra copia inferior.
 // Todas las piezas del montoncito comparten giro y escala; delta está en su plano local.
 float alturaSobre(glm::vec2 delta) const {
  float altura=-1e20f;
  for(int z=0;z<N;++z)for(int x=0;x<N;++x) {
   const float fondo=inf[z*N+x];if(fondo>1e10f)continue;
   const float bx=x+delta.x/paso.x,bz=z+delta.y/paso.y;
   const int x0=std::max(0,int(std::floor(bx))),x1=std::min(N-1,int(std::floor(bx+1)));
   const int z0=std::max(0,int(std::floor(bz))),z1=std::min(N-1,int(std::floor(bz+1)));
   for(int j=z0;j<=z1;++j)for(int i=x0;i<=x1;++i)
    if(sup[j*N+i]>-1e10f)altura=std::max(altura,sup[j*N+i]-fondo);
  }
  return altura;
 }
};
