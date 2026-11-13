# Brazalete de Salud — demo web (Vercel + Neon)

Pagina que muestra BPM, SpO2, temperatura y movimiento del brazo. El ESP32 envia datos por HTTPS
y la pagina los lee de una base Postgres (Neon). Tiene un **simulador** para demostrar sin hardware.

## Ya hecho
- Base de datos Neon: proyecto **brazalete-salud-demo** con las tablas `devices` y `readings`.

## Pasos para publicarla (5 minutos)
1. En Neon (console.neon.tech) abrir el proyecto `brazalete-salud-demo` -> **Connect** -> copiar la
   connection string (pooled).
2. En Vercel: **Add New -> Project**. Subir esta carpeta (`vercel` por consola, o importar desde GitHub).
   Es un proyecto sin framework: la carpeta `public` es el sitio y `api/` son las funciones.
3. En Vercel -> Settings -> Environment Variables, crear:
   - `DATABASE_URL` = la connection string de Neon
   - `INGEST_KEY`   = una clave inventada por vos (la usa el ESP32)
4. Deploy. Abrir la URL: boton **Iniciar simulador** -> ya se ven datos en vivo.
   (Si la API no responde, el simulador sigue funcionando en modo local, sin base.)

## API
| Metodo | Ruta | Uso |
|---|---|---|
| POST | `/api/ingest` | ESP32. Header `x-api-key: INGEST_KEY`. JSON: `device, bpm, spo2, temp, motion, ax, ay, az, finger, bat` |
| POST | `/api/demo` | Simulador del navegador (sin clave, guarda como `simulated`) |
| GET | `/api/readings?device=ID&limit=60` | Lecturas para el dashboard |

Prueba rapida (reemplazar URL y clave):
`curl -X POST https://TU-PROYECTO.vercel.app/api/ingest -H "x-api-key: TU_CLAVE" -H "content-type: application/json" -d '{"bpm":72,"spo2":98,"temp":36.6,"motion":10,"finger":true}'`

## Firmware (`firmware/`)
PlatformIO, ESP32. Completar WiFi, `SERVER_URL` y `API_KEY` (la misma `INGEST_KEY`) en `src/main.cpp`.
Usa DS18B20 en **IO4** (como en el esquematico V2; el firmware viejo usaba el pin 26).
No se compilo ni probo en hardware real: revisar nombres de librerias al primer build.
