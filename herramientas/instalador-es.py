#!/usr/bin/env python3
"""Descarga ESP Web Tools y traduce al español los textos que ve quien instala.

ESP Web Tools no trae traducciones: sus textos vienen fijos en inglés dentro de los
archivos JS. Este script baja la versión fijada desde jsDelivr, cambia cada texto y
falla si alguno no aparece las veces esperadas (así una versión nueva no deja textos
en inglés sin avisar).

Uso: python3 herramientas/instalador-es.py <carpeta destino>
"""

import json
import re
import sys
import urllib.request
from pathlib import Path

VERSION = "10.4.0"
PAQUETE = f"esp-web-tools@{VERSION}"
CDN = f"https://cdn.jsdelivr.net/npm/{PAQUETE}"
LISTA = f"https://data.jsdelivr.com/v1/packages/npm/{PAQUETE}"

# (archivo, texto original, traducción, veces que aparece).
# En el texto original un espacio equivale a cualquier cantidad de espacios o saltos
# de línea, porque las plantillas del paquete parten las frases en varias líneas.
TRADUCCIONES = [
    # Botón
    ("install-button.js", "<slot name='unsupported'>Your browser does not support installing things on ESP devices. Use Mozilla Firefox, Google Chrome or Microsoft Edge.</slot>",
     "<slot name='unsupported'>Tu navegador no puede grabar por USB. Usa Chrome o Edge en una computadora de escritorio.</slot>", 1),
    ("install-button.js", "<slot name='not-allowed'>You can only install ESP devices on HTTPS websites or on the localhost.</slot>",
     "<slot name='not-allowed'>La instalación necesita una página con HTTPS.</slot>", 1),
    ("install-button.js", 'n.innerText="Connect"', 'n.innerText="Conectar"', 1),

    # Diálogo cuando no se eligió puerto
    ("index-*.js", '<div slot="headline">No port selected</div>', '<div slot="headline">No se eligió ningún puerto</div>', 1),
    ("index-*.js", "If you didn't select a port because you didn't see your device listed, try the following steps:",
     "Si no elegiste un puerto porque el medidor no aparecía en la lista, revisa esto:", 1),
    ("index-*.js", "Make sure that the device is connected to this computer (the one that runs the browser that shows this website)",
     "Que el medidor esté conectado a esta computadora (la que tiene abierta esta página).", 1),
    ("index-*.js", "Most devices have a tiny light when it is powered on. If yours has one, make sure it is on.",
     "Casi todas las placas tienen un LED que prende al recibir corriente. Si la tuya lo tiene, que esté encendido.", 1),
    ("index-*.js", "Make sure that the USB cable you use can be used for data and is not a power-only cable.",
     "Que el cable USB sea de datos. Muchos cables solo sirven para cargar.", 1),
    ("index-*.js", "If you are using a Linux flavor, make sure that your user is part of the <code>dialout</code> group so it has permission to access the device.",
     "En Linux, que tu usuario pertenezca al grupo <code>dialout</code> para tener permiso de usar el puerto.", 1),
    ("index-*.js", "sudo usermod -a -G dialout YourUserName", "sudo usermod -a -G dialout tu_usuario", 1),
    ("index-*.js", "You may need to log out & back in or reboot to activate the new group access.",
     "Después cierra sesión y vuelve a entrar, o reinicia, para que el cambio tome efecto.", 1),
    ("index-*.js", "Make sure you have the right drivers installed. Below are the drivers for common chips used in ESP devices:",
     "Que tengas instalado el driver del chip USB de tu placa. Estos son los más comunes:", 1),
    ("index-*.js", "CP2102 drivers:", "Driver CP2102:", 1),
    ("index-*.js", "CH342, CH343, CH9102 drivers:", "Driver CH342, CH343 y CH9102:", 1),
    ("index-*.js", "CH340, CH341 drivers:", "Driver CH340 y CH341:", 1),
    ("index-*.js", ">Windows & Mac</a", ">Windows y Mac</a", 1),
    ("index-*.js", "(download via blue button with ${s} icon)", "(se descarga con el botón azul del ícono ${s})", 2),
    ("index-*.js", ">Cancel</ew-text-button>", ">Cancelar</ew-text-button>", 1),
    ("index-*.js", "> Try Again </ew-text-button>", ">Intentar de nuevo</ew-text-button>", 1),
    ("index-*.js", ">Close</ew-text-button>", ">Cerrar</ew-text-button>", 1),

    # Diálogo de instalación: títulos y pantallas
    ("install-dialog-*.js", 't=this._renderProgress("Connecting")', 't=this._renderProgress("Conectando")', 1),
    ("install-dialog-*.js", ">Close</ew-text-button>", ">Cerrar</ew-text-button>", 1),
    ("install-dialog-*.js", "Connected to ${this._info.name}", "Conectado a ${this._info.name}", 1),
    ("install-dialog-*.js", "`Update ${this._manifest.name}`", "`Actualizar ${this._manifest.name}`", 1),
    ("install-dialog-*.js", "`Install ${this._manifest.name}`", "`Instalar ${this._manifest.name}`", 2),
    ("install-dialog-*.js", ">Visit Device<", ">Abrir el panel del medidor<", 2),
    ("install-dialog-*.js", ">Add to Home Assistant<", ">Agregar a Home Assistant<", 2),
    ("install-dialog-*.js", '"Change Wi-Fi":"Connect to Wi-Fi"', '"Cambiar el Wi-Fi":"Conectar al Wi-Fi"', 1),
    ("install-dialog-*.js", ">Logs & Console<", ">Registro y consola<", 2),
    ("install-dialog-*.js", ">Fund Development<", ">Apoyar el desarrollo<", 1),
    ("install-dialog-*.js", '<div slot="headline">Erase User Data</div>', '<div slot="headline">Borrar datos guardados</div>', 1),
    ("install-dialog-*.js", 'i="Configure Wi-Fi"', 'i="Configurar el Wi-Fi"', 1),
    ("install-dialog-*.js", '"Trying to connect"', '"Intentando conectar"', 1),
    ("install-dialog-*.js", "The connected device has Wi-Fi turned off, so it can't be configured right now.<br />Enable the device's Wi-Fi, then try again.",
     "El medidor tiene el Wi-Fi apagado y no se puede configurar ahora.<br />Enciende su Wi-Fi e intenta de nuevo.", 1),
    ("install-dialog-*.js", "> Back </ew-text-button>", ">Atrás</ew-text-button>", 5),
    ("install-dialog-*.js", '"Scanning for networks"', '"Buscando redes"', 1),
    ("install-dialog-*.js", 'i="Unable to connect"', 'i="No se pudo conectar"', 1),
    ("install-dialog-*.js", 'i="Timeout"', 'i="Se agotó el tiempo"', 1),
    ("install-dialog-*.js", "i=`Unknown error (${this._client.error})`", "i=`Error desconocido (${this._client.error})`", 1),
    ("install-dialog-*.js", "<div>Connect your device to the network to start using it.</div>",
     "<div>Conecta el medidor a tu red para ver su panel desde el celular o la computadora.</div>", 1),
    ("install-dialog-*.js", 'label="Network"', 'label="Red"', 1),
    ("install-dialog-*.js", "> Join other… <", ">Otra red…<", 1),
    ("install-dialog-*.js", 'label="Network Name"', 'label="Nombre de la red"', 1),
    ("install-dialog-*.js", 'label="Password"', 'label="Contraseña"', 1),
    ("install-dialog-*.js", '"Skip":"Back"', '"Omitir":"Atrás"', 1),
    ("install-dialog-*.js", ">Connect</ew-text-button>", ">Conectar</ew-text-button>", 1),
    ("install-dialog-*.js", 'label="Device connected to the network!"', 'label="¡El medidor ya está en tu red!"', 1),
    ("install-dialog-*.js", '<div slot="headline">Skip</div>', '<div slot="headline">Omitir</div>', 1),
    ("install-dialog-*.js", "> Continue </ew-text-button>", ">Continuar</ew-text-button>", 1),
    ("install-dialog-*.js", '["Erase device",', '["Borrar el medidor",', 1),
    ("install-dialog-*.js", "Do you want to erase the device before installing ${this._manifest.name}? All data on the device will be lost.",
     "¿Quieres borrar el medidor antes de instalar ${this._manifest.name}? Se pierde todo lo que tenga guardado.", 1),
    ("install-dialog-*.js", "</ew-checkbox> Erase device </label>", "</ew-checkbox> Borrar el medidor </label>", 1),
    ("install-dialog-*.js", "> Next </ew-text-button>", ">Siguiente</ew-text-button>", 2),
    ("install-dialog-*.js", 'e="Erase User Data"', 'e="Borrar datos guardados"', 1),
    ("install-dialog-*.js", "Do you want to reset your device and erase all user data from your device?",
     "¿Quieres reiniciar el medidor y borrar todo lo que tiene guardado, incluido el Wi-Fi?", 1),
    ("install-dialog-*.js", "> Erase User Data </ew-text-button>", ">Borrar datos guardados</ew-text-button>", 1),
    ("install-dialog-*.js", 'e="Installing"', 'e="Instalando"', 3),
    ("install-dialog-*.js", 'this._renderProgress("Erasing")', 'this._renderProgress("Borrando")', 1),
    ("install-dialog-*.js", 'r="Wrapping up"', 'r="Terminando"', 1),
    ("install-dialog-*.js", 'r="Installing"', 'r="Instalando"', 1),
    ("install-dialog-*.js", 'This will take ${"ESP8266"===this._installState.chipFamily?"a minute":"2 minutes"}.<br /> Keep this page visible to prevent slow down',
     'Tarda ${"ESP8266"===this._installState.chipFamily?"un minuto":"unos 2 minutos"}.<br /> Deja esta página a la vista para que no se haga lenta', 1),
    ("install-dialog-*.js", 'label="Installation complete!"', 'label="¡Instalación terminada!"', 1),
    ("install-dialog-*.js", 'e="Installation failed"', 'e="La instalación falló"', 1),
    ("install-dialog-*.js", 'this._renderProgress("Preparing installation")', 'this._renderProgress("Preparando la instalación")', 1),
    ("install-dialog-*.js", 'e="Confirm Installation"', 'e="Confirmar la instalación"', 1),
    ("install-dialog-*.js", 'const r=i?"update to":"install"', 'const r=i?"actualizar a":"instalar"', 1),
    ("install-dialog-*.js", "Your device is running ${this._info.firmware}&nbsp;${this._info.version}.",
     "Tu medidor tiene ${this._info.firmware}&nbsp;${this._info.version}.", 1),
    ("install-dialog-*.js", "Do you want to ${r} ${this._manifest.name}&nbsp;${this._manifest.version}?",
     "¿Quieres ${r} ${this._manifest.name}&nbsp;${this._manifest.version}?", 1),
    ("install-dialog-*.js", "<br /><br />All data on the device will be erased.", "<br /><br />Se borrará todo lo que tenga guardado.", 1),
    ("install-dialog-*.js", "> Install </ew-text-button>", ">Instalar</ew-text-button>", 1),
    ("install-dialog-*.js", "> Reset Device </ew-text-button>", ">Reiniciar el medidor</ew-text-button>", 1),
    ("install-dialog-*.js", "> Download Logs </ew-text-button>", ">Descargar el registro</ew-text-button>", 1),
    ("install-dialog-*.js", '["Logs",e]', '["Registro",e]', 1),

    # Diálogo de instalación: errores y avance
    ("install-dialog-*.js", '"Serial port is not readable/writable. Close any other application using it and try again."',
     '"No se puede usar el puerto USB. Cierra cualquier otro programa que lo tenga abierto e intenta de nuevo."', 1),
    ("install-dialog-*.js", '"Failed to download manifest"', '"No se pudo descargar la descripción del firmware."', 1),
    ("install-dialog-*.js", '"Serial port is not ready. Close any other application using it and try again."',
     '"El puerto USB no está listo. Cierra cualquier otro programa que lo tenga abierto e intenta de nuevo."', 1),
    ("install-dialog-*.js", 'message:"Initializing..."', 'message:"Iniciando..."', 1),
    ("install-dialog-*.js", 'message:"Failed to initialize. Try resetting your device or holding the BOOT button while clicking INSTALL."',
     'message:"No se pudo iniciar la placa. Desconéctala y vuelve a conectarla, o mantén presionado el botón BOOT mientras das clic en Instalar."', 1),
    ("install-dialog-*.js", "message:`Initialized. Found ${a}`", "message:`Placa ${a} lista`", 1),
    ("install-dialog-*.js", "message:`Your ${a} board is not supported.`", "message:`Este firmware no es para la placa ${a}. Usa un ESP32.`", 1),
    ("install-dialog-*.js", 'message:"Preparing installation..."', 'message:"Preparando la instalación..."', 1),
    ("install-dialog-*.js", "`Downloading firmware ${e.path} failed: ${i.status}`", "`No se pudo descargar el firmware ${e.path} (${i.status})`", 1),
    ("install-dialog-*.js", 'message:"Installation prepared"', 'message:"Instalación preparada"', 1),
    ("install-dialog-*.js", 'message:"Erasing device..."', 'message:"Borrando el medidor..."', 1),
    ("install-dialog-*.js", 'message:"Device erased"', 'message:"Medidor borrado"', 1),
    ("install-dialog-*.js", 'message:"Writing progress: 0%"', 'message:"Grabando: 0%"', 1),
    ("install-dialog-*.js", "message:`Writing progress: ${s}%`", "message:`Grabando: ${s}%`", 1),
    ("install-dialog-*.js", 'message:"Writing complete"', 'message:"Grabación terminada"', 1),
    ("install-dialog-*.js", 'message:"All done!"', 'message:"¡Listo!"', 1),
    ("install-dialog-*.js", 'throw new Se("Failed to connect with the device")', 'throw new Se("No se pudo conectar con la placa")', 1),
]


def patron(texto: str) -> re.Pattern:
    return re.compile(r"\s*".join(re.escape(parte) for parte in texto.split(" ")))


def descargar(url: str) -> bytes:
    with urllib.request.urlopen(url, timeout=60) as respuesta:
        return respuesta.read()


def archivos_web() -> list[str]:
    datos = json.loads(descargar(LISTA))
    rutas = []

    def recorrer(nodo, ruta=""):
        for hijo in nodo.get("files", []):
            actual = f"{ruta}/{hijo['name']}"
            if hijo["type"] == "directory":
                recorrer(hijo, actual)
            elif actual.startswith("/dist/web/") and actual.endswith(".js"):
                rutas.append(actual.removeprefix("/dist/web/"))

    recorrer(datos)
    if "install-button.js" not in rutas:
        sys.exit("La lista del paquete no trae install-button.js")
    return rutas


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    destino = Path(sys.argv[1])
    destino.mkdir(parents=True, exist_ok=True)

    textos = {nombre: descargar(f"{CDN}/dist/web/{nombre}").decode() for nombre in archivos_web()}
    errores = []
    for comodin, original, traduccion, veces in TRADUCCIONES:
        nombre = next((n for n in textos if Path(n).match(comodin)), None)
        if nombre is None:
            errores.append(f"no hay archivo {comodin}")
            continue
        textos[nombre], cambios = patron(original).subn(lambda _: traduccion, textos[nombre])
        if cambios != veces:
            errores.append(f"{comodin}: «{original[:60]}» apareció {cambios} veces, se esperaban {veces}")
    if errores:
        sys.exit("Traducción incompleta:\n  " + "\n  ".join(errores))

    for nombre, contenido in textos.items():
        (destino / nombre).write_text(contenido, encoding="utf-8")
    (destino / "LICENSE").write_bytes(descargar(f"{CDN}/LICENSE"))
    print(f"ESP Web Tools {VERSION} en español: {len(textos)} archivos, {len(TRADUCCIONES)} textos")


if __name__ == "__main__":
    main()
