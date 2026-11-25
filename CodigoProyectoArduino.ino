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
//Valor centinela, NO es una distancia  real: indica que el sensor no recibio eco
//(nada al alcance o lectura fallida). Es mayor que DISTANCIA_DETECCION, asi que 
//nunca se confunde con una persona.
const float LECTURA_INVALIDA = 999;


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


  //Timetout de 25000 us (25 ms): equivale a ~4.3 m de ida y vuelta, que es mas que 
  // el alcance util del HC-SR04 ( ~4 m). Si no hay eco antes, se descarta la lectura
  //y evitamos bloquear el loop() esperando un pulso que no va a llegar.
  long duracion = pulseIn(echoPin, HIGH, 25000);

  if (duracion == 0)
  {
    return LECTURA_INVALIDA;
  }

  // 0.0343 cm/us es la velocidad del sonido (343 m/s). Se divide entre 2 porque
  //la duración mide el viaje de ida y vuelta, y solo nos interesa la ida.

  float distancia = duracion * 0.0343 / 2;

  return distancia;
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

  // se ignoran lecturas <= 2cm: es el minimo que mide el HC-SR04, por debajo 
  //el eco es poco confiable y genera falsos positivos. LECTURA_INVALIDA queda
  //fuera por ser mayor que DISTANCIA_DETECCION.
  
  bool detectadoEntrada =
    distanciaEntrada > 2 &&
    distanciaEntrada < DISTANCIA_DETECCION;


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
  bool detectadoSalida =
    distanciaSalida > 2 &&
    distanciaSalida < DISTANCIA_DETECCION;


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
