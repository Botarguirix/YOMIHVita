# Tarea para Codex: reimplementar `FixedMath` en C++ (fase 1)

## Contexto
Estamos portando *Your Only Move Is HUSTLE* (Godot 3.5.1) a PS Vita. El juego depende de
una librería nativa cerrada en Rust (`tbfg`), que solo existe para x86-64. Su clase
`FixedMath` hace matemática de punto fijo determinista y se usa ~1300 veces en 118
scripts. Hay que reproducirla **bit a bit** en C++ portable.
Antes de empezar, lee `docs/HALLAZGOS.md`.

## Prohibido
- Leer, copiar o subir `original/`, `tools/harness/run/` o cualquier archivo del juego.
  Son material con copyright. No los necesitas.
- Modificar `tests/golden/` o `tools/tbfg_compare.py`: son el criterio de aceptación.
- Instalar compiladores u otro software sin preguntarle antes al usuario.
  Ahora mismo no hay ninguno instalado: sugiere w64devkit (MinGW) o MSVC Build Tools.

## Entregables
1. `native/tbfg_core/`: biblioteca C++17 **sin dependencias de Godot**.
   - Tipo de punto fijo **I48F16**: `int64_t` con 16 bits de fracción, como `fixed::FixedI64<U16>`.
   - Debe compilar para **ARM32** (PS Vita, `arm-vita-eabi-gcc`): sin `__int128`, sin
     intrínsecos x86, sin `long double`. Las multiplicaciones y divisiones que
     necesiten 128 bits se hacen con aritmética manual de 64×64→128 bits.
   - Prohibido usar `float` o `double` en los cálculos, para que el resultado sea igual en cualquier CPU.
2. `native/tbfg_cli/`: un ejecutable de pruebas `tbfg_cli` que lee de stdin y escribe en stdout (protocolo abajo).
3. Un script de compilación (`native/build.bat` o CMake) que deje el binario en `native/build/tbfg_cli.exe`.
4. `native/NOTAS.md`: decisiones tomadas, algoritmos reproducidos y lo que no coincida.

## Protocolo del CLI
- Entrada: una línea por caso, `metodo<TAB>arg1<TAB>arg2...`.
- Salida: **exactamente una línea por caso**. Si hay un error, la línea es `<null>`.
- Cómo codificar el resultado:
  - String: el decimal tal cual.
  - int: en decimal.
  - bool: `true` o `false`.
  - Vector `{x,y}`: `x,y`.
  - División por cero: `<null>`. El original hace panic y devuelve null.
- El CLI debe procesar las 57.000 líneas en pocos segundos (sin flush por línea).

## API que hay que reproducir (los nombres de parámetro son los originales)
Los parámetros String son decimales que se convierten a I48F16. Lo que se devuelve como String
es un I48F16 formateado.

| Método | Parámetros | Devuelve |
|---|---|---|
| `add sub mul div` | x, y | String |
| `abs` | n | String |
| `sign` | a | int |
| `floor ceil round` | value | int |
| `powu` | n:String, e:int | String |
| `lt gt le ge eq` | x, y | bool |
| `deg2rad` | n | String |
| `angle_to_vec` | t | vector |
| `vec_to_angle` | x, y | String |
| `rotate_vec` | x, y, t | vector |
| `angle_dist` | angle1, angle2 | String |
| `lerp_angle` | from, to, weight | String |
| `lerp_string` | a, b, f | String |
| `lerp_int` | a:String, b:String, max:int, f:int | ver casos |
| `int_map` | value, istart, istop, ostart, ostop (todos int) | int |
| `vec_len vec_len_squared` | x, y | String |
| `vec_dist vec_dist_squared vec_dot` | x1, y1, x2, y2 | String |
| `vec_add vec_sub` | x1, y1, x2, y2 | vector |
| `vec_mul vec_div vec_scale` | x, y, rhs/f | vector |
| `vec_round` | x, y | vector de int |
| `normalized_vec` | x, y | vector |
| `normalized_vec_times` | x, y, magnitude | vector |
| `get_x_intercept` | x1, y1, x2, y2 | String |

Si un método no está claro (por ejemplo `lerp_int` o `get_x_intercept`), dedúcelo de los casos.

## Pistas
- El original usa los crates `fixed` 1.19.0 y `fixed-sqrt` 0.2.5, ambos de código
  abierto (MIT/Apache). **Lee su código fuente** para copiar exactamente:
  - **Parseo** (`FromStr`): cómo redondea los decimales que no caben en 16 bits.
    Ejemplo: `add("6.28318","534930.9869")` → `534937.27007`.
  - **Formato** (`Display`): imprime los decimales mínimos que identifican el valor.
    Ejemplos: `div("1","3")` → `0.33333`, `div("1","65536")` → `0.00002`.
  - **Raíz cuadrada** de `fixed-sqrt`: `vec_len("1","1")` → `1.41406` (= 92672/65536).
- La trigonometría (`angle_to_vec`, `vec_to_angle`, `rotate_vec`, `deg2rad`, `lerp_angle`,
  `angle_dist`) es propia de `tbfg` y no conocemos el algoritmo. Prueba lo más probable:
  constantes de π en I48F16 y series de Taylor o CORDIC. Si no consigues que coincida al 100 %,
  documenta el error máximo en `NOTAS.md` y sigue: Claude desensamblará esas funciones.
- Orden sugerido: parseo y formato → `add/sub/abs/comparaciones` → `mul/div/powu` →
  redondeos → `sqrt` y vectores → interpolaciones → trigonometría.

## Criterio de aceptación
```
python tools/tbfg_compare.py tests/golden/fixedmath_golden.tsv -- native/build/tbfg_cli.exe
```
Debe terminar con código 0 (57.000/57.000). Si no llegas al 100 %, deja los fallos
acotados a métodos concretos y explícalos en `NOTAS.md`.

## Qué entregar al terminar (para la revisión)
1. La salida completa del comparador.
2. La lista de archivos creados.
3. Los métodos que no coinciden y la hipótesis de por qué.

No hace falta resumir el código. La revisión se hará ejecutando el comparador y leyendo solo lo que falle.
