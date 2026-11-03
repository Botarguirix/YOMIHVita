extends Node

# Pruebas numéricas contra tbfg.dll original para deducir el formato de punto fijo.

const PROBES = [
	["add", ["1", "2"]],
	["add", ["0.1", "0.2"]],
	["sub", ["1", "0.3"]],
	["mul", ["0.1", "0.1"]],
	["mul", ["1.5", "-2"]],
	["div", ["1", "3"]],
	["div", ["2", "3"]],
	["div", ["1", "7"]],
	["div", ["-1", "3"]],
	["div", ["1", "65536"]],
	["div", ["1", "4294967296"]],
	["div", ["1", "8589934592"]],
	["mul", ["0.00001", "0.00001"]],
	["add", ["2147483647", "1"]],
	["add", ["9223372036", "1"]],
	["sqrt", ["2"]],
	["vec_len", ["3", "4"]],
	["vec_len", ["1", "1"]],
	["vec_dist", ["0", "0", "1", "1"]],
	["deg2rad", ["180"]],
	["deg2rad", ["1"]],
	["vec_to_angle", ["0", "1"]],
	["vec_to_angle", ["1", "1"]],
	["angle_to_vec", ["1"]],
	["angle_to_vec", ["0.5"]],
	["rotate_vec", ["1", "0", "1"]],
	["normalized_vec", ["3", "4"]],
	["normalized_vec_times", ["3", "4", "10"]],
	["lerp_string", ["0", "10", "0.33"]],
	["lerp_int", [0, 10, "0.33"]],
	["lerp_angle", ["0", "3", "0.5"]],
	["angle_dist", ["0", "3"]],
	["sign", ["-0.5"]],
	["powu", ["1.1", 3]],
	["ceil", ["1.2"]],
	["floor", ["-1.2"]],
	["round", ["2.5"]],
	["round", ["-2.5"]],
	["abs", ["-3.3"]],
	["from", [3]],
	["int_map", [5, 0, 10, 0, 100]],
	["get_x_intercept", ["0", "0", "1", "1", "5"]],
	["vec_round", ["1.6", "-1.6"]],
	["vec_scale", ["1", "2", "0.5"]],
	["lt", ["1", "2"]],
	["eq", ["0.1", "0.10"]],
]

var out = []

func log_line(s):
	out.append(str(s))
	print(s)

func _ready():
	var fm = load("res://bin/FixedMath.gdns").new()
	for p in PROBES:
		var r = fm.callv(p[0], p[1])
		log_line("%s%s = %s  (%s)" % [p[0], p[1], var2str(r), typeof(r)])
	var o = load("res://bin/FGObject.gdns").new()
	log_line("FGObject.get_data() = " + var2str(o.get_data()))
	var f = File.new()
	f.open("user://harness_probe.txt", File.WRITE)
	f.store_string(PoolStringArray(out).join("\n"))
	f.close()
	get_tree().quit()
