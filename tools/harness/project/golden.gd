extends Node

# Genera casos de referencia de FixedMath con tbfg.dll original.
# Salida: user://fixedmath_golden.tsv, una línea por caso:
#   metodo <TAB> arg1 <TAB> ... <TAB> => <TAB> resultado
# Codificación del resultado: String tal cual, int en decimal, bool true/false,
# vector {x, y} como "x,y", null como <null>.

const SEED = 20261102
const PER_METHOD = 1500

# Firma de cada método: "s" = String decimal, "i" = int, y un generador de valores.
const METHODS = {
	"add": ["s:big", "s:big"],
	"sub": ["s:big", "s:big"],
	"mul": ["s:mid", "s:mid"],
	"div": ["s:mid", "s:nonzero"],
	"abs": ["s:big"],
	"sign": ["s:big"],
	"floor": ["s:big"],
	"ceil": ["s:big"],
	"round": ["s:big"],
	"powu": ["s:base", "i:exp"],
	"lt": ["s:small", "s:small"],
	"gt": ["s:small", "s:small"],
	"le": ["s:small", "s:small"],
	"ge": ["s:small", "s:small"],
	"eq": ["s:small", "s:small"],
	"deg2rad": ["s:deg"],
	"angle_to_vec": ["s:angle"],
	"vec_to_angle": ["s:mid", "s:mid"],
	"rotate_vec": ["s:mid", "s:mid", "s:angle"],
	"angle_dist": ["s:angle", "s:angle"],
	"lerp_angle": ["s:angle", "s:angle", "s:unit"],
	"lerp_string": ["s:mid", "s:mid", "s:unit"],
	"lerp_int": ["s:mid", "s:mid", "i:max", "i:frac"],
	"int_map": ["i:any", "i:any", "i:any", "i:any", "i:any"],
	"vec_len": ["s:mid", "s:mid"],
	"vec_len_squared": ["s:mid", "s:mid"],
	"vec_dist": ["s:mid", "s:mid", "s:mid", "s:mid"],
	"vec_dist_squared": ["s:mid", "s:mid", "s:mid", "s:mid"],
	"vec_dot": ["s:mid", "s:mid", "s:mid", "s:mid"],
	"vec_add": ["s:big", "s:big", "s:big", "s:big"],
	"vec_sub": ["s:big", "s:big", "s:big", "s:big"],
	"vec_mul": ["s:mid", "s:mid", "s:mid"],
	"vec_div": ["s:mid", "s:mid", "s:nonzero"],
	"vec_scale": ["s:mid", "s:mid", "s:mid"],
	"vec_round": ["s:big", "s:big"],
	"normalized_vec": ["s:nonzero", "s:mid"],
	"normalized_vec_times": ["s:nonzero", "s:mid", "s:mid"],
	"get_x_intercept": ["s:mid", "s:mid", "s:mid", "s:mid"],
}

# Valores frecuentes en el juego y bordes conocidos.
const SPECIAL = ["0", "1", "-1", "0.5", "-0.5", "2", "0.1", "0.00001", "0.00002",
	"0.33333", "1.41406", "3.14159", "6.28318", "1.57079", "10", "-10", "100",
	"0.99999", "1.00001", "-0.00001", "15", "0.25", "0.75", "1.5"]

var rng = RandomNumberGenerator.new()

func rand_decimal(limit: float, max_decimals: int) -> String:
	var r = rng.randi_range(0, 9)
	if r == 0:
		return SPECIAL[rng.randi_range(0, SPECIAL.size() - 1)]
	var int_part = rng.randi_range(0, int(limit))
	var s = str(int_part)
	var n_dec = rng.randi_range(0, max_decimals)
	if n_dec > 0:
		s += "."
		for _i in range(n_dec):
			s += str(rng.randi_range(0, 9))
	if rng.randi_range(0, 1) == 1 and s != "0":
		s = "-" + s
	return s

func gen(kind: String):
	match kind:
		"s:big": return rand_decimal(1000000, 7)
		"s:mid": return rand_decimal(3000, 6)
		"s:small": return rand_decimal(rng.randi_range(0, 1) * 1000 + 3, 6)
		"s:nonzero":
			var v = rand_decimal(3000, 5)
			while float(v) == 0.0:
				v = rand_decimal(3000, 5)
			return v
		"s:base": return rand_decimal(3, 5)
		"s:deg": return rand_decimal(720, 4)
		"s:angle": return rand_decimal(7, 5)
		"s:unit":
			var n = rng.randi_range(0, 100000)
			return "1" if n == 100000 else "0.%05d" % n
		"i:exp": return rng.randi_range(0, 6)
		"i:max": return rng.randi_range(1, 1000)
		"i:frac": return rng.randi_range(0, 1000)
		"i:any": return rng.randi_range(-1000, 1000)
	push_error("tipo desconocido " + kind)
	return null

func enc(v) -> String:
	match typeof(v):
		TYPE_STRING: return v
		TYPE_INT: return str(v)
		TYPE_BOOL: return "true" if v else "false"
		TYPE_DICTIONARY: return enc(v["x"]) + "," + enc(v["y"])
		TYPE_NIL: return "<null>"
	return "<tipo %d>" % typeof(v)

func _ready():
	rng.seed = SEED
	var fm = load("res://bin/FixedMath.gdns").new()
	var f = File.new()
	f.open("user://fixedmath_golden.tsv", File.WRITE)
	var total = 0
	for m in METHODS:
		var kinds = METHODS[m]
		for i in range(PER_METHOD):
			var args = []
			for k in kinds:
				args.append(gen(k))
			# lerp_int: f debe ser <= max
			if m == "lerp_int":
				args[3] = rng.randi_range(0, args[2])
			# int_map: el rango de entrada no puede ser vacío
			if m == "int_map" and args[1] == args[2]:
				args[2] += 1
			var r = fm.callv(m, args)
			var line = PoolStringArray([m])
			for a in args:
				line.append(str(a))
			line.append("=>")
			line.append(enc(r))
			f.store_line(line.join("\t"))
			total += 1
	f.close()
	print("casos: %d en %s" % [total, OS.get_user_data_dir()])
	get_tree().quit()
