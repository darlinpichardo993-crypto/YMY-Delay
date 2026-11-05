# YMY DELAY — BY ORYMY
**VINTAGE BACHATA DELAY · YMY RECORDS**
*"THE CLASSIC BACHATA ECHO."*

Delay vocal corto, cálido y discreto inspirado en la bachata romántica de los 90 y 2000.
Plugin VST3 + Standalone para Windows 10/11 x64, hecho con C++, JUCE 8 y CMake.
Compatible con FL Studio 21, 24 y 25. Versión 1.0.0.

---

## 1. Cómo compilarlo en GitHub (sin instalar nada)

1. Crea un repositorio nuevo en GitHub (vacío).
2. **Extrae el ZIP** y sube el **contenido** de la carpeta `YMY-DELAY` al repositorio, de modo que
   `CMakeLists.txt` quede en la raíz. Con Git:
   ```bash
   cd YMY-DELAY
   git init
   git add .
   git commit -m "YMY DELAY 1.0.0"
   git branch -M main
   git remote add origin https://github.com/TU_USUARIO/YMY-DELAY.git
   git push -u origin main
   ```
   (La carpeta oculta `.github` debe subirse; con Git o GitHub Desktop se sube sola.)
3. Entra en la pestaña **Actions** → workflow **Windows Build**. Se ejecuta solo en cada push;
   también puedes lanzarlo con **Run workflow**.
4. Cuando termine (la primera vez tarda unos 10–15 min), baja los archivos desde **Artifacts**:
   - `YMY-DELAY-ZIP` → `YMY DELAY.zip` (todo incluido)
   - `YMY-DELAY-Setup` → `YMY DELAY Setup.exe`
   - `YMY-DELAY-VST3-Standalone` → `YMY DELAY.vst3` y `YMY DELAY.exe`
5. Para publicar una *Release* automática, crea una etiqueta: `git tag v1.0.0 && git push origin v1.0.0`.

## 2. Instalación

**Con instalador:** ejecuta `YMY DELAY Setup.exe`. Instala el VST3 en
`C:\Program Files\Common Files\VST3` (puedes elegir otra carpeta), instala el Standalone y
crea accesos directos opcionales. Se desinstala desde *Configuración → Aplicaciones*.
Solo toca la carpeta `YMY DELAY.vst3`; no sobrescribe archivos de otros plugins.

**Manual:** copia la carpeta `YMY DELAY.vst3` a `C:\Program Files\Common Files\VST3`.

En FL Studio: *Options → Manage plugins → Find installed plugins*, y busca **YMY DELAY** en
el selector de efectos.

## 3. Compilar en local (opcional)

Requisitos: Visual Studio 2022 (carga de trabajo C++), CMake ≥ 3.22 y Git.
```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```
Resultados en `build/YMYDelay_artefacts/Release/VST3` y `.../Standalone`.

## 4. Host Tempo Sync

- Lee el BPM del transporte del host con `AudioProcessor::getPlayHead()` en cada bloque de audio.
- Con **SYNC** activo: tiempo = `60000 / BPM × duración de la división`. Sigue cambios de tempo en
  tiempo real y no hace falta escribir el BPM.
- Si el host deja de enviar BPM un instante, usa el último BPM válido. Sin host (Standalone) usa **BPM**.
- Divisiones: 1/1, 1/2, 1/4, 1/8, 1/16, 1/32 (rectas), con **Dotted** y **Triplet** desde 1/2.
- **FREE** (SYNC apagado): el knob TIME controla el tiempo en ms (1–500) y no depende del BPM.
- **TAP**: sin BPM del host fija el BPM manual; con host fija el tiempo FREE y apaga SYNC.
- Los cambios de tiempo se suavizan con una rampa de 50 ms (sin clicks); el eco hace un pequeño
  glide de tono, como una cinta.
- El motor llega a 500 ms. Si una división no cabe a ese tempo (p. ej. 1/2 a 100 BPM) se divide
  entre 2 hasta que quepa, y el display TIME muestra un `*`.

## 5. Controles

| Control | Función |
|---|---|
| TIME | Tiempo del delay en modo FREE (en SYNC lo gobierna NOTE + BPM) |
| FEEDBACK | Repeticiones (0–90 %) |
| MIX | Mezcla seco/húmedo |
| TONE | Oscurece cada repetición (1,2–12 kHz) |
| WIDTH | Amplitud estéreo del eco (no actúa en MONO) |
| DUCK | Baja el eco mientras suena la voz |
| SYNC / NOTE / TAP | Sincronización, división y tap tempo |
| MONO / STEREO / PING-PONG | Modo del delay |
| ANALOG | Saturación suave + modulación sutil |
| FILTER, HP, LP | Filtros pasa-altos y pasa-bajos sobre el eco |
| OUT | Nivel de salida (−24 a +12 dB) |
| ON | Activa/desactiva el efecto (bypass sin clicks) |
| GUI SCALE | 75 %, 90 %, 100 %, 110 %, 125 %, 150 %, 175 %, 200 % (se guarda con el proyecto) |

## 6. Presets

20 presets de fábrica (BACHATA, VOCALS, SPECIAL). Los presets de usuario se pueden **guardar,
renombrar y eliminar** desde la barra superior y se guardan en
`%APPDATA%\ORYMY\YMY DELAY\Presets`.

## 7. Estructura del proyecto

```
YMY-DELAY/
├─ CMakeLists.txt        README.md   LICENSE   .gitignore
├─ Source/
│  ├─ PluginProcessor.*  PluginEditor.*
│  ├─ DSP/               DelayEngine.h  TempoSync.h
│  ├─ Parameters/        Parameters.h
│  ├─ Presets/           PresetManager.*
│  └─ Utilities/         YMYLookAndFeel.h  Version.h
├─ Resources/  Installer/YMY-DELAY.iss  Packaging/
└─ .github/workflows/windows-build.yml
```

## 8. Notas

- **Estado de verificación:** el código fue escrito sin compilador disponible en el entorno donde se
  generó. La primera ejecución del workflow es la primera compilación real. Si falla, copia el
  error del log de Actions para corregirlo. El paso *pluginval* valida el VST3 (nivel 5) y no
  bloquea la publicación de los archivos si da avisos.
- **Licencia de JUCE:** JUCE se distribuye bajo AGPLv3 o licencia comercial. Para vender o
  distribuir binarios sin publicar el código necesitas una licencia comercial de JUCE.
- Los cambios de modo (MONO/STEREO/PING-PONG) con el audio sonando pueden producir un pequeño
  salto en las repeticiones ya grabadas en el buffer.
