# YOMI Hustle → PS Vita: hallazgos de la prueba de concepto

## Juego original
- Godot **3.5.1** (custom build `6fed1ffa3`, con GodotSteam), `.pck` v1 sin cifrar, 5452 archivos.
- Contenido extraído en `original/pck/` con `tools/extract_pck.ps1`.
- Uso exclusivo local/personal: el `LICENSE.txt` prohíbe redistribuir sin permiso de Ian Sly.

## Librería nativa `tbfg` (bloqueo principal)
- Rust, crate `turn_based_fighting_game_lib`, godot-rust (`gdnative-core` 0.11.3).
- Dependencias relevantes: `fixed` 1.19.0, `fixed-sqrt` 0.2.5.
- Módulos: `utils.rs` (FixedMath), `char.rs` (FGObject), `obj.rs` (BaseObject),
  `point.rs` (FixedVec2), `methods.rs` (NativeMethods).
- Binario **no stripeado**: tiene símbolos Rust, útil para desensamblar.
- Desde GDScript solo la usan `obj/BaseObj.gd` y `mechanics/SweptHitbox.gd`.

### Formato numérico
`FixedI64<U16>` (I48F16): 16 bits de fracción, resolución 1/65536.
- Los valores viajan como `String` decimales y se devuelven con el formato
  mínimo de `fixed` (`div("1","3")` → `"0.33333"`).
- La raíz cuadrada es la de `fixed-sqrt` y es aproximada: `vec_len("1","1")` → `"1.41406"` (= 92672/65536).
- `normalized_vec_times("3","4","10")` → `5.99991, 7.99988` (no exacto): hay que replicar el orden de operaciones.
- Trigonometría propia: `angle_to_vec("1")` → `0.5403, 0.84148`; `vec_to_angle("1","1")` → `"0.7903"` (no π/4 exacto).

### API (extraída del binario con el arnés)
Ver `tools/harness/signatures_raw.txt`. Resumen:
- **FixedMath** (38 métodos): aritmética (`add sub mul div abs sign powu`),
  redondeo (`floor ceil round` → int), comparación (`lt gt le ge eq` → bool),
  vectores (`vec_*`, `normalized_vec*`, `rotate_vec`, `angle_to_vec`, `vec_to_angle`)
  → `{x, y}`, interpolación (`lerp_string lerp_int lerp_angle angle_dist`),
  `deg2rad`, `int_map`, `get_x_intercept`.
- **FGObject** (41 métodos): estado físico (`position_x/y` int, `vel_x/y` String,
  `facing`, `grounded`, `snap_to_ground`, `position_float`), fuerzas, fricción,
  gravedad, pushback, `get_data()`, `copy_to(target)`.
- **NativeMethods.copy_state(from_state: Node, to_state: Node)**.

## Arnés de referencia (`tools/harness/`)
Ejecuta el `.exe` original con un `.pck` propio que carga `tbfg.dll`:
```
python tools\make_pck.py tools\harness\project tools\harness\run\harness.pck
tools\harness\run\YourOnlyMoveIsHUSTLE.exe --main-pack harness.pck --no-window
```
Escenas: `main.gd` (API), `probe.gd` (pruebas numéricas), `signatures.gd` (firmas).
Sirve como **oráculo**: la reimplementación en C++ para Vita debe dar resultados
idénticos bit a bit en lotes de pruebas generados aquí.

## Proyecto descompilado (`original/project/`)
GDRE Tools v2.7.4 (`tools/gdre/`): 502/502 scripts descompilados y 2538 recursos
convertidos (14 con pérdida). Se abre con Godot 3.5.1 + GodotSteam.

### Cómo usa el juego la librería nativa
- **FixedMath** está por todas partes: ~1300 llamadas en **118 scripts**
  (`mul` 335, `round` 137, `gt` 74, `lt` 68, `normalized_vec_times` 61, `div` 60…),
  siempre a través de `host.fixed` (`var fixed = FixedMath.new()` en `obj/BaseObj.gd:79`).
  → La reimplementación debe conservar **la misma clase, los mismos nombres de método y
  la misma interfaz de String**, para no tocar 118 scripts.
- **FGObject** solo se usa como `chara` dentro de `obj/BaseObj.gd` (envoltorio fino,
  líneas 227-913): el resto del juego llama a métodos de `BaseObj`.
- **NativeMethods.copy_state(from, to)** clona estado copiando las propiedades listadas
  en `property_list` (`ObjectState.gd:410-412`, `mechanics/Hitbox.gd:184`). Es la ruta
  crítica de la vista previa fantasma (se clona todo el estado de la partida).
- Coste en Vita: cada operación convierte texto → I48F16 → texto. En C++ nativo
  es asumible, pero es el primer punto a medir con el profiler.

## Siguientes pasos
1. Desensamblar las funciones de `tbfg.so` (con símbolos) para reproducir sqrt, trigonometría y las fuerzas.
2. Escribir el módulo C++ `tbfg` para Godot 3.5 y validarlo contra el arnés.
3. Compilar godot-vita con el módulo y medir rendimiento en hardware.
