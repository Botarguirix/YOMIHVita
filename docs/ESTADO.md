# Estado y relevo

## Estado actual

- FixedMath reimplementado en C++ (`native/`): coincide bit a bit con la tbfg original en
  los 37 métodos usados por el juego (55501/57000 casos golden; solo falla `get_x_intercept`, sin uso).
- Prototipo de referencia en Python (`native/proto/fixedmath_ref.py`) y oráculo contra la DLL original (`tools/oracle.py`).
- No se ha compilado para ARM32/Vita ni probado en consola.

## Último relevo

- **Asistente y rama:** Claude, `claude/fixedmath`.
- **Objetivo y archivos:** fase 1 de `CODEX_TAREA_FixedMath.md`.
  - Nuevos: `native/tbfg_core/fixed16.{h,cpp}`, `native/tbfg_cli/main.cpp`, `native/build.bat`,
    `native/NOTAS.md`, `native/proto/fixedmath_ref.py`, `tools/oracle.py`,
    `tools/harness/project/oracle.{gd,tscn}`.
  - Modificado: `.gitignore` (excluye `tools/w64devkit*`).
  - Sin cambios en `tests/golden/` ni en `tools/tbfg_compare.py`.
- **Comandos y resultados:**
  - `native\build.bat` con w64devkit 2.10.1 (GCC 16.2, `-Wall -Wextra -pedantic`): sin avisos.
  - `python tools/tbfg_compare.py tests/golden/fixedmath_golden.tsv -- native/build/tbfg_cli.exe`:
    55501/57000 en 0,5 s. Los 1499 fallos son todos de `get_x_intercept`.
  - El prototipo Python da el mismo resultado con el mismo comando (`-- python native/proto/fixedmath_ref.py`).
- **Pendientes o bloqueos:**
  - `get_x_intercept` sin identificar (no se usa en el juego).
  - Falta compilación cruzada ARM32 (VitaSDK) para verificar la portabilidad.
  - MSVC sin probar.
- **Próximo paso:** fase 2. Generar casos golden de `FGObject` (secuencias de operaciones
  sobre el estado físico) con el arnés y reimplementarlo; después, el módulo de Godot 3.5 que exponga
  `FixedMath`, `FGObject` y `NativeMethods` con los mismos nombres.

## Formato para cada relevo

- Asistente y rama:
- Objetivo y archivos modificados:
- Comandos ejecutados y resultados:
- Pendientes o bloqueos:
- Proximo paso:
