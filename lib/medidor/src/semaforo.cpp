#include "semaforo.h"

Nivel Semaforo::actualizar(uint16_t ppm) {
  const Nivel anterior = nivel_;
  Nivel nuevo;
  if (ppm >= u_.alto) {
    nuevo = Nivel::Alto;
  } else if (ppm >= u_.ventilar) {
    nuevo = (anterior == Nivel::Alto && ppm + u_.histeresis > u_.alto) ? Nivel::Alto : Nivel::Ventilar;
  } else if (anterior == Nivel::Alto && ppm + u_.histeresis > u_.alto) {
    nuevo = Nivel::Alto;
  } else if (anterior != Nivel::Bueno && ppm + u_.histeresis > u_.ventilar) {
    nuevo = Nivel::Ventilar;
  } else {
    nuevo = Nivel::Bueno;
  }
  recienAlto_ = nuevo == Nivel::Alto && anterior != Nivel::Alto;
  nivel_ = nuevo;
  return nivel_;
}

const char* nombreNivel(Nivel nivel) {
  switch (nivel) {
    case Nivel::Alto: return "alto";
    case Nivel::Ventilar: return "ventilar";
    default: return "bueno";
  }
}

const char* consejoNivel(Nivel nivel) {
  switch (nivel) {
    case Nivel::Alto: return "Ventila ya";
    case Nivel::Ventilar: return "Abre una ventana";
    default: return "Aire bueno";
  }
}

uint32_t colorNivel(Nivel nivel) {
  switch (nivel) {
    case Nivel::Alto: return 0xE5484D;
    case Nivel::Ventilar: return 0xF5A524;
    default: return 0x00C389;
  }
}
