"""Extract only SDK headers/libraries; materialize Unix links on Windows.
The caller verifies the pinned layer SHA256 before using this script.
"""
import pathlib, posixpath, shutil, sys, tarfile
archive, destination = map(pathlib.Path, sys.argv[1:3])
root = destination.resolve()
root.mkdir(parents=True, exist_ok=True)
prefixes = ('opt/miyoo/arm-miyoo-linux-uclibcgnueabi/sysroot/',
            'opt/miyoo/lib/gcc/arm-miyoo-linux-uclibcgnueabi/')
links = []
def target(name):
    clean = posixpath.normpath(name).lstrip('/')
    out = (root / clean).resolve()
    if root not in out.parents:
        raise ValueError('Archive path escapes SDK: ' + name)
    return out
with tarfile.open(archive, 'r:gz') as source:
    for member in source:
        name = member.name.removeprefix('./')
        if not name.startswith(prefixes):
            continue
        out = target(name)
        if member.isdir():
            out.mkdir(parents=True, exist_ok=True)
        elif member.isfile():
            out.parent.mkdir(parents=True, exist_ok=True)
            with source.extractfile(member) as inp, out.open('wb') as dest:
                shutil.copyfileobj(inp, dest)
        elif member.issym() or member.islnk():
            if member.linkname.startswith('/'):
                base = 'opt/miyoo/arm-miyoo-linux-uclibcgnueabi/sysroot'
                link = base + member.linkname
            elif member.issym():
                link = posixpath.join(posixpath.dirname(name), member.linkname)
            else:
                link = member.linkname
            links.append((out, target(link)))
for _ in range(20):
    remaining = []
    for out, link in links:
        if link.is_file():
            out.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(link, out)
        elif link.is_dir():
            shutil.copytree(link, out, dirs_exist_ok=True)
        else:
            remaining.append((out, link))
    if not remaining or len(remaining) == len(links):
        links = remaining
        break
    links = remaining
print('SDK extracted:', root)
if links:
    print('Unresolved SDK links (must not be silently used by build):')
    for out, link in links:
        print(out.relative_to(root), '->', link.relative_to(root))
