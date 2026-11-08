#pragma once
#include <stdint.h>

// Niveles de CO₂ en interiores. Los umbrales son los que se usan con frecuencia para salones y oficinas:
// cerca de 420 ppm es aire de exterior; arriba de 800 conviene abrir; arriba de 1200 hay que ventilar ya.
enum class Nivel : uint8_t { Bueno, Ventilar, Alto };

struct Umbrales {
  uint16_t ventilar = 800;
  uint16_t alto = 1200;
  uint16_t histeresis = 50;  // para bajar de nivel hay que quedar esta cantidad por debajo del umbral
};

class Semaforo {
 public:
  explicit Semaforo(Umbrales umbrales = Umbrales()) : u_(umbrales) {}

  // Devuelve el nivel para una lectura. Sube en cuanto se cruza un umbral y baja solo con margen,
  // así el foco no parpadea cuando la lectura ronda el límite.
  Nivel actualizar(uint16_t ppm);
  Nivel nivel() const { return nivel_; }
  // true solo en la lectura en que se entra a Nivel::Alto (para sonar la alarma una vez).
  bool recienAlto() const { return recienAlto_; }

 private:
  Umbrales u_;
  Nivel nivel_ = Nivel::Bueno;
  bool recienAlto_ = false;
};

const char* nombreNivel(Nivel nivel);
const char* consejoNivel(Nivel nivel);
uint32_t colorNivel(Nivel nivel);  // 0xRRGGBB
