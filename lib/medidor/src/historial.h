#pragma once
#include <stddef.h>
#include <stdint.h>

// Promedios de CO₂ por intervalo en un búfer circular. Con 288 intervalos de 5 minutos guarda 24 horas
// en menos de 600 bytes de RAM.
class Historial {
 public:
  static constexpr size_t CAPACIDAD = 288;

  explicit Historial(uint32_t segundosPorIntervalo = 300) : intervalo_(segundosPorIntervalo) {}

  // Suma una lectura tomada en el segundo "ahora" (segundos desde el arranque). Cierra el intervalo
  // cuando pasa su duración y guarda el promedio redondeado.
  void agregar(uint16_t ppm, uint32_t ahora);

  size_t cantidad() const { return cantidad_; }
  // i = 0 es el valor más antiguo que se conserva.
  uint16_t valor(size_t i) const;
  uint32_t segundosPorIntervalo() const { return intervalo_; }

  // Escribe "[412,415,...]" en orden cronológico. Devuelve los caracteres escritos (sin el nulo) o 0
  // si el búfer no alcanza.
  size_t json(char* destino, size_t tamano) const;

 private:
  void guardar(uint16_t promedio);

  uint32_t intervalo_;
  uint16_t valores_[CAPACIDAD] = {};
  size_t inicio_ = 0;
  size_t cantidad_ = 0;
  uint32_t suma_ = 0;
  uint16_t muestras_ = 0;
  uint32_t inicioIntervalo_ = 0;
  bool hayIntervalo_ = false;
};
