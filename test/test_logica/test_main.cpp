#include <string.h>
#include <unity.h>

#include "historial.h"
#include "semaforo.h"

void setUp() {}
void tearDown() {}

void test_niveles_basicos() {
  Semaforo s;
  TEST_ASSERT_TRUE(s.actualizar(420) == Nivel::Bueno);
  TEST_ASSERT_TRUE(s.actualizar(800) == Nivel::Ventilar);
  TEST_ASSERT_TRUE(s.actualizar(1200) == Nivel::Alto);
  TEST_ASSERT_EQUAL_STRING("alto", nombreNivel(s.nivel()));
  TEST_ASSERT_EQUAL_STRING("Ventila ya", consejoNivel(Nivel::Alto));
  TEST_ASSERT_EQUAL_HEX32(0x00C389, colorNivel(Nivel::Bueno));
}

void test_histeresis_evita_parpadeo() {
  Semaforo s;
  s.actualizar(1210);
  TEST_ASSERT_TRUE(s.actualizar(1190) == Nivel::Alto);  // dentro del margen: sigue alto
  TEST_ASSERT_TRUE(s.actualizar(1151) == Nivel::Alto);
  TEST_ASSERT_TRUE(s.actualizar(1150) == Nivel::Ventilar);  // ya bajó 50 ppm
  TEST_ASSERT_TRUE(s.actualizar(790) == Nivel::Ventilar);
  TEST_ASSERT_TRUE(s.actualizar(751) == Nivel::Ventilar);
  TEST_ASSERT_TRUE(s.actualizar(750) == Nivel::Bueno);
  TEST_ASSERT_TRUE(s.actualizar(799) == Nivel::Bueno);  // subir no lleva margen
  TEST_ASSERT_TRUE(s.actualizar(800) == Nivel::Ventilar);
}

void test_de_alto_a_bueno_de_golpe() {
  Semaforo s;
  s.actualizar(1500);
  TEST_ASSERT_TRUE(s.actualizar(500) == Nivel::Bueno);  // abrieron todo: no se queda en ventilar
}

void test_alarma_suena_una_vez_por_subida() {
  Semaforo s;
  s.actualizar(900);
  TEST_ASSERT_FALSE(s.recienAlto());
  s.actualizar(1250);
  TEST_ASSERT_TRUE(s.recienAlto());
  s.actualizar(1300);
  TEST_ASSERT_FALSE(s.recienAlto());
  s.actualizar(700);
  s.actualizar(1250);
  TEST_ASSERT_TRUE(s.recienAlto());
}

void test_umbrales_propios() {
  Umbrales u;
  u.ventilar = 1000;
  u.alto = 1500;
  Semaforo s(u);
  TEST_ASSERT_TRUE(s.actualizar(999) == Nivel::Bueno);
  TEST_ASSERT_TRUE(s.actualizar(1000) == Nivel::Ventilar);
}

void test_historial_promedia_por_intervalo() {
  Historial h(300);
  // Lecturas cada 5 s: el primer intervalo (0 a 295 s) alterna 600 y 601.
  for (uint32_t t = 0; t < 300; t += 5) h.agregar(t % 10 ? 601 : 600, t);
  TEST_ASSERT_EQUAL_UINT32(0, h.cantidad());  // el intervalo sigue abierto
  h.agregar(900, 300);                         // abre el segundo y cierra el primero
  TEST_ASSERT_EQUAL_UINT32(1, h.cantidad());
  TEST_ASSERT_EQUAL_UINT16(601, h.valor(0));  // 600.5 redondea hacia arriba
}

void test_historial_conserva_las_ultimas_24_horas() {
  Historial h(300);
  const uint32_t intervalos = Historial::CAPACIDAD + 10;
  for (uint32_t i = 0; i <= intervalos; i++) h.agregar(static_cast<uint16_t>(400 + i), i * 300);
  TEST_ASSERT_EQUAL_UINT32(Historial::CAPACIDAD, h.cantidad());
  TEST_ASSERT_EQUAL_UINT16(400 + 10, h.valor(0));  // los 10 más viejos se descartaron
  TEST_ASSERT_EQUAL_UINT16(400 + intervalos - 1, h.valor(Historial::CAPACIDAD - 1));
  TEST_ASSERT_EQUAL_UINT16(0, h.valor(Historial::CAPACIDAD));
}

void test_historial_hueco_sin_lecturas() {
  Historial h(300);
  h.agregar(500, 0);
  h.agregar(700, 5000);  // el sensor no respondió más de una hora
  h.agregar(700, 5300);
  TEST_ASSERT_EQUAL_UINT32(2, h.cantidad());
  TEST_ASSERT_EQUAL_UINT16(500, h.valor(0));
  TEST_ASSERT_EQUAL_UINT16(700, h.valor(1));
}

void test_historial_json() {
  Historial h(60);
  char buf[64];
  TEST_ASSERT_EQUAL_UINT32(2, h.json(buf, sizeof buf));
  TEST_ASSERT_EQUAL_STRING("[]", buf);
  for (uint32_t i = 0; i < 4; i++) h.agregar(static_cast<uint16_t>(410 + i), i * 60);
  TEST_ASSERT_EQUAL_UINT32(strlen("[410,411,412]"), h.json(buf, sizeof buf));
  TEST_ASSERT_EQUAL_STRING("[410,411,412]", buf);
  TEST_ASSERT_EQUAL_UINT32(0, h.json(buf, 8));  // no cabe: no escribe a medias
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_niveles_basicos);
  RUN_TEST(test_histeresis_evita_parpadeo);
  RUN_TEST(test_de_alto_a_bueno_de_golpe);
  RUN_TEST(test_alarma_suena_una_vez_por_subida);
  RUN_TEST(test_umbrales_propios);
  RUN_TEST(test_historial_promedia_por_intervalo);
  RUN_TEST(test_historial_conserva_las_ultimas_24_horas);
  RUN_TEST(test_historial_hueco_sin_lecturas);
  RUN_TEST(test_historial_json);
  return UNITY_END();
}
