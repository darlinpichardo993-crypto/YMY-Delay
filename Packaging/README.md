# Packaging

El empaquetado lo genera el workflow `.github/workflows/windows-build.yml`:

| Archivo | Contenido |
|---|---|
| `YMY DELAY.zip` | `YMY DELAY.vst3`, `YMY DELAY.exe`, `YMY DELAY Setup.exe`, README y LICENSE |
| `YMY DELAY Setup.exe` | Instalador (script en `Installer/YMY-DELAY.iss`) |
| `YMY DELAY.vst3` | Plugin VST3 (carpeta/bundle de Windows) |
| `YMY DELAY.exe` | Aplicacion Standalone |
