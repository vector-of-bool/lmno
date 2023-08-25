import os
import re
import sys
from pathlib import Path

from dagon import fs, http, option, proc, task, ui

HERE = Path(__file__).parent.resolve()

USE_BPT = option.add(
    "bpt", Path, default=None, doc="Use this path to an existing bpt executable instead of a downloaded version"
)
BPT_VERSION = option.add("bpt.version", str, default="1.0.0-beta.1", doc="The version of bpt to download")
TOOLCHAIN = option.add(
    "toolchain", Path, default_factory=lambda: _get_default_toolchain(), doc="The toolchain to use for the build"
)
BUILD_DIR = option.add(
    "build-dir",
    Path,
    default=HERE.parent.joinpath("_build/_dagon"),
    doc="The directory in which build results will be written",
)


def _get_default_toolchain() -> Path:
    if os.name == "posix":
        return HERE / "tools/gcc-11.yaml"
    raise RuntimeError(
        f"No default toolchain is defined for this platform. "
        f'Please define one and pass the "--option=toolchain=<tc-filepath>"'
    )


def _get_bpt_url() -> str:
    use = USE_BPT.get()
    if use is not None:
        return use.as_uri()
    filename = {
        "win32": "bpt-win-x64.exe",
        "linux": "bpt-linux-x64",
        "darwin": "bpt-macos-x64",
    }.get(sys.platform)
    if filename is None:
        raise RuntimeError(
            f'We do not have a pre-built bpt to download for "{sys.platform}". Obtain a '
            "suitable bpt executable and specify it using `--option=bpt=<filepath>` on the Dagon command line."
        )
    url = f"https://github.com/vector-of-bool/bpt/releases/download/1.0.0-beta.1/{filename}"
    return url


__bpt_dl = http.download_task(".bpt.dl", _get_bpt_url, doc="Downloads a BPT for the current system")

clean = task.fn_task("clean", lambda: fs.remove(BUILD_DIR.get(), recurse=True, absent_ok=True))


@task.define(depends=[__bpt_dl])
async def __bpt_exe() -> Path:
    p = await task.result_of(__bpt_dl)
    os.chmod(p, 0o755)
    return p


def _progress(e: proc.ProcessOutputItem):
    line = e.out.decode()
    ui.print(line.rstrip())
    mat = re.search(r"\[\s*(\d+)/(\d+)\]", line)
    if not mat:
        return
    cur, total = mat.groups()
    ui.progress(int(cur) / int(total))


@task.define(depends=[__bpt_exe], order_only_depends=[clean])
async def build() -> None:
    ui.status("Compiling, linking, and running tests")
    await proc.run(
        [
            await task.result_of(__bpt_exe),
            "build",
            ("--out", BUILD_DIR.get()),
            ("--toolchain", TOOLCHAIN.get()),
        ],
        on_output=_progress,
    )
    ui.print("All build and unit tests ran successfully", type=ui.MessageType.Information)


@task.define()
async def docs() -> None:
    await proc.run(["sphinx-build", HERE / "docs", HERE / "_build/docs", "-jauto", "-Wa"], on_output="status")
