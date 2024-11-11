#!/usr/bin/env python3

print("+ Preparing .dmg disk image +")

if verbose:
    print("Determining size of \"dist\"...")
size = 0
for path, dirs, files in os.walk("dist"):
    for file in files:
        size += os.path.getsize(os.path.join(path, file))
size += int(size * 0.15)

if verbose:
    print("Creating temp image for modification...")

tempname: str = appname + ".temp.dmg"

run(["hdiutil", "create", tempname, "-srcfolder", "dist", "-format", "UDRW", "-size", str(size), "-volname", appname], check=True, universal_newlines=True)

if verbose:
    print("Attaching temp image...")
output = run(["hdiutil", "attach", tempname, "-readwrite"], check=True, universal_newlines=True, stdout=PIPE).stdout

m = re.search(r"/Volumes/(.+$)", output)
disk_root = m.group(0)

print("+ Applying fancy settings +")

bg_path = os.path.join(disk_root, ".background", os.path.basename('background.tiff'))
os.mkdir(os.path.dirname(bg_path))
if verbose:
    print('background.tiff', "->", bg_path)
shutil.copy2('contrib/macdeploy/background.tiff', bg_path)

os.symlink("/Applications", os.path.join(disk_root, "Applications"))

print("+ Finalizing .dmg disk image +")

run(["hdiutil", "detach", f"/Volumes/{appname}"], universal_newlines=True)

run(["hdiutil", "convert", tempname, "-format", "UDZO", "-o", appname, "-imagekey", "zlib-level=9"], check=True, universal_newlines=True)

os.unlink(tempname)
