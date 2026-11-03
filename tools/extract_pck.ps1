# Extrae un .pck de Godot 3.x (formato v1, sin cifrar) a una carpeta.
# Uso: .\extract_pck.ps1 -Pck <ruta.pck> -Out <carpeta>
param(
  [Parameter(Mandatory)] [string]$Pck,
  [Parameter(Mandatory)] [string]$Out
)

$fs = [IO.File]::OpenRead($Pck)
$br = New-Object IO.BinaryReader($fs)
if ([Text.Encoding]::ASCII.GetString($br.ReadBytes(4)) -ne 'GDPC') { throw 'No es un .pck de Godot' }
$fmt = $br.ReadUInt32()
$ver = '{0}.{1}.{2}' -f $br.ReadUInt32(), $br.ReadUInt32(), $br.ReadUInt32()
if ($fmt -ne 1) { throw "Formato de pck $fmt no soportado (solo v1 / Godot 3.x)" }
$br.ReadBytes(64) | Out-Null
$count = $br.ReadUInt32()

$entries = for ($i = 0; $i -lt $count; $i++) {
  $len = $br.ReadUInt32()
  $path = [Text.Encoding]::UTF8.GetString($br.ReadBytes($len)).TrimEnd([char]0)
  [pscustomobject]@{ Path = $path; Offset = $br.ReadUInt64(); Size = $br.ReadUInt64(); Md5 = $br.ReadBytes(16) }
}

foreach ($e in $entries) {
  $dest = Join-Path $Out ($e.Path -replace '^res://', '')
  New-Item -ItemType Directory -Force (Split-Path $dest) | Out-Null
  $fs.Position = [int64]$e.Offset
  $buf = $br.ReadBytes([int]$e.Size)
  [IO.File]::WriteAllBytes($dest, $buf)
}
$br.Close()
"Godot $ver - $count archivos extraidos en $Out"
