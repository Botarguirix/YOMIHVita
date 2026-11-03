# Notas de la implementación FixedMath (fase 1)

## Resultado
`python tools/tbfg_compare.py tests/golden/fixedmath_golden.tsv -- native/build/tbfg_cli.exe`
→ **55501/57000**: los 37 métodos que usa el juego coinciden al 100 %; solo falla
`get_x_intercept` (1/1500), que **ningún script del juego llama**.

## Estructura
- `tbfg_core/fixed16.{h,cpp}`: núcleo I48F16. C++17, sin Godot, sin `__int128`,
  sin coma flotante y sin excepciones (los errores van en `tbfg::Status`).
- `tbfg_cli/main.cpp`: protocolo del comparador y correspondencia método → núcleo.
  El futuro módulo de Godot repetirá esta correspondencia devolviendo `Variant`.
- `proto/fixedmath_ref.py`: referencia en Python con la misma lógica; sirvió para
  descubrir los algoritmos y cumple el mismo protocolo.
- `build.bat`: compila con g++ (probado con w64devkit 2.10.1, GCC 16.2) o con MSVC (sin probar).

## Algoritmos reproducidos (origen identificado en el binario)
- **Parseo** (`fixed` 1.19 `u16::dec_str_frac_to_bin`): bloques de 6 dígitos y redondeo
  al más cercano con empates a par. Peculiaridad: con más de 6 decimales, si el texto se
  agota coincidiendo con el punto medio, cuenta como empate
  (`-796153.8233871` → `…970`, no `…969`).
- **Formato** (`fixed` 1.19 `write_frac_dec`): 5 dígitos como máximo; se corta cuando el
  resto queda a menos de medio ulp **o a menos de 10/65536** del valor (por eso imprime
  `3266.8` y no `3266.79999`); redondeo con empates a par.
- **mul**: producto de 128 bits >> 16, redondeo hacia −∞. **div**: truncamiento hacia cero.
- **sqrt** (`fixed-sqrt`): `isqrt(bits) << 8`, es decir, solo 8 bits fraccionarios.
- **sin/cos/atan2** (`fixed_trigonometry` 0.4.3, MIT/Apache):
  - `sin`: Taylor de grado 9 con un solo plegado en ±π/2. Diverge para |x| > ~4,7, igual que el original.
  - `cos(x) = sin(wrap_phase(x + π/2))`.
  - `atan2`: `xy / (x² + 0,28125·y²)` por octantes. Da `null` si el denominador se trunca a 0.
  - `π` = 205887 (truncado); `π/2` = 102943.
- `lerp_string` = `a·(1−f) + b·f`; `lerp_angle` y `angle_dist` siguen la fórmula de Godot
  con `fmod` truncado; `deg2rad` = `(n·π) / 180` truncado.

## Pendiente
- `get_x_intercept`: algoritmo no identificado (casos en `tools/oracle.py`). No bloquea nada
  porque el juego no lo usa; si hace falta, desensamblar `utils.rs` en `tbfg.so`.
- **Sin compilar para ARM32/Vita.** La portabilidad se ha cuidado por diseño
  (enteros de 64 bits, sin UB en desplazamientos), pero no se ha verificado con `arm-vita-eabi-gcc`.
- Fase 2: `FGObject` y `NativeMethods.copy_state`, y el envoltorio como módulo de Godot 3.5.
