# Bloques Robot 2 · SpeedyBee

<img src="logo.png" alt="SpeedyBee" width="160">

Programá nuestro minisumo **Robot 2** con bloques (como Scratch), probalo en un **simulador 3D** y descargá el código para el Arduino.

## Cómo abrirlo

- **En la compu:** descargá este repositorio y abrí `index.html` con Chrome o Edge (necesita internet para cargar Blockly y Three.js).
- **En la web:** activá GitHub Pages (Settings → Pages → rama `main`, carpeta raíz) y queda en `https://<usuario>.github.io/<repositorio>/`.

## Qué tiene

- **Lecciones desde cero** (13): prender un motor, avanzar, girar, PWM, repetir, sensor de línea, TOF, sensores laterales, buscar y atacar, estrategias con los DIP. Cada una carga sus bloques y prepara el simulador.
- **Simulador 3D** con el modelo de nuestro robot: física con agarre de las gomas, ruedas que giran, cuchilla que levanta, haces de luz de los sensores, robot en el banco (ruedas al aire), rival que se ubica tocando el dohyo y monitor.
- **Descargar .ino**: baja un .zip con la carpeta lista para el Arduino IDE.

## Robot 2 (Arduino Nano)

| Parte | Pin |
|---|---|
| Motor izquierdo (avance / retroceso) | 5 / 6 |
| Motor derecho (avance / retroceso) | 10 / 11 |
| Sensor de línea | A1 |
| Lateral izq. / diagonal izq. | A2 / A3 |
| Diagonal der. / lateral der. | A7 / A6 |
| TOF VL53L0X | I²C por TCA9548A (0x70), canal 7 |
| Botón de inicio | 12 (a 5 V, arranca al soltarlo) |
| DIP 1 / DIP 2 | 4 / 8 |
| LEDs | 2, 3, 9 |

Necesita la librería **VL53L0X** de Pololu.

## Carpeta `arduino/ejemplos`

Los tres ejemplos de la página ya generados: `simple`, `competencia` (estrategias por DIP) y `sigilosa`.
