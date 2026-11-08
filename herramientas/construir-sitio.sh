#!/usr/bin/env bash
# Arma dist/ con la landing y la imagen del firmware del último release, lista para publicar.
set -euo pipefail
RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
REPO="GustavoQuinteroC/medidor-co2"
rm -rf "$RAIZ/dist"
cp -r "$RAIZ/sitio" "$RAIZ/dist"
mkdir -p "$RAIZ/dist/firmware"
ETIQUETA="$(curl -sf "https://api.github.com/repos/$REPO/releases/latest" | python3 -c 'import json,sys;print(json.load(sys.stdin)["tag_name"])')"
curl -sfL -o "$RAIZ/dist/firmware/medidor-co2.bin" "https://github.com/$REPO/releases/download/$ETIQUETA/medidor-co2.bin"
# Instalador ESP Web Tools servido desde el sitio y traducido al español.
python3 -I "$RAIZ/herramientas/instalador-es.py" "$RAIZ/dist/vendor/esp-web-tools"
# La versión del manifest sigue a la etiqueta para que el instalador ofrezca actualizar.
sed -i "s/\"version\": \"[^\"]*\"/\"version\": \"${ETIQUETA#v}\"/" "$RAIZ/dist/manifest.json"
echo "dist/ listo con el firmware $ETIQUETA ($(stat -c %s "$RAIZ/dist/firmware/medidor-co2.bin") bytes)"
