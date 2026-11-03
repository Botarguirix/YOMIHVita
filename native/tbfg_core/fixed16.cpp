#include "fixed16.h"

namespace tbfg {

namespace {

typedef uint64_t u64;

inline fx wrap(u64 v) { return fx(v); }
inline u64 uabs(fx a) { return a < 0 ? u64(0) - u64(a) : u64(a); }
inline fx neg(fx a) { return wrap(u64(0) - u64(a)); }

// Producto 64x64 -> 128 bits sin signo, en partes de 32 bits.
void umul128(u64 x, u64 y, u64 &hi, u64 &lo) {
	u64 x0 = x & 0xFFFFFFFFu, x1 = x >> 32;
	u64 y0 = y & 0xFFFFFFFFu, y1 = y >> 32;
	u64 p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, p11 = x1 * y1;
	u64 mid = (p00 >> 32) + (p01 & 0xFFFFFFFFu) + (p10 & 0xFFFFFFFFu);
	lo = (p00 & 0xFFFFFFFFu) | (mid << 32);
	hi = p11 + (p01 >> 32) + (p10 >> 32) + (mid >> 32);
}

bool is_digit(char c) { return c >= '0' && c <= '9'; }

// fixed 1.19 u16::dec_str_frac_to_bin: bloques de 6 dígitos. Devuelve false si el
// redondeo llega a 1.0 (acarreo a la parte entera). Con más de 6 dígitos, una
// entrada que se agota coincidiendo con el punto medio cuenta como empate.
bool parse_frac(const std::string &d, u64 &out) {
	size_t n = d.size();
	if (n <= 6) {
		u64 val = 0;
		for (size_t i = 0; i < 6; i++)
			val = val * 10 + (i < n ? u64(d[i] - '0') : 0);
		u64 numer = (val << 11) + 15625;
		if ((numer >> 16) >= 31250)
			return false;
		u64 q = numer / 31250;
		if (numer % 31250 == 0 && (q & 1))
			q--;
		out = q;
		return true;
	}
	u64 prefix = 0;
	for (size_t i = 0; i < 6; i++)
		prefix = prefix * 10 + u64(d[i] - '0');
	u64 floor = (prefix << 11) / 31250;
	u64 boundary = floor;
	bool add5 = true, tie = true;
	for (size_t i = 0; i < n; i++) {
		u64 b = u64(d[i] - '0');
		if (!add5 && boundary == 0) {
			tie = false;
			break;
		}
		u64 p = boundary * 10;
		u64 bdigit = p >> 16;
		boundary = p & 0xFFFF;
		if (add5) {
			boundary += 5;
			if (boundary > 0xFFFF) {
				bdigit++;
				boundary &= 0xFFFF;
			}
			add5 = false;
		}
		if (b < bdigit) {
			out = floor;
			return true;
		}
		if (b > bdigit) {
			tie = false;
			break;
		}
	}
	if (tie && (floor & 1) == 0) {
		out = floor;
		return true;
	}
	if (floor + 1 > 0xFFFF)
		return false;
	out = floor + 1;
	return true;
}

fx atan_poly_1(fx y, fx x, Status &st) {
	return div(mul(x, y), add(mul(x, x), mul(ATAN_C, mul(y, y))), st);
}

fx atan_poly_2(fx y, fx x, Status &st) {
	return div(mul(x, y), add(mul(y, y), mul(ATAN_C, mul(x, x))), st);
}

fx wrap_phase(fx phi) {
	while (phi < -PI)
		phi += TAU;
	while (PI <= phi)
		phi -= TAU;
	return phi;
}

} // namespace

fx parse(const std::string &text, Status &st) {
	size_t b = 0, e = text.size();
	while (b < e && (text[b] == ' ' || text[b] == '\t' || text[b] == '\r' || text[b] == '\n'))
		b++;
	while (e > b && (text[e - 1] == ' ' || text[e - 1] == '\t' || text[e - 1] == '\r' || text[e - 1] == '\n'))
		e--;
	bool negative = false;
	if (b < e && (text[b] == '+' || text[b] == '-')) {
		negative = text[b] == '-';
		b++;
	}
	std::string s = text.substr(b, e - b);
	size_t dot = s.find('.');
	std::string ip = s.substr(0, dot);
	std::string fp = dot == std::string::npos ? std::string() : s.substr(dot + 1);
	if (ip.empty() && fp.empty()) {
		st.fail();
		return 0;
	}
	for (char c : ip + fp) {
		if (!is_digit(c)) {
			st.fail();
			return 0;
		}
	}
	u64 int_part = 0;
	for (char c : ip)
		int_part = int_part * 10 + u64(c - '0');
	u64 val = int_part << FRAC;
	if (!fp.empty()) {
		u64 frac = 0;
		val += parse_frac(fp, frac) ? frac : u64(ONE);
	}
	return negative ? wrap(u64(0) - val) : wrap(val);
}

// fixed 1.19 display::write_frac_dec + round_and_trim, con 5 dígitos como máximo.
std::string format(fx v) {
	bool negative = v < 0;
	u64 m = uabs(v);
	u64 ip = m >> FRAC;
	u64 frac = m & 0xFFFF;
	u64 tie = 0;
	int digits[5];
	int nd = 0;
	if (frac) {
		for (int i = 0; i < 5; i++) {
			frac *= 10;
			digits[nd++] = int(frac >> 16);
			frac &= 0xFFFF;
			u64 negf = (u64(0x10000) - frac) & 0xFFFF;
			if (frac < 10 || negf < 10)
				break;
			tie = tie * 10 + (i == 0 ? 5 : 0);
			if (frac < tie || negf < tie)
				break;
		}
	}
	if (frac > 0x8000 || (frac == 0x8000 && nd > 0 && (digits[nd - 1] & 1))) {
		int i = nd - 1;
		while (i >= 0 && digits[i] == 9)
			digits[i--] = 0;
		if (i >= 0)
			digits[i]++;
		else
			ip++;
	}
	while (nd > 0 && digits[nd - 1] == 0)
		nd--;
	std::string s = std::to_string(ip);
	if (nd > 0) {
		s += '.';
		for (int i = 0; i < nd; i++)
			s += char('0' + digits[i]);
	}
	if (negative && (ip != 0 || nd > 0))
		s = "-" + s;
	return s;
}

fx from_int(int64_t n) { return wrap(u64(n) << FRAC); }
fx add(fx a, fx b) { return wrap(u64(a) + u64(b)); }
fx sub(fx a, fx b) { return wrap(u64(a) - u64(b)); }

fx asr(fx a, int n) { return a >= 0 ? a >> n : ~((~a) >> n); }

fx mul(fx a, fx b) {
	u64 hi, lo;
	umul128(uabs(a), uabs(b), hi, lo);
	if ((a < 0) != (b < 0)) {
		lo = ~lo + 1;
		hi = ~hi + (lo == 0 ? 1 : 0);
	}
	return wrap((lo >> FRAC) | (hi << (64 - FRAC)));
}

fx div(fx a, fx b, Status &st) {
	if (b == 0) {
		st.fail();
		return 0;
	}
	u64 A = uabs(a), B = uabs(b);
	u64 q = A / B, r = A % B, qf = 0;
	for (int i = 0; i < FRAC; i++) {
		r <<= 1;
		qf <<= 1;
		if (r >= B) {
			r -= B;
			qf |= 1;
		}
	}
	q = (q << FRAC) | qf;
	return (a < 0) != (b < 0) ? wrap(u64(0) - q) : wrap(q);
}

fx div_int(fx a, int64_t n) {
	u64 q = uabs(a) / u64(n);
	return a < 0 ? wrap(u64(0) - q) : wrap(q);
}

fx powi(fx b, int64_t e) {
	if (e == 0)
		return ONE;
	fx t = b;
	for (int64_t i = 0; i < e - 1; i++)
		t = mul(t, b);
	return t;
}

int64_t floor_int(fx a) { return asr(a, FRAC); }
int64_t ceil_int(fx a) { return neg(asr(neg(a), FRAC)); }

int64_t round_int(fx a) {
	const fx h = ONE >> 1;
	return a >= 0 ? asr(add(a, h), FRAC) : neg(asr(add(neg(a), h), FRAC));
}

// fixed-sqrt: raíz entera de los bits desplazada 8 (8 bits de precisión).
fx sqrt(fx a, Status &st) {
	if (a < 0) {
		st.fail();
		return 0;
	}
	u64 n = u64(a), res = 0, bit = u64(1) << 62;
	while (bit > n)
		bit >>= 2;
	while (bit) {
		if (n >= res + bit) {
			n -= res + bit;
			res = (res >> 1) + bit;
		} else {
			res >>= 1;
		}
		bit >>= 2;
	}
	return fx(res << (FRAC / 2));
}

// Taylor de grado 9 con un único plegado en ±π/2: diverge fuera de ~[-3π/2, 3π/2],
// igual que el original.
fx sin(fx x) {
	if (x < -PI_HALF)
		x = sub(-PI_HALF, add(x, PI_HALF));
	else if (PI_HALF < x)
		x = sub(PI_HALF, sub(x, PI_HALF));
	fx s = sub(x, div_int(powi(x, 3), 6));
	s = add(s, div_int(powi(x, 5), 120));
	s = sub(s, asr(div_int(powi(x, 7), 315), 4));
	s = add(s, asr(div_int(div_int(div_int(powi(x, 9), 81), 7), 5), 7));
	return s;
}

fx cos(fx x) { return sin(wrap_phase(add(x, PI_HALF))); }

fx atan2(fx y, fx x, Status &st) {
	fx ya = y < 0 ? neg(y) : y, xa = x < 0 ? neg(x) : x;
	fx d = sub(ya, xa);
	if (y == 0)
		return x < 0 ? PI : 0;
	if (y > 0) {
		if (x == 0)
			return PI_HALF;
		if (x > 0)
			return d < 0 ? atan_poly_1(y, x, st) : sub(PI_HALF, atan_poly_2(y, x, st));
		return 0 <= d ? sub(PI_HALF, atan_poly_2(y, x, st)) : add(PI, atan_poly_1(y, x, st));
	}
	if (x == 0)
		return -PI_HALF;
	if (x > 0)
		return d < 0 ? atan_poly_1(y, x, st) : sub(-PI_HALF, atan_poly_2(y, x, st));
	return 0 <= d ? sub(-PI_HALF, atan_poly_2(y, x, st)) : add(-PI, atan_poly_1(y, x, st));
}

fx fmod(fx a, fx b) {
	u64 r = uabs(a) % uabs(b);
	return a >= 0 ? wrap(r) : wrap(u64(0) - r);
}

fx angle_dist(fx a, fx b) {
	fx d = fmod(sub(b, a), TAU);
	return sub(fmod(add(d, d), TAU), d);
}

} // namespace tbfg
