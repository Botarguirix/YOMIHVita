# Reglas compartidas para asistentes

- Leer `README.md`, `docs/HALLAZGOS.md` y `docs/ESTADO.md` antes de cambiar codigo.
- Respetar la especificacion de `docs/CODEX_TAREA_FixedMath.md` para la fase FixedMath.
- No leer, copiar, modificar ni subir `original/` o `tools/harness/run/`. No subir bibliotecas originales, recursos comerciales, secretos ni herramientas descargadas.
- No modificar `tests/golden/` ni `tools/tbfg_compare.py` para adaptar los resultados a una implementacion.
- Mantener el nucleo portable a ARM32, C++17, sin dependencias de Godot; seguir las restricciones numericas de la especificacion.
- Usar ramas `codex/<tarea>` o `claude/<tarea>` y cambios acotados. No sobrescribir trabajo ajeno.
- No trabajar simultaneamente en el mismo checkout; usar clones separados si hay concurrencia.
- Documentar comandos de validacion, resultados y pendientes en `docs/ESTADO.md`. Distinguir pruebas ejecutadas de las pendientes y no afirmar pruebas en Vita que no se realizaron.
- Revisar `git diff --cached --stat` y los archivos preparados antes de publicar.
