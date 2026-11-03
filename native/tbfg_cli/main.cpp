// CLI de pruebas de FixedMath. Protocolo (docs/CODEX_TAREA_FixedMath.md):
//   stdin:  metodo<TAB>arg1<TAB>arg2...   (una línea por caso)
//   stdout: resultado codificado, una línea por caso; <null> si hay error.
#include "../tbfg_core/fixed16.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

using namespace tbfg;

namespace {

typedef std::vector<std::string> Args;

bool parse_int(const std::string &s, int64_t &out) {
	size_t i = 0;
	bool negative = false;
	if (i < s.size() && (s[i] == '-' || s[i] == '+'))
		negative = s[i++] == '-';
	if (i == s.size())
		return false;
	uint64_t v = 0;
	for (; i < s.size(); i++) {
		if (s[i] < '0' || s[i] > '9')
			return false;
		v = v * 10 + uint64_t(s[i] - '0');
	}
	out = negative ? int64_t(uint64_t(0) - v) : int64_t(v);
	return true;
}

std::string vec(fx x, fx y) { return format(x) + "," + format(y); }
std::string ivec(int64_t x, int64_t y) { return std::to_string(x) + "," + std::to_string(y); }
std::string boolean(bool b) { return b ? "true" : "false"; }

fx vec_len(fx x, fx y, Status &st) { return sqrt(add(mul(x, x), mul(y, y)), st); }

// kinds: 's' = decimal I48F16, 'i' = entero.
bool read_args(const Args &a, const char *kinds, fx *f, int64_t *n, Status &st) {
	size_t count = 0;
	while (kinds[count])
		count++;
	if (a.size() < count + 1)
		return false;
	for (size_t i = 0; i < count; i++) {
		if (kinds[i] == 's')
			f[i] = parse(a[i + 1], st);
		else if (!parse_int(a[i + 1], n[i]))
			return false;
	}
	return st.ok;
}

std::string call(const Args &a) {
	const std::string &m = a[0];
	Status st;
	fx f[5] = {};
	int64_t n[5] = {};
	std::string r;

#define ARGS(kinds)                          \
	if (!read_args(a, kinds, f, n, st)) \
		return "<null>";

	if (m == "add") { ARGS("ss") r = format(add(f[0], f[1])); }
	else if (m == "sub") { ARGS("ss") r = format(sub(f[0], f[1])); }
	else if (m == "mul") { ARGS("ss") r = format(mul(f[0], f[1])); }
	else if (m == "div") { ARGS("ss") r = format(div(f[0], f[1], st)); }
	else if (m == "abs") { ARGS("s") r = format(f[0] < 0 ? sub(0, f[0]) : f[0]); }
	else if (m == "sign") { ARGS("s") r = std::to_string((f[0] > 0) - (f[0] < 0)); }
	else if (m == "floor") { ARGS("s") r = std::to_string(floor_int(f[0])); }
	else if (m == "ceil") { ARGS("s") r = std::to_string(ceil_int(f[0])); }
	else if (m == "round") { ARGS("s") r = std::to_string(round_int(f[0])); }
	else if (m == "powu") { ARGS("si") r = format(powi(f[0], n[1])); }
	else if (m == "lt") { ARGS("ss") r = boolean(f[0] < f[1]); }
	else if (m == "gt") { ARGS("ss") r = boolean(f[0] > f[1]); }
	else if (m == "le") { ARGS("ss") r = boolean(f[0] <= f[1]); }
	else if (m == "ge") { ARGS("ss") r = boolean(f[0] >= f[1]); }
	else if (m == "eq") { ARGS("ss") r = boolean(f[0] == f[1]); }
	else if (m == "deg2rad") { ARGS("s") r = format(div_int(mul(f[0], PI), 180)); }
	else if (m == "angle_to_vec") { ARGS("s") r = vec(cos(f[0]), sin(f[0])); }
	else if (m == "vec_to_angle") { ARGS("ss") r = format(atan2(f[1], f[0], st)); }
	else if (m == "rotate_vec") {
		ARGS("sss")
		fx c = cos(f[2]), s = sin(f[2]);
		r = vec(sub(mul(f[0], c), mul(f[1], s)), add(mul(f[0], s), mul(f[1], c)));
	}
	else if (m == "angle_dist") { ARGS("ss") r = format(angle_dist(f[0], f[1])); }
	else if (m == "lerp_angle") { ARGS("sss") r = format(add(f[0], mul(angle_dist(f[0], f[1]), f[2]))); }
	else if (m == "lerp_string") { ARGS("sss") r = format(add(mul(f[0], sub(ONE, f[2])), mul(f[1], f[2]))); }
	else if (m == "lerp_int") {
		ARGS("ssii")
		fx t = div(from_int(n[3]), from_int(n[2]), st);
		r = std::to_string(floor_int(add(f[0], mul(sub(f[1], f[0]), t))));
	}
	else if (m == "int_map") {
		ARGS("iiiii")
		fx t = div(from_int(sub(n[0], n[1])), from_int(sub(n[2], n[1])), st);
		r = std::to_string(floor_int(add(from_int(n[3]), mul(t, from_int(sub(n[4], n[3]))))));
	}
	else if (m == "vec_len") { ARGS("ss") r = format(vec_len(f[0], f[1], st)); }
	else if (m == "vec_len_squared") { ARGS("ss") r = format(add(mul(f[0], f[0]), mul(f[1], f[1]))); }
	else if (m == "vec_dist") { ARGS("ssss") r = format(vec_len(sub(f[2], f[0]), sub(f[3], f[1]), st)); }
	else if (m == "vec_dist_squared") {
		ARGS("ssss")
		fx dx = sub(f[2], f[0]), dy = sub(f[3], f[1]);
		r = format(add(mul(dx, dx), mul(dy, dy)));
	}
	else if (m == "vec_dot") { ARGS("ssss") r = format(add(mul(f[0], f[2]), mul(f[1], f[3]))); }
	else if (m == "vec_add") { ARGS("ssss") r = vec(add(f[0], f[2]), add(f[1], f[3])); }
	else if (m == "vec_sub") { ARGS("ssss") r = vec(sub(f[0], f[2]), sub(f[1], f[3])); }
	else if (m == "vec_mul") { ARGS("sss") r = vec(mul(f[0], f[2]), mul(f[1], f[2])); }
	else if (m == "vec_div") { ARGS("sss") r = vec(div(f[0], f[2], st), div(f[1], f[2], st)); }
	else if (m == "vec_scale") { ARGS("sss") r = ivec(round_int(mul(f[0], f[2])), round_int(mul(f[1], f[2]))); }
	else if (m == "vec_round") { ARGS("ss") r = ivec(round_int(f[0]), round_int(f[1])); }
	else if (m == "normalized_vec" || m == "normalized_vec_times") {
		bool times = m == "normalized_vec_times";
		if (times) { ARGS("sss") } else { ARGS("ss") }
		fx l = vec_len(f[0], f[1], st);
		fx x = div(f[0], l, st), y = div(f[1], l, st);
		r = times ? vec(mul(x, f[2]), mul(y, f[2])) : vec(x, y);
	}
	else {
		// get_x_intercept: algoritmo sin identificar (el juego no lo usa).
		return "<null>";
	}
#undef ARGS
	return st.ok ? r : "<null>";
}

} // namespace

int main() {
	std::ios::sync_with_stdio(false);
	std::string line, out;
	while (std::getline(std::cin, line)) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		Args a;
		size_t start = 0;
		for (;;) {
			size_t tab = line.find('\t', start);
			a.push_back(line.substr(start, tab - start));
			if (tab == std::string::npos)
				break;
			start = tab + 1;
		}
		out += call(a);
		out += '\n';
	}
	std::fwrite(out.data(), 1, out.size(), stdout);
	return 0;
}
