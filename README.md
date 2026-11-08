# Medidor de CO₂ con ESP32

Firmware para un medidor de CO₂ de interiores con ESP32 y el sensor Sensirion SCD41. Muestra la concentración en una pantalla OLED, cambia el color de un LED según el nivel, suena una vez cuando hay que ventilar y sirve un panel web con la gráfica de las últimas 24 horas.

**Página e instalación desde el navegador:** https://proyectos.gustavoquintero.dev/medidor-co2/

## Para qué sirve

El CO₂ que exhalamos se acumula en un salón, una oficina o una sala de juntas cerrada. Medirlo dice cuándo hace falta abrir una ventana:

| CO₂ | LED | Qué hacer |
|---|---|---|
| menos de 800 ppm | verde | Aire bueno |
| 800 a 1,199 ppm | ámbar | Abre una ventana |
| 1,200 ppm o más | rojo + 3 pitidos | Ventila ya |

Para bajar de nivel la lectura debe quedar 50 ppm por debajo del umbral, así el LED no parpadea cuando la lectura ronda el límite.

## Piezas

| Pieza | Para qué |
|---|---|
| ESP32 DevKit (ESP32-WROOM-32) | Microcontrolador con WiFi |
| Sensirion SCD41 (módulo I2C) | CO₂ real por espectroscopía fotoacústica, temperatura y humedad |
| Pantalla OLED SSD1306 0.96" I2C | Lectura en el aparato |
| LED WS2812 (un píxel) | Semáforo |
| Zumbador activo de 3.3 V (opcional) | Aviso al llegar a rojo |

## Conexiones

| Pieza | Pin | ESP32 |
|---|---|---|
| SCD41 y OLED | SDA | GPIO21 |
| SCD41 y OLED | SCL | GPIO22 |
| SCD41 y OLED | VCC / GND | 3V3 / GND |
| WS2812 | DIN | GPIO4 (VIN 5 V, GND) |
| Zumbador | + | GPIO25 |
| Botón BOOT | — | GPIO0 (ya viene en la placa) |

## Uso

1. **Instalar:** desde la página del proyecto con Chrome o Edge de escritorio y un cable USB de datos, o con PlatformIO (`pio run -e esp32dev -t upload`).
2. **WiFi:** la primera vez abre la red `medidor-co2`. Conéctate desde el celular y elige tu WiFi. Sin WiFi también mide; solo no hay panel web.
3. **Panel:** abre http://medidor-co2.local (o la IP que muestre tu router).
4. **Botón BOOT:** una pulsación silencia la alarma hasta que el nivel baje; sostenerlo 10 s borra el WiFi guardado.
5. **Calibrar:** deja el medidor 3 minutos junto a una ventana abierta y usa el botón del panel (fija 420 ppm, el CO₂ del aire exterior).

## API del aparato

| Ruta | Respuesta |
|---|---|
| `GET /api/lectura` | `{"valida":true,"co2":812,"temperatura":23.4,"humedad":48,"nivel":"ventilar","consejo":"Abre una ventana","hace":2,"version":"1.0.0"}` |
| `GET /api/historial` | `{"minutos_por_punto":5,"valores":[612,640,...]}` (hasta 288 puntos, 24 h) |
| `POST /api/calibrar?ppm=420` | Recalibración forzada del SCD41 |

## Desarrollo

```bash
pio test -e native     # semáforo con histéresis e historial de 24 h, en la PC
pio run -e esp32dev    # firmware
```

La lógica que no depende del hardware vive en `lib/medidor/` y se prueba en la PC con Unity. GitHub Actions corre esas pruebas, compila el firmware y, al etiquetar una versión, publica en Releases la imagen completa (`medidor-co2.bin`) que usa el botón de instalar del sitio. El botón usa [ESP Web Tools](https://github.com/esphome/esp-web-tools) (Apache-2.0); `herramientas/instalador-es.py` lo descarga y traduce sus textos al español al armar el sitio.

**Estado:** el firmware compila y la lógica tiene pruebas automáticas; aún no se ha probado en un aparato armado.

## Licencia

MIT. Hecho por [Gustavo Quintero](https://gustavoquintero.dev).
