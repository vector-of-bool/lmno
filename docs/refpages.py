from pathlib import Path
import re
import shutil
from itertools import chain
from typing import Iterable, Mapping, NamedTuple, Sequence, Union
import textwrap
import yaml

JSONScalar = Union[float, int, bool, None, str]
JSONObject = Mapping[str, 'JSONValue']
JSONSequence = Sequence['JSONValue']
JSONValue = Union[JSONObject, JSONSequence, JSONScalar]


def generate(input_dir: Path, out_dir: Path) -> None:
    tmpdir = out_dir.with_suffix('.tmp')
    shutil.rmtree(tmpdir, ignore_errors=True)
    tmpdir.mkdir(parents=True)
    _generate_into(input_dir, tmpdir)
    deldir: Path | None = None
    if out_dir.exists():
        deldir = out_dir.with_name('.del')
        out_dir.rename(deldir)
    tmpdir.rename(out_dir)
    if deldir:
        shutil.rmtree(deldir)


class YamlLoader(yaml.SafeLoader):
    pass


def _generate_into(indir: Path, outdir: Path) -> None:
    idx = indir / 'index.yaml'
    data = _load_yaml(idx, indir)
    assert isinstance(data, Mapping), data
    ctx = RefContext((), '')
    _gen_new_rst_file(outdir, data, ctx)


def _load_yaml(fpath: Path, dir: Path) -> JSONValue:

    class loader_cls(yaml.SafeLoader):
        pass

    loader_cls.add_constructor('!include', lambda ldr, node: _include_yaml(ldr, node, dir))
    loader_cls.add_constructor('!include-str', lambda ldr, node: _include_str(ldr, node, dir))
    loader = loader_cls(fpath.read_text())
    data: JSONValue = loader.get_single_data()
    return data


def _include_yaml(loader: yaml.BaseLoader, node: yaml.Node, basedir: Path) -> JSONValue:
    path = node.value
    assert isinstance(path, str), '!include must be tagged on a string scalar'
    resolve = basedir / path
    return _load_yaml(resolve, basedir)


def _include_str(ldr: yaml.BaseLoader, node: yaml.Node, basedir: Path) -> str:
    path = node.value
    assert isinstance(path, str), '!include-str must be tagged on a string scalar'
    return basedir.joinpath(path).read_text()


class RefContext(NamedTuple):
    namespace_templates: tuple[str, ...]
    namespace: str

    @property
    def namespace_directive(self) -> str:
        if not self.namespace:
            return ''
        tmpls = ' '.join(f'template {t}' for t in self.namespace_templates)
        return f'.. namespace:: {tmpls} {self.namespace}\n'


def _gen_new_rst_file(out_dir: Path, item: JSONObject, ctx: RefContext):
    head = textwrap.dedent(r'''
        .. THESE PAGES ARE GENERATED! ANY MODIFICATIONS WILL BE DISCARDED

        .. default-domain:: cpp
        .. default-role:: cpp:expr
    ''')

    lines = _gen_rst_root(out_dir, item, ctx)
    out_dir.mkdir(exist_ok=True, parents=True)
    index_rst = out_dir / 'index.rst'
    print('Writing', index_rst)
    index_rst.write_text(head + '\n' + '\n'.join(lines) + '\n')


def _gen_rst_root(out_dir: Path, item: JSONObject, ctx: RefContext) -> Iterable[str]:
    if 'page' in item:
        page = item['page']
        assert isinstance(page, Mapping), page
        return _gen_page_generic(out_dir, page, ctx)
    elif 'ent-page' in item:
        ent = item['ent-page']
        assert isinstance(ent, Mapping)
        return _gen_page_entity(out_dir, ent, ctx)
    elif 'rst' in item:
        content = item['rst']
        assert isinstance(content, str)
        return _iterlines(content)
    assert False, f'No "page" or "ent-page" in item: {item!r}'


def _gen_page_entity(out_dir: Path, ent: JSONObject, ctx: RefContext) -> Iterable[str]:
    ns = ent.get('ns')
    if ns:
        assert isinstance(ns, str), ns
        ctx = RefContext((), ns)
    name = _getstr(ent, 'name')
    kind = _getstr(ent, 'kind')
    prefix = {
        'struct': 'Struct',
        'type': 'Type Alias',
        'class': 'Class',
        'fn': 'Function',
        'ctor': 'Constructor',
        'const': 'Constant',
        'var': 'Variable',
        'concept': 'Concept',
    }[kind]
    if 'template' in ent and kind != 'concept':
        prefix = f'{prefix} Template'
    fqname = f'{ctx.namespace}::{name}'
    yield from _gen_page_generic(out_dir, {'title': f'{prefix}: ``{fqname}``'}, ctx)
    yield from _gen_ent_content(out_dir, ent, ctx)


def _gen_page_generic(out_dir: Path, page: JSONObject, ctx: RefContext) -> Iterable[str]:
    title = page.get('title')
    if title:
        assert isinstance(title, str), title
        ovr = '#' * len(title)
        yield from (ovr, title, ovr)
        yield ''

    ns = page.get('ns')
    if ns:
        assert isinstance(ns, str)
        ctx = RefContext((), ns)
        yield ctx.namespace_directive
        yield ''

    intro = page.get('intro')
    if intro:
        assert isinstance(intro, str), intro
        yield from _iterlines(intro)
        yield ''

    contents = page.get('contents', {}) or {}
    assert isinstance(contents, Mapping)
    toc_entries: list[str] = []
    for header, tbl in contents.items():
        assert isinstance(tbl, Sequence)
        yield from _gen_summary_table(header, out_dir, tbl, toc_entries, ctx)
        yield ''

    main = page.get('main')
    if main:
        assert isinstance(main, str)
        yield main
        yield ''

    ents = page.get('entities', []) or []
    assert isinstance(ents, Sequence), ents
    for ent in ents:
        assert isinstance(ent, Mapping), ent
        yield from _gen_ent_content(out_dir, ent, ctx)
        yield ''

    outro = page.get('outro')
    if outro:
        assert isinstance(outro, str)
        yield outro
        yield ''

    if toc_entries:
        yield '.. rubric:: Contents:'
        yield '.. toctree::'
        yield ''
        for l in toc_entries:
            yield f'  {l}'


def _gen_ent_content(out_dir: Path, ent: JSONObject, ctx: RefContext) -> Iterable[str]:
    kind = _getstr(ent, 'kind')
    fn = {
        'struct': _gen_class,
        'class': _gen_class,
        'type': _gen_type_alias,
        'fn': _gen_fn,
        'ctor': _gen_fn,
        'const': _gen_var,
        'var': _gen_var,
        'concept': _gen_concept,
    }.get(kind)
    assert fn, f'Unknown entity "kind": {kind!r}'
    yield from fn(out_dir, ent, ctx)


def _gen_type_alias(out_dir: Path, typ: JSONObject, ctx: RefContext) -> Iterable[str]:
    yield ctx.namespace_directive

    name = _getstr(typ, 'name')
    sig = name
    tmpl = typ.get('template')
    if tmpl:
        sig = f'template {tmpl} {sig}'
    is_ = typ.get('is')
    if is_:
        sig = f'{sig} = {is_}'

    yield f'.. type:: {sig}'
    yield ''
    yield from _indent_all(2, _gen_page_generic(out_dir, typ, ctx))
    yield ''


def _gen_var(out_dir: Path, ent: JSONObject, ctx: RefContext) -> Iterable[str]:
    yield ctx.namespace_directive

    name = _getstr(ent, 'name')
    typ = _getstr(ent, 'type')
    tmpl = ent.get('template')
    if tmpl:
        typ = f'template {tmpl} {typ}'
    is_ = ent.get('is')
    if is_:
        name = f'{name} = {is_}'
    yield f'.. var:: {typ} {name}'
    yield ''
    yield from _indent_all(2, _gen_var_body(out_dir, ent, ctx))
    yield ''


def _gen_var_body(out_dir: Path, var: JSONObject, ctx: RefContext) -> Iterable[str]:
    yield from _gen_page_generic(out_dir, var, ctx)


def _gen_concept(out_dir: Path, con: JSONObject, ctx: RefContext) -> Iterable[str]:
    yield ctx.namespace_directive
    name = _getstr(con, 'name')
    tmpl = _getstr(con, 'template')
    is_ = con.get('is')
    sig = f'template {tmpl} {name}'
    if is_:
        sig = f'{sig} = {is_}'
    yield f'.. concept:: {sig}'
    yield ''
    yield from _indent_all(2, _gen_page_generic(out_dir, con, ctx))
    yield ''


def _gen_class(out_dir: Path, ent: JSONObject, ctx: RefContext) -> Iterable[str]:
    yield ctx.namespace_directive

    name = _getstr(ent, 'name')
    sig = name
    tmpl = ent.get('template')
    if tmpl:
        sig = f'template {tmpl} {sig}'
    yield f'.. {ent["kind"]}:: {sig}'

    yield ''
    new_tmpls = ctx.namespace_templates
    if tmpl:
        assert isinstance(tmpl, str)
        new_tmpls = new_tmpls + (tmpl, )
    yield from _indent_all(2, _gen_page_generic(out_dir, ent, RefContext(new_tmpls, f'{ctx.namespace}::{name}')))
    yield ''


def _gen_fn(out_dir: Path, ent: JSONObject, ctx: RefContext) -> Iterable[str]:
    yield ctx.namespace_directive
    sigs = ent.get('sigs')
    assert isinstance(sigs, Sequence)
    yield '.. function::'
    many = len(sigs) > 1
    for nth, fn in enumerate(sigs):
        sig: JSONValue = fn
        tmpl = ''
        if isinstance(fn, Mapping):
            sig = fn['sig']
            if 'template' in fn:
                tmpl = f'template {fn["template"]} '
        if many:
            yield f'    {tmpl} [[#{nth+1}]] {sig}'
        else:
            yield f'    {tmpl} {sig}'
    yield ''

    yield from _indent_all(2, _gen_fn_body(out_dir, sigs, ent, ctx))


def _gen_fn_body(out_dir: Path, sigs: JSONSequence, ent: JSONObject, ctx: RefContext) -> Iterable[str]:
    for nth, sig in enumerate(sigs):
        if not isinstance(sig, Mapping) or not 'desc' in sig:
            continue
        desc = sig['desc']
        assert isinstance(desc, str)
        inner = '\n'.join(_indent_all(3, [desc])).lstrip()
        yield f'{nth+1}. {inner}'

    yield ''

    pars = ent.get('params', {})
    assert isinstance(pars, Mapping)
    for name, desc in pars.items():
        assert isinstance(desc, str)
        desc = '\n  '.join(_iterlines(desc))
        yield f':param {name}:\n  {desc}'

    yield ''
    yield from _gen_page_generic(out_dir, ent, ctx)
    yield ''


def _gen_summary_table(header: str, out_dir: Path, items: JSONSequence, toc_entries: list[str],
                       ctx: RefContext) -> Iterable[str]:
    yield from _dedent(fr'''
        .. csv-table::
            :header: {header}
            :class: summary-table mono-links
    ''')

    for it in items:
        assert isinstance(it, Mapping), it
        name = _getstr(it, 'name')
        slug = it.get('slug') or name
        assert isinstance(slug, str), f'"slug" must be a string (Got {slug!r})'
        _gen_new_rst_file(out_dir / slug, it, ctx)
        link = slug + '/index'
        toc_entries.append(link)
        name = _escape(name)
        desc = it['desc']
        assert isinstance(desc, str), desc
        desc = desc.replace('"', '""')
        ref = f':doc:`{name} <{link}>`'.replace('"', '""')
        yield f'    "{ref}", "{desc}"'


def _getstr(obj: JSONObject, key: str) -> str:
    v = obj.get(key)
    assert isinstance(v, str), f'Property "{key}" of object {obj!r} must be a string (Got {v!r})'
    return v


def _getmap(obj: JSONObject, key: str) -> JSONObject:
    v = obj.get(key)
    assert isinstance(v, Mapping), f'Property "{key}" of object {obj!r} must be a mapping (Got {v!r})'
    return v


def _dedent(s: str) -> Iterable[str]:
    s = textwrap.dedent(s)
    yield from _iterlines(s)
    yield ''


def _iterlines(s: str) -> Iterable[str]:
    if '\n' in s:
        yield from chain.from_iterable(_iterlines(l) for l in s.splitlines())
    else:
        yield s


def _indent_all(depth: int, lines: Iterable[str]) -> Iterable[str]:
    indent = (' ' * depth)
    for s in lines:
        for line in _iterlines(s):
            yield f'{indent}{line}'


def _escape(v: str) -> str:
    return re.sub(r'([<>])', r'\\\1', v)
