#include "historial.h"

#include <stdio.h>

void Historial::agregar(uint16_t ppm, uint32_t ahora) {
  if (!hayIntervalo_) {
    inicioIntervalo_ = ahora;
    hayIntervalo_ = true;
  }
  // Si pasó más de un intervalo (por ejemplo, el sensor falló un rato) se cierra el que había
  // y el nuevo empieza en esta lectura.
  if (ahora - inicioIntervalo_ >= intervalo_) {
    if (muestras_ > 0) guardar(static_cast<uint16_t>((suma_ + muestras_ / 2) / muestras_));
    suma_ = 0;
    muestras_ = 0;
    inicioIntervalo_ = ahora;
  }
  suma_ += ppm;
  muestras_++;
}

void Historial::guardar(uint16_t promedio) {
  const size_t posicion = (inicio_ + cantidad_) % CAPACIDAD;
  valores_[posicion] = promedio;
  if (cantidad_ < CAPACIDAD) {
    cantidad_++;
  } else {
    inicio_ = (inicio_ + 1) % CAPACIDAD;
  }
}

uint16_t Historial::valor(size_t i) const {
  return i < cantidad_ ? valores_[(inicio_ + i) % CAPACIDAD] : 0;
}

size_t Historial::json(char* destino, size_t tamano) const {
  if (tamano < 3) return 0;
  size_t usado = 0;
  destino[usado++] = '[';
  for (size_t i = 0; i < cantidad_; i++) {
    const int n = snprintf(destino + usado, tamano - usado, i ? ",%u" : "%u", static_cast<unsigned>(valor(i)));
    if (n < 0 || static_cast<size_t>(n) >= tamano - usado) return 0;
    usado += static_cast<size_t>(n);
  }
  if (usado + 2 > tamano) return 0;
  destino[usado++] = ']';
  destino[usado] = '\0';
  return usado;
}
