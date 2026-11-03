extends Node

# Arnés de referencia: carga tbfg.dll original y vuelca su API y resultados.

const CLASSES = ["FixedMath", "NativeMethods", "FGObject"]

var out = []

func log_line(s):
	out.append(str(s))
	print(s)

func _ready():
	for c in CLASSES:
		var script = load("res://bin/%s.gdns" % c)
		if script == null:
			log_line("ERROR no se pudo cargar " + c)
			continue
		var inst = script.new()
		log_line("== %s (%s)" % [c, inst])
		var base = ClassDB.class_get_method_list(inst.get_class())
		var base_names = {}
		for m in base:
			base_names[m.name] = true
		for m in inst.get_method_list():
			if not base_names.has(m.name):
				log_line("  method %s args=%s ret=%s" % [m.name, m.args, m.return])
		for p in inst.get_property_list():
			log_line("  prop %s type=%s" % [p.name, p.type])
		if inst is Reference:
			pass
		elif inst.has_method("free"):
			inst.free()
	var f = File.new()
	f.open("user://harness_api.txt", File.WRITE)
	f.store_string(PoolStringArray(out).join("\n"))
	f.close()
	log_line("guardado en " + OS.get_user_data_dir())
	get_tree().quit()
