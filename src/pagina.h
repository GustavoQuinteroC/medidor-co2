#pragma once
#include <pgmspace.h>

// Panel que sirve el propio medidor en http://medidor-co2.local. Sin librerías externas:
// funciona en una red sin internet. La gráfica es un SVG que se dibuja con los datos de /api/historial.
static const char PAGINA[] PROGMEM = R"html(<!doctype html>
<html lang="es"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Medidor de CO2</title>
<style>
:root{--f:#081236;--t:#eaf0ff;--t2:#a9b8e6;--c:#0f1f57;--bueno:#00c389;--ventilar:#f5a524;--alto:#e5484d}
*{box-sizing:border-box;margin:0}body{background:var(--f);color:var(--t);font:16px/1.5 system-ui,sans-serif;padding:20px;max-width:720px;margin:auto}
.lectura{display:flex;align-items:baseline;gap:10px;margin-top:8px}.ppm{font:700 clamp(4rem,18vw,7rem)/1 ui-monospace,monospace;letter-spacing:-.04em}
.estado{display:inline-block;margin-top:10px;padding:6px 14px;border-radius:999px;font-weight:700;color:#081236;background:var(--bueno)}
.estado[data-nivel=ventilar]{background:var(--ventilar)}.estado[data-nivel=alto]{background:var(--alto);color:#fff}
.datos{display:flex;gap:24px;margin-top:16px;color:var(--t2)}.datos b{color:var(--t);font-size:1.3rem;display:block}
.tarjeta{background:var(--c);border-radius:18px;padding:16px;margin-top:22px}svg{width:100%;height:auto;display:block}
small{color:var(--t2)}button{font:inherit;border:1px solid #2a3c86;background:none;color:var(--t);border-radius:10px;padding:8px 12px;cursor:pointer}
</style></head><body>
<small>Medidor de CO&#8322;</small>
<div class="lectura"><span class="ppm" id="ppm">--</span><span>ppm</span></div>
<span class="estado" id="estado" data-nivel="bueno">Esperando lectura</span>
<div class="datos"><div><b id="temp">--</b>temperatura</div><div><b id="hum">--</b>humedad</div></div>
<div class="tarjeta"><small>Últimas 24 horas (promedio cada 5 min)</small><svg id="grafica" viewBox="0 0 600 220" role="img" aria-label="Historial de CO2"></svg></div>
<div class="tarjeta"><small>Calibración: deja el medidor 3 minutos junto a una ventana abierta y presiona.</small><p><button id="calibrar">Calibrar a 420 ppm</button> <small id="msg"></small></p></div>
<p><small id="ver"></small></p>
<script>
const $=id=>document.getElementById(id);
async function lectura(){try{const r=await(await fetch('/api/lectura')).json();$('ver').textContent='Firmware '+r.version;
if(!r.valida)return;$('ppm').textContent=r.co2;$('temp').textContent=r.temperatura.toFixed(1)+' °C';$('hum').textContent=r.humedad+' %';
$('estado').textContent=r.consejo;$('estado').dataset.nivel=r.nivel;document.title=r.co2+' ppm · Medidor de CO2'}catch(e){}}
async function historial(){try{const r=await(await fetch('/api/historial')).json();const v=r.valores;const s=$('grafica');
const W=600,H=220,m=30,max=Math.max(1500,...v),y=p=>H-m-(p-400)/(max-400)*(H-2*m);
let h='';for(const [p,c] of [[800,'#f5a524'],[1200,'#e5484d']])h+=`<line x1="${m}" x2="${W}" y1="${y(p)}" y2="${y(p)}" stroke="${c}" stroke-dasharray="4 4"/><text x="0" y="${y(p)+4}" fill="${c}" font-size="11">${p}</text>`;
if(v.length>1){const x=i=>m+i*(W-m)/(v.length-1);h+=`<polyline fill="none" stroke="#5b7cff" stroke-width="2.5" points="${v.map((p,i)=>x(i)+','+y(p)).join(' ')}"/>`}
else h+=`<text x="${W/2}" y="${H/2}" fill="#a9b8e6" text-anchor="middle" font-size="14">La gráfica aparece después de 10 minutos</text>`;s.innerHTML=h}catch(e){}}
$('calibrar').onclick=async()=>{if(!confirm('¿El medidor lleva 3 minutos al aire libre o junto a una ventana abierta?'))return;
$('msg').textContent='Calibrando…';const r=await(await fetch('/api/calibrar',{method:'POST'})).json();$('msg').textContent=r.ok?'Listo. Corrección: '+r.correccion+' ppm':r.error};
lectura();historial();setInterval(lectura,5000);setInterval(historial,60000);
</script></body></html>)html";
