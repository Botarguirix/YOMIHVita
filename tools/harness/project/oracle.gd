extends Node

# Oráculo: lee user://oracle_in.tsv (metodo<TAB>args...; los args que empiezan
# por "i:" son int) y escribe en user://oracle_out.tsv el resultado de tbfg.dll
# con la misma codificación que golden.gd. Lo usa tools/oracle.py.

func enc(v) -> String:
	match typeof(v):
		TYPE_STRING: return v
		TYPE_INT: return str(v)
		TYPE_BOOL: return "true" if v else "false"
		TYPE_DICTIONARY: return enc(v["x"]) + "," + enc(v["y"])
		TYPE_NIL: return "<null>"
	return "<tipo %d>" % typeof(v)

func _ready():
	var fm = load("res://bin/FixedMath.gdns").new()
	var fin = File.new()
	var fout = File.new()
	fin.open("user://oracle_in.tsv", File.READ)
	fout.open("user://oracle_out.tsv", File.WRITE)
	while not fin.eof_reached():
		var line = fin.get_line()
		if line == "":
			continue
		var parts = line.split("\t")
		var method = parts[0]
		# "bits:metodo" devuelve los bits I48F16 exactos (valor * 65536).
		var as_bits = method.begins_with("bits:")
		if as_bits:
			method = method.substr(5)
		var args = []
		for i in range(1, parts.size()):
			var a = parts[i]
			args.append(int(a.substr(2)) if a.begins_with("i:") else a)
		var r = fm.callv(method, args)
		if as_bits:
			if typeof(r) == TYPE_STRING:
				r = fm.mul(r, "65536")
			elif typeof(r) == TYPE_DICTIONARY:
				r = {"x": fm.mul(r["x"], "65536"), "y": fm.mul(r["y"], "65536")}
		fout.store_line(enc(r))
	fin.close()
	fout.close()
	get_tree().quit()
