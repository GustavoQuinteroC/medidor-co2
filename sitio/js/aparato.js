// Un solo momento animado: la lectura del aparato sube como en un salón cerrado y el LED cambia de color.
const ppm = document.getElementById('ppm');
const consejo = document.getElementById('consejo');
const aparato = document.getElementById('aparato');
const niveles = [[1200, 'alto', 'Ventila ya'], [800, 'ventilar', 'Abre una ventana'], [0, 'bueno', 'Aire bueno']];

function pintar(valor) {
  const [, nivel, texto] = niveles.find(([umbral]) => valor >= umbral);
  ppm.textContent = valor.toLocaleString('es-MX');
  consejo.textContent = texto;
  aparato.dataset.nivel = nivel;
}

if (matchMedia('(prefers-reduced-motion: reduce)').matches) {
  pintar(1240);
} else {
  let valor = 610;
  pintar(valor);
  const paso = setInterval(() => {
    valor = Math.min(1240, valor + 18 + Math.round(Math.random() * 14));
    pintar(valor);
    if (valor >= 1240) clearInterval(paso);
  }, 90);
}
