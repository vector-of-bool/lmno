import subprocess
import sys
from typing import Iterable, Sequence
import re

#* Toggle this to render 'nul' characters in strings
RENDER_NUL = False
#* Toggle this to disable unmangling of output
DO_UNMANGLE = True

# The regex used to detect mangled strings in compiler output
REWRITE_RE = re.compile(
    r'''
    # Match octal sequences and escape sequences:
    ((\\\d{11}|\\\d{3}|\\[a-z])+)
    |
    # Match big lists of integers:
    (
        \{\{
            (-?\d+)(,[ ]-?\d+)+?(,[ ]\.\.\.)?
        \}\}
    )
    ''',
    flags=re.VERBOSE,
)


def iter_code_units(mangled: str) -> Iterable[int]:
    """Iterate the code units in a mangled string"""
    if mangled[0] == '\\':
        return unmangle_escaped(mangled)
    elif mangled[0] == '{':
        return unmangle_iseq(mangled)
    assert False, f'Unknown mangled string: {mangled!r}'


def unmangle_iseq(s: str) -> Iterable[int]:
    """Iterate the code units from a list of integers"""
    inner = s[2:-2]
    elided = False
    if inner.endswith(', ...'):
        # Clang elides long lists.
        elided = True
        inner = inner[:-5]
    # Split on commas:
    parts = inner.split(',')
    # Convert each part into an int:
    vals = map(int, parts)
    # Convert them to the positive values:
    positive = (
        v if v >= 0 else (v + 256)  #
        for v in vals)
    # Yield those:
    yield from positive
    if elided:
        # We elided, so also yield a string that will show that message:
        yield from map(int, '...⟨elided by compiler!⟩'.encode('utf-8'))


def unmangle_escaped(s: str) -> Iterable[int]:
    """Unmangle nested escape sequences."""
    parts = s[1:].split('\\')
    for p in parts:
        if len(p) == 1 and p.isalpha():
            return {
                'n': '\n',
                't': '\t',
            }[p]
        i = int(p, base=8)
        yield i & 0xff


def decode(mat: 're.Match[str]') -> str:
    """Substitution function for re.sub()"""
    s = mat.group(1) or mat.group(3)
    cus = iter_code_units(s)
    good = bytes(cus).decode(errors='replace')
    if s.startswith('{'):
        good = f'{{"{good}"}}'
    if RENDER_NUL:
        good = good.replace('\x00', '␀')
    else:
        good = good.replace('\x00', '')
    return good


def unmangle(content: str) -> str:
    if not DO_UNMANGLE:
        return content
    return REWRITE_RE.sub(decode, content)


def main(command: Sequence[str]) -> int:
    res = subprocess.run(command, check=False, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    c = res.stdout.decode()
    sys.stdout.write(unmangle(c))
    return res.returncode


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
