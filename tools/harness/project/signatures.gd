extends Node

# Llama a cada método nativo con 12 argumentos Nil: godot-rust informa en stderr
# el nombre y el tipo esperado de cada parámetro, y cuántos sobran.

const CLASSES = ["FixedMath", "NativeMethods", "FGObject"]

func _ready():
	for c in CLASSES:
		var inst = load("res://bin/%s.gdns" % c).new()
		var base_names = {}
		for m in ClassDB.class_get_method_list(inst.get_class()):
			base_names[m.name] = true
		for m in inst.get_method_list():
			if base_names.has(m.name):
				continue
			printerr("@@SIG %s.%s" % [c, m.name])
			var args = []
			args.resize(12)
			inst.callv(m.name, args)
	get_tree().quit()
