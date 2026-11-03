// Aritmética I48F16 determinista, réplica bit a bit de la tbfg original:
//  - fixed 1.19.0 (FixedI64<U16>): parseo, formato, mul, div.
//  - fixed-sqrt 0.2.5: raíz con 8 bits de precisión.
//  - fixed_trigonometry 0.4.3: sin, cos, atan2.
// C++17 portable (ARM32 incluido): sin __int128, sin coma flotante, sin excepciones.
// Referencia ejecutable de cada algoritmo: native/proto/fixedmath_ref.py.
#pragma once

#include <cstdint>
#include <string>

namespace tbfg {

typedef int64_t fx; // bits I48F16

const int FRAC = 16;
const fx ONE = fx(1) << FRAC;
const fx PI = 205887;      // fixed::consts::PI truncado
const fx PI_HALF = 102943; // PI / 2
const fx TAU = 2 * PI;
const fx ATAN_C = 18432;   // 0.28125

// Estado de error: el original hace panic (división por cero, raíz de negativo,
// texto inválido) y Godot recibe null.
struct Status {
	bool ok = true;
	void fail() { ok = false; }
};

// Conversión con texto, idéntica a FromStr / Display de fixed 1.19.
fx parse(const std::string &s, Status &st);
std::string format(fx v);

// Operaciones básicas (desbordamiento con wrapping de i64).
fx from_int(int64_t n);          // n << 16 sin comportamiento indefinido
fx add(fx a, fx b);
fx sub(fx a, fx b);
fx mul(fx a, fx b);              // redondeo hacia -inf
fx div(fx a, fx b, Status &st);  // truncamiento hacia cero
fx div_int(fx a, int64_t n);     // fixed / entero positivo, truncando
fx asr(fx a, int n);             // desplazamiento aritmético (floor)
fx powi(fx b, int64_t e);

int64_t floor_int(fx a);
int64_t ceil_int(fx a);
int64_t round_int(fx a);         // empates lejos de cero

fx sqrt(fx a, Status &st);

fx sin(fx x);
fx cos(fx x);
fx atan2(fx y, fx x, Status &st);
fx fmod(fx a, fx b);
fx angle_dist(fx a, fx b);

} // namespace tbfg
