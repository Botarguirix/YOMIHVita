"""Prototipo de referencia de FixedMath (I48F16) en Python, con solo aritmética entera.

Sirve para descubrir los algoritmos exactos contra tests/golden antes de portarlos a C++.
También cumple el protocolo del CLI:
    python tools/tbfg_compare.py tests/golden/fixedmath_golden.tsv -- python native/proto/fixedmath_ref.py
"""
import sys

FRAC = 16
ONE = 1 << FRAC
MASK64 = (1 << 64) - 1


def wrap64(v):
    """Desbordamiento como i64 de dos complementos (wrapping)."""
    v &= MASK64
    return v - (1 << 64) if v >> 63 else v


# --- Parseo y formato ---------------------------------------------------------------

def parse_frac(d):
    """Réplica de fixed 1.19 u16::dec_str_frac_to_bin (bloques de 6 dígitos).

    Devuelve los 16 bits fraccionarios, o None si el redondeo llega a 1.0.
    Con más de 6 dígitos, si la entrada se agota coincidiendo con el punto
    medio se trata como empate (redondeo a par), aunque no lo sea.
    """
    n = len(d)
    if n <= 6:
        numer = (int(d) * 10 ** (6 - n) << 11) + 15625
        if (numer >> 16) >= 31250:
            return None
        div_, rem = divmod(numer, 31250)
        if rem == 0 and div_ & 1:
            div_ -= 1
        return div_
    floor = (int(d[:6]) << 11) // 31250
    boundary, add5, tie = floor, True, True
    for ch in d:
        b = ord(ch) - 48
        if not add5 and boundary == 0:
            tie = False
            break
        p = boundary * 10
        bdigit, boundary = p >> 16, p & 0xFFFF
        if add5:
            boundary += 5
            if boundary > 0xFFFF:
                bdigit += 1
                boundary &= 0xFFFF
            add5 = False
        if b < bdigit:
            return floor
        if b > bdigit:
            tie = False
            break
    if tie and floor & 1 == 0:
        return floor
    return None if floor + 1 > 0xFFFF else floor + 1


def parse(s):
    """Decimal -> bits I48F16, como FixedI64::<U16>::from_str."""
    s = s.strip()
    neg = s.startswith("-")
    if s[:1] in "+-":
        s = s[1:]
    ip, _, fp = s.partition(".")
    if not ip and not fp:
        raise ValueError("string has no digits")
    if not (ip + fp).isdigit():
        raise ValueError("invalid digit found in string")
    val = int(ip or "0") << FRAC
    frac = parse_frac(fp) if fp else 0
    val += ONE if frac is None else frac
    return wrap64(-val if neg else val)


def frac_from_digits(q, digits):
    """Parte fraccionaria decimal q/10^digits -> bits, como en parse()."""
    den = 10 ** digits
    r_q, r = divmod(q * ONE, den)
    if 2 * r > den or (2 * r == den and r_q & 1):
        r_q += 1
    return r_q


def fmt(bits):
    """Bits I48F16 -> el decimal más corto que al parsearse da los mismos bits."""
    neg = bits < 0
    m = -bits if neg else bits
    ip, fb = divmod(m, ONE)
    # Réplica de fixed 1.19 display::write_frac_dec + round_and_trim para 16 bits
    # fraccionarios: se trabaja en u16, con 5 dígitos como máximo.
    frac, tie, digits = fb, 0, []
    if frac:
        for i in range(5):
            frac *= 10
            digits.append(frac >> 16)
            frac &= 0xFFFF
            if frac < 10 or (-frac) & 0xFFFF < 10:
                break
            tie = tie * 10 + (5 if i == 0 else 0)
            if frac < tie or (-frac) & 0xFFFF < tie:
                break
    if frac > 0x8000 or (frac == 0x8000 and digits and digits[-1] & 1):
        i = len(digits) - 1
        while i >= 0 and digits[i] == 9:
            digits[i] = 0
            i -= 1
        if i >= 0:
            digits[i] += 1
        else:
            ip += 1
    while digits and digits[-1] == 0:
        digits.pop()
    s = str(ip)
    q = bool(digits)
    if digits:
        s += "." + "".join(map(str, digits))
    if neg and (ip or q):
        s = "-" + s
    return s


# --- Operaciones básicas --------------------------------------------------------------

def mul(a, b):
    return wrap64((a * b) >> FRAC)


def div(a, b):
    if b == 0:
        raise ZeroDivisionError
    q = abs(a << FRAC) // abs(b)
    return wrap64(q if (a < 0) == (b < 0) else -q)


def isqrt(n):
    if n < 0:
        raise ValueError("sqrt de negativo")
    x = 0
    if n:
        x = 1 << ((n.bit_length() + 1) // 2)
        while True:
            y = (x + n // x) // 2
            if y >= x:
                break
            x = y
    return x


def sqrt(a):
    """sqrt de fixed-sqrt: raíz entera de los bits, desplazada 8 (8 bits de precisión)."""
    return isqrt(a) << (FRAC // 2)


def floor_int(a):
    return a >> FRAC


def ceil_int(a):
    return -((-a) >> FRAC)


def round_int(a):
    """Redondeo al más cercano, empates lejos de cero."""
    h = ONE >> 1
    return (a + h) >> FRAC if a >= 0 else -((-a + h) >> FRAC)


def trunc_int(a):
    return a >> FRAC if a >= 0 else -((-a) >> FRAC)


def powu(n, e):
    r = ONE
    for _ in range(e):
        r = mul(r, n)
    return r


def vec_len(x, y):
    return sqrt(wrap64(mul(x, x) + mul(y, y)))


def normalized(x, y):
    l = vec_len(x, y)
    return div(x, l), div(y, l)


def lerp(a, b, f):
    return wrap64(a + mul(wrap64(b - a), f))


# --- Trigonometría: réplica de fixed_trigonometry 0.4.3 ------------------------------

PI = 205887            # fixed::consts::PI truncado a I48F16
PI_HALF = PI // 2      # 102943
TAU = 2 * PI
ATAN_C = 18432         # 0.28125


def tdiv_int(a, n):
    """Fixed / entero: truncamiento hacia cero."""
    q = abs(a) // n
    return q if a >= 0 else -q


def powi(b, p):
    if p == 0:
        return ONE
    t = b
    for _ in range(p - 1):
        t = mul(t, b)
    return t


def sin(x):
    """Taylor de grado 9 con un único plegado en ±π/2 (diverge fuera de ~[-3π/2, 3π/2])."""
    if x < -PI_HALF:
        x = -PI_HALF - (x + PI_HALF)
    elif PI_HALF < x:
        x = PI_HALF - (x - PI_HALF)
    s = x - tdiv_int(powi(x, 3), 6)
    s = s + tdiv_int(powi(x, 5), 120)
    s = s - (tdiv_int(powi(x, 7), 315) >> 4)
    s = s + (tdiv_int(tdiv_int(tdiv_int(powi(x, 9), 81), 7), 5) >> 7)
    return wrap64(s)


def wrap_phase(phi):
    while phi < -PI:
        phi += TAU
    while PI <= phi:
        phi -= TAU
    return phi


def cos(x):
    return sin(wrap_phase(x + PI_HALF))


def atan_poly_1(y, x):
    return div(mul(x, y), mul(x, x) + mul(ATAN_C, mul(y, y)))


def atan_poly_2(y, x):
    return div(mul(x, y), mul(y, y) + mul(ATAN_C, mul(x, x)))


def atan2(y, x):
    ya, xa = abs(y), abs(x)
    if y == 0:
        return PI if x < 0 else 0
    if y > 0:
        if x == 0:
            return PI_HALF
        if x > 0:
            return atan_poly_1(y, x) if ya - xa < 0 else PI_HALF - atan_poly_2(y, x)
        return PI_HALF - atan_poly_2(y, x) if 0 <= ya - xa else PI + atan_poly_1(y, x)
    if x == 0:
        return -PI_HALF
    if x > 0:
        return atan_poly_1(y, x) if ya - xa < 0 else -PI_HALF - atan_poly_2(y, x)
    return -PI_HALF - atan_poly_2(y, x) if 0 <= ya - xa else -PI + atan_poly_1(y, x)


def rotate(x, y, t):
    c, s = cos(t), sin(t)
    return wrap64(mul(x, c) - mul(y, s)), wrap64(mul(x, s) + mul(y, c))


def fmod(a, b):
    """Resto con el signo del dividendo (como % en Rust)."""
    r = abs(a) % abs(b)
    return r if a >= 0 else -r


def angle_dist(a, b):
    d = fmod(b - a, TAU)
    return fmod(2 * d, TAU) - d


METHODS = {
    "add": (("s", "s"), lambda x, y: fmt(wrap64(x + y))),
    "sub": (("s", "s"), lambda x, y: fmt(wrap64(x - y))),
    "mul": (("s", "s"), lambda x, y: fmt(mul(x, y))),
    "div": (("s", "s"), lambda x, y: fmt(div(x, y))),
    "abs": (("s",), lambda n: fmt(abs(n))),
    "lt": (("s", "s"), lambda x, y: x < y),
    "gt": (("s", "s"), lambda x, y: x > y),
    "le": (("s", "s"), lambda x, y: x <= y),
    "ge": (("s", "s"), lambda x, y: x >= y),
    "eq": (("s", "s"), lambda x, y: x == y),
    "sign": (("s",), lambda a: (a > 0) - (a < 0)),
    "floor": (("s",), floor_int),
    "ceil": (("s",), ceil_int),
    "round": (("s",), round_int),
    "powu": (("s", "i"), lambda n, e: fmt(powu(n, e))),
    "vec_len": (("s", "s"), lambda x, y: fmt(vec_len(x, y))),
    "vec_len_squared": (("s", "s"), lambda x, y: fmt(wrap64(mul(x, x) + mul(y, y)))),
    "vec_dist": (("s",) * 4, lambda x1, y1, x2, y2: fmt(vec_len(x2 - x1, y2 - y1))),
    "vec_dist_squared": (("s",) * 4, lambda x1, y1, x2, y2:
                         fmt(wrap64(mul(x2 - x1, x2 - x1) + mul(y2 - y1, y2 - y1)))),
    "vec_dot": (("s",) * 4, lambda x1, y1, x2, y2: fmt(wrap64(mul(x1, x2) + mul(y1, y2)))),
    "vec_add": (("s",) * 4, lambda x1, y1, x2, y2: (fmt(x1 + x2), fmt(y1 + y2))),
    "vec_sub": (("s",) * 4, lambda x1, y1, x2, y2: (fmt(x1 - x2), fmt(y1 - y2))),
    "vec_mul": (("s",) * 3, lambda x, y, r: (fmt(mul(x, r)), fmt(mul(y, r)))),
    "vec_div": (("s",) * 3, lambda x, y, r: (fmt(div(x, r)), fmt(div(y, r)))),
    "vec_scale": (("s",) * 3, lambda x, y, f: (round_int(mul(x, f)), round_int(mul(y, f)))),
    "vec_round": (("s", "s"), lambda x, y: (round_int(x), round_int(y))),
    "normalized_vec": (("s", "s"), lambda x, y: tuple(fmt(v) for v in normalized(x, y))),
    "normalized_vec_times": (("s",) * 3, lambda x, y, m:
                             tuple(fmt(mul(v, m)) for v in normalized(x, y))),
    "lerp_string": (("s",) * 3, lambda a, b, f: fmt(wrap64(mul(a, ONE - f) + mul(b, f)))),
    "deg2rad": (("s",), lambda n: fmt(tdiv_int(mul(n, PI), 180))),
    "angle_to_vec": (("s",), lambda t: (fmt(cos(t)), fmt(sin(t)))),
    "vec_to_angle": (("s", "s"), lambda x, y: fmt(atan2(y, x))),
    "rotate_vec": (("s",) * 3, lambda x, y, t: tuple(fmt(v) for v in rotate(x, y, t))),
    "angle_dist": (("s", "s"), lambda a, b: fmt(angle_dist(a, b))),
    "lerp_angle": (("s",) * 3, lambda a, b, w: fmt(wrap64(a + mul(angle_dist(a, b), w)))),
    "lerp_int": (("s", "s", "i", "i"), lambda a, b, mx, f:
                 floor_int(lerp(a, b, div(f << FRAC, mx << FRAC)))),
    "int_map": (("i",) * 5, lambda v, i0, i1, o0, o1:
                floor_int(wrap64((o0 << FRAC) + mul(div((v - i0) << FRAC, (i1 - i0) << FRAC),
                                                    (o1 - o0) << FRAC)))),
}


def encode(v):
    if v is None:
        return "<null>"
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, tuple):
        return ",".join(encode(x) for x in v)
    return str(v)


def run_line(line):
    parts = line.rstrip("\r\n").split("\t")
    spec = METHODS.get(parts[0])
    if spec is None:
        return "<sin implementar>"
    kinds, fn = spec
    try:
        args = [parse(a) if k == "s" else int(a) for k, a in zip(kinds, parts[1:])]
        return encode(fn(*args))
    except (ZeroDivisionError, ValueError, OverflowError):
        return "<null>"


if __name__ == "__main__":
    out = [run_line(l) for l in sys.stdin]
    sys.stdout.write("\n".join(out) + "\n")
