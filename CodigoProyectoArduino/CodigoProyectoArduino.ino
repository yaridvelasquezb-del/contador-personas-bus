#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===============================
// SENSOR DE ENTRADA
// ===============================
#define TRIG_ENTRADA 2
#define ECHO_ENTRADA 3

// ===============================
// SENSOR DE SALIDA
// ===============================
#define TRIG_SALIDA 10
#define ECHO_SALIDA 11

// ===============================
// CONFIGURACIÓN DEL AUTOBÚS
// ===============================
const int CAPACIDAD_MAXIMA = 12;

int puestosDisponibles = CAPACIDAD_MAXIMA;

// Distancia máxima para detectar persona
const int DISTANCIA_DETECCION = 7;

// Tiempo que se muestra "Persona entro/salio"
const unsigned long TIEMPO_MENSAJE = 2000;

// Estados de los sensores
bool entradaOcupada = false;
bool salidaOcupada = false;

// Control de mensajes
unsigned long tiempoMensaje = 0;
bool mostrandoEvento = false;


// =================================================
// MEDIR DISTANCIA
// =================================================
float medirDistancia(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  long duracion = pulseIn(echoPin, HIGH, 25000);

  if (duracion == 0)
  {
    return 999;
  }

  float distancia = duracion * 0.0343 / 2;

  return distancia;
}

// =================================================
// DETECTAR PERSONA
// =================================================
bool personaDetectada(float distancia)
{
  return distancia > 2 &&
         distancia < DISTANCIA_DETECCION;
}




// =================================================
// MOSTRAR ESTADO DEL BUS
// =================================================
void mostrarEstadoBus()
{
  lcd.clear();

  // ===============================
  // BUS VACIO
  // ===============================
  if (puestosDisponibles == CAPACIDAD_MAXIMA)
  {
    lcd.setCursor(0, 0);
    lcd.print("BUS VACIO");

    lcd.setCursor(0, 1);
    lcd.print("Puestos: ");
    lcd.print(puestosDisponibles);
  }

  // ===============================
  // BUS LLENO
  // ===============================
  else if (puestosDisponibles == 0)
  {
    lcd.setCursor(0, 0);
    lcd.print("BUS LLENO");

    lcd.setCursor(0, 1);
    lcd.print("Puestos: 0");
  }

  // ===============================
  // BUS CON PASAJEROS
  // ===============================
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("Puestos:");

    lcd.setCursor(9, 0);
    lcd.print(puestosDisponibles);

    lcd.setCursor(0, 1);
    lcd.print("Disponibles");
  }
}


// =================================================
// MOSTRAR EVENTO
// =================================================
void mostrarEvento(const char* mensaje)
{
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(mensaje);

  lcd.setCursor(0, 1);
  lcd.print("Puestos: ");
  lcd.print(puestosDisponibles);

  mostrandoEvento = true;
  tiempoMensaje = 1000;
}


// =================================================
// SETUP
// =================================================
void setup()
{
  Serial.begin(9600);

  // Sensor entrada
  pinMode(TRIG_ENTRADA, OUTPUT);
  pinMode(ECHO_ENTRADA, INPUT);

  // Sensor salida
  pinMode(TRIG_SALIDA, OUTPUT);
  pinMode(ECHO_SALIDA, INPUT);

  // LCD
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("CONTROL BUS");

  lcd.setCursor(0, 1);
  lcd.print("Iniciando...");

  delay(1500);

  mostrarEstadoBus();
}


// =================================================
// LOOP
// =================================================
void loop()
{
  // ===============================================
  // MEDIR SENSOR DE ENTRADA
  // ===============================================
  float distanciaEntrada =
    medirDistancia(TRIG_ENTRADA, ECHO_ENTRADA);

  delay(30);

  // ===============================================
  // MEDIR SENSOR DE SALIDA
  // ===============================================
  float distanciaSalida =
    medirDistancia(TRIG_SALIDA, ECHO_SALIDA);


  // ===============================================
  // DETECTAR ENTRADA
  // ===============================================

bool detectadoEntrada = personaDetectada(distanciaEntrada);



  if (detectadoEntrada && !entradaOcupada)
  {
    entradaOcupada = true;

    if (puestosDisponibles > 0)
    {
      puestosDisponibles--;

      Serial.println("PERSONA ENTRO");

      Serial.print("Puestos disponibles: ");
      Serial.println(puestosDisponibles);

      mostrarEvento("Persona entro");
      delay(1000);
    }
    else
    {
      Serial.println("BUS LLENO");

      mostrarEvento("BUS LLENO");
    }
  }

  // Sensor de entrada libre
  if (!detectadoEntrada)
  {
    entradaOcupada = false;
  }


  // ===============================================
  // DETECTAR SALIDA
  // ===============================================
bool detectadoSalida = personaDetectada(distanciaSalida);


  if (detectadoSalida && !salidaOcupada)
  {
    salidaOcupada = true;

    if (puestosDisponibles < CAPACIDAD_MAXIMA)
    {
      puestosDisponibles++;

      Serial.println("PERSONA SALIO");

      Serial.print("Puestos disponibles: ");
      Serial.println(puestosDisponibles);

      mostrarEvento("Persona salio");
      delay(1000);
    }
  }

  // Sensor de salida libre
  if (!detectadoSalida)
  {
    salidaOcupada = false;
  }


  // ===============================================
  // VOLVER A MOSTRAR ESTADO
  // ===============================================
  if (mostrandoEvento)
  {
    if (millis() - tiempoMensaje >= TIEMPO_MENSAJE)
    {
      mostrandoEvento = false;

      mostrarEstadoBus();
    }
  }

  delay(50);
}
