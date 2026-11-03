# YOMIHVita

Espacio de trabajo para investigar y desarrollar la compatibilidad de **Your Only Move Is HUSTLE** con PS Vita, colaborando con Claude y Codex.

El repositorio contiene documentacion, herramientas propias y casos de referencia. No incluye el juego original ni sus recursos, ejecutables o bibliotecas comerciales. No es un port jugable: la implementacion nativa sigue pendiente.

## Por donde empezar

1. Leer `AGENTS.md` (reglas compartidas) y `docs/HALLAZGOS.md` (investigacion existente).
2. Consultar `docs/ESTADO.md` para conocer la tarea actual.
3. Para implementar FixedMath, seguir `docs/CODEX_TAREA_FixedMath.md`.

## Estructura

- `docs/`: hallazgos, especificaciones y relevos entre asistentes.
- `tools/`: extraccion, empaquetado, comparador y arnes de referencia.
- `tests/golden/`: resultados de referencia; no alterarlos para hacer pasar pruebas.
- `native/`: ubicacion prevista para la implementacion C++17.
- `original/`: archivos privados de una copia del usuario, excluidos de Git.

## Trabajar con Claude y Codex

Usar una rama por tarea: `claude/<tarea>` o `codex/<tarea>`. Antes de comenzar, actualizar `main` con Fetch/Pull en GitHub Desktop y crear la rama. No editar los mismos archivos simultaneamente en una misma carpeta; usar clones separados si ambos trabajan a la vez.

Al terminar, crear un commit, publicar la rama y abrir un pull request. El otro asistente puede revisar el diff. Registrar lo realizado, pruebas y pendientes en `docs/ESTADO.md` antes del relevo.

Para Claude: abrir esta carpeta con Claude Code y pedirle que lea `CLAUDE.md`. Si se usa otra modalidad de Claude, darle acceso al repositorio mediante su integracion con GitHub. Crear este repositorio no configura automaticamente esa conexion.

## Validacion prevista

Cuando exista el CLI nativo:

```text
python tools/tbfg_compare.py tests/golden/fixedmath_golden.tsv -- native/build/tbfg_cli.exe
```

Los pasos locales del arnes se describen en `docs/HALLAZGOS.md`; requieren los binarios propios del usuario en las rutas ignoradas.
