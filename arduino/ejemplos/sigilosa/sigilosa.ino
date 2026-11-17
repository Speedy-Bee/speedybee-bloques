// =====================================================================
//  robot2_bloques · Minisumo 2
//  Programa armado con Bloques Robot 2 (17/11/26, 1:39 p. m.).
//  Si querés cambiarlo, cambiá los bloques y volvé a descargar: no edites a mano.
//  Necesita la librería VL53L0X de Pololu. Placa: Arduino Nano.
// =====================================================================

#include <Wire.h>
#include <VL53L0X.h>

// ---------------- Pines del Minisumo 2 ----------------
const byte DIP1 = 4;
const byte DIP2 = 8;
const int  L1 = 9;          // LED: motor derecho hacia atrás
const int  L3 = 2;          // LED: motor izquierdo hacia adelante
const int  L4 = 3;          // LED: motor izquierdo hacia atrás
const byte BOTON = 12;      // a 5 V con pull-down: arranca al soltarlo
const byte MI_AVANCE = 5, MI_RETROCESO = 6;
const byte MR_AVANCE = 10, MR_RETROCESO = 11;
const byte SENSOR_LINEA = A1, SENSOR_IZQ = A2, SENSOR_DIAG_IZQ = A3, SENSOR_DER = A6, SENSOR_DIAG_DER = A7;
#define TCA9548A_ADDR 0x70
#define CANAL_TOF 7

// ---------------- Ajustes (pestaña "Robot") ----------------
const bool DIP_ON_ES_5V    = true;    // true: el DIP en ON conecta a 5 V
const int  UMBRAL_LINEA    = 900;   // menos que esto = blanco
const int  UMBRAL_OPONENTE = 100;   // más que esto = rival (sensores analógicos)
const int  DISTANCIA_CENTRO = 150;  // mm: el TOF "ve" al rival a esta distancia o menos
const long BAUDIOS         = 115200;
const unsigned long PERIODO_TX = 50;   // ms entre envíos de telemetría

// ---------------- Estado ----------------
VL53L0X sensorToF;
bool tofDisponible = false;
bool sI, sDI, sC, sDD, sD;
int rawI, rawDI, rawDD, rawD;
int linea = 1023, distancia = 8190;
int motIzq = 0, motDer = 0, cuenta = 0;
byte estrategia = 1;
const char* estado = "ESPERA";
bool enCombate = false;
unsigned long inicioCombate = 0, ultimoEnvio = 0;

// =====================================================================
//  Funciones del robot (no hace falta tocarlas)
// =====================================================================
void seleccionarCanal(uint8_t canal) {
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << canal);
  Wire.endTransmission();
}

void led(int pin, bool on) { if (pin >= 0) digitalWrite(pin, on); }

void motor(byte pinAvance, byte pinRetro, int v) {
  v = constrain(v, -255, 255);
  analogWrite(pinAvance, v > 0 ? v : 0);
  analogWrite(pinRetro, v < 0 ? -v : 0);
}

// izq / der = rueda física, de -255 a 255, positivo = adelante
void motores(int izq, int der) {
  motIzq = constrain(izq, -255, 255);
  motDer = constrain(der, -255, 255);
  motor(MI_AVANCE, MI_RETROCESO, motIzq);
  motor(MR_AVANCE, MR_RETROCESO, motDer);
  led(L3, motIzq > 0); led(L4, motIzq < 0); led(L1, motDer < 0);
}

void leerSensores() {
  rawD = analogRead(SENSOR_DER);        sD  = rawD  > UMBRAL_OPONENTE;
  rawDD = analogRead(SENSOR_DIAG_DER);  sDD = rawDD > UMBRAL_OPONENTE;
  rawDI = analogRead(SENSOR_DIAG_IZQ);  sDI = rawDI > UMBRAL_OPONENTE;
  rawI = analogRead(SENSOR_IZQ);        sI  = rawI  > UMBRAL_OPONENTE;
  linea = analogRead(SENSOR_LINEA);
  sC = false;
  distancia = 8190;
  if (tofDisponible) {
    seleccionarCanal(CANAL_TOF);
    int d = sensorToF.readRangeContinuousMillimeters();
    if (!sensorToF.timeoutOccurred()) {
      distancia = d;
      sC = (d > 0 && d <= DISTANCIA_CENTRO);
    }
  }
}

bool hayLinea()    { return linea < UMBRAL_LINEA; }
bool hayOponente() { return sI || sDI || sC || sDD || sD; }

bool dipOn(byte n) { return (digitalRead(n == 1 ? DIP1 : DIP2) == HIGH) == DIP_ON_ES_5V; }
byte leerDip()     { return 1 + (dipOn(1) ? 1 : 0) + (dipOn(2) ? 2 : 0); }

// Telemetría compatible con monitor_minisumo.html
// estado,sI,sDI,sC,sDD,sD,linea,linea,motIzq,motDer,cuenta,estrategia,distanciaTOF
void enviar() {
  Serial.print(estado);     Serial.print(',');
  Serial.print(sI);         Serial.print(',');
  Serial.print(sDI);        Serial.print(',');
  Serial.print(sC);         Serial.print(',');
  Serial.print(sDD);        Serial.print(',');
  Serial.print(sD);         Serial.print(',');
  Serial.print(linea);      Serial.print(',');
  Serial.print(linea);      Serial.print(',');
  Serial.print(motIzq);     Serial.print(',');
  Serial.print(motDer);     Serial.print(',');
  Serial.print(cuenta);     Serial.print(',');
  Serial.print(estrategia); Serial.print(',');
  Serial.println(tofDisponible ? distancia : -1);
}

void telemetria() {
  if (millis() - ultimoEnvio >= PERIODO_TX) { ultimoEnvio = millis(); enviar(); }
}

// Espera sin dejar de leer los sensores
void esperar(unsigned long ms) {
  unsigned long t0 = millis();
  while (millis() - t0 < ms) { leerSensores(); telemetria(); }
}

void frenar() {
  motores(motIzq > 0 ? -150 : (motIzq < 0 ? 150 : 0), motDer > 0 ? -150 : (motDer < 0 ? 150 : 0));
  delay(60);
  motores(0, 0);
}

// Cuenta de seguridad: los LEDs parpadean cada vez más rápido
void cuentaRegresiva(unsigned long segundos, bool luces) {
  estado = "CUENTA";
  unsigned long total = segundos * 1000UL, inicio = millis();
  while (millis() - inicio < total) {
    unsigned long t = millis() - inicio;
    cuenta = (total - t + 999) / 1000;
    int intervalo = map(t, 0, total, 500, 50);
    if (luces) { led(L1, HIGH); led(L3, HIGH); led(L4, HIGH); }
    leerSensores(); enviar();
    delay(intervalo);
    led(L1, LOW); led(L3, LOW); led(L4, LOW);
    delay(intervalo);
  }
  cuenta = 0;
}

void evitarLinea(int retroMs, int lado, int giroMs) {
  if (!hayLinea()) return;
  estado = "LINEA";
  motores(-255, -255); delay(retroMs);
  motores(255 * lado, -255 * lado); delay(giroMs);
}

void pasitos(int avanzaMs, int pausaMs, int vel) {
  unsigned long fase = (millis() - inicioCombate) % (unsigned long)(avanzaMs + pausaMs);
  if (fase < (unsigned long)avanzaMs) motores(vel, vel); else motores(0, 0);
}

// =====================================================================
//  Al encender
// =====================================================================
void setup() {
  Serial.begin(BAUDIOS);
  Wire.begin();
  pinMode(L1, OUTPUT); pinMode(L3, OUTPUT); pinMode(L4, OUTPUT);
  pinMode(MI_AVANCE, OUTPUT); pinMode(MI_RETROCESO, OUTPUT);
  pinMode(MR_AVANCE, OUTPUT); pinMode(MR_RETROCESO, OUTPUT);
  pinMode(BOTON, INPUT);
  pinMode(DIP1, DIP_ON_ES_5V ? INPUT : INPUT_PULLUP);
  pinMode(DIP2, DIP_ON_ES_5V ? INPUT : INPUT_PULLUP);
  seleccionarCanal(CANAL_TOF);
  if (sensorToF.init()) {
    sensorToF.setTimeout(50);
    sensorToF.setMeasurementTimingBudget(20000);
    sensorToF.startContinuous();
    tofDisponible = true;
  }
  randomSeed(analogRead(A0));
  motores(0, 0);
}

// =====================================================================
//  Bucle principal
// =====================================================================
void loop() {
  leerSensores();

  if (!enCombate) {
    // Esperando el botón: quieto, leyendo los DIP
    estado = "ESPERA";
    estrategia = leerDip();
    motores(0, 0);
    telemetria();
    if (digitalRead(BOTON) == HIGH) {
      delay(30);
      if (digitalRead(BOTON) == HIGH) {
        while (digitalRead(BOTON) == HIGH) { motores(0, 0); }   // arranca al soltar
        delay(30);
        estrategia = leerDip();
        cuentaRegresiva(5, true);
        enCombate = true;
        inicioCombate = millis();
        estado = "SALIDA";
        leerSensores();

        // ---- tus bloques de la salida ----
        {  // durante 200 ms, se corta si ve al rival o la línea
          motores(0, 0);
          unsigned long t0_1 = millis();
          while (millis() - t0_1 < (unsigned long)(200)) {
            leerSensores();
            telemetria();
            if (hayOponente() || hayLinea()) break;
          }
        }
      }
    }
    return;
  }

  // ---- tus bloques del combate (se repiten siempre) ----
  estado = "COMBATE";
  evitarLinea(120, 1, 100);
  if ((distancia >= 0 && distancia < (30))) {
    estado = "ATAQUE";
    motores(255, 255);
  } else if (sDD) {
    estado = "CENTRANDO";
    motores(60, -60);
  } else if (sDI) {
    estado = "CENTRANDO";
    motores(-60, 60);
  } else if (sD) {
    estado = "CENTRANDO";
    motores(90, -90);
  } else if (sI) {
    estado = "CENTRANDO";
    motores(-90, 90);
  } else {
    estado = "PASITOS";
    pasitos(40, 40, 120);
  }

  telemetria();
}
