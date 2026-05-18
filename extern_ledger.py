#!/usr/bin/env python3
from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parent
EXTERN_RE = re.compile(r"^extern\s+([A-Za-z_][A-Za-z0-9_]*)")

RULES: list[tuple[str, str, str]] = [
    ("compiler-intrinsic", r"^(popcount_u64|byteswap_u16|byteswap_u32|byteswap_u64|x86_mm_pause)$", "LLVM or target CPU intrinsic"),
    ("os-c-api", r"^(GetModuleHandleA|GetProcAddress|windows_.*|posix_.*|apple_.*|freebsd_.*|bsd_.*|linux_.*|gnu_.*|xsi_.*|time|posix_errno|FencedRDTSC|common_sleep_for_ms|high_resolution_now_ns|atomic_flag_.*|debug_breakpoint|x86_debug_break|arm64_debug_break|arm64_msvc_yield|arm64_asm_yield)$", "OS, libc, thread, clock, atomics, or debug runtime boundary"),
    ("external-c-api", r"^(ZYAN_SUCCESS|Zydis.*|Discord_.*|av_err2string)$", "Linked third-party API with C-compatible surface"),
    ("external-cpp-shim", r"^(spdlog_.*|tracy_.*|profiler_connected|aes_.*|sha1_.*|format_.*|elf_info_.*|slab_heap_.*|object_pool_.*|unique_function_.*|scope_guard_call|shared_first_mutex_.*|adaptive_mutex_.*|recursive_mutex_.*|range_item_.*|va_arg_read|bit_array_count_true_avx2|trace_.*|stop_state_alloc|jthread_.*)$", "External C++ or runtime facility that must be exposed through an extern \"C\" shim"),
    ("project-owned-to-port", r"^(config_.*|patcher_.*|key_file_.*|key_manager_.*|alloc_key_manager|make_playstation_store_image_url|io_.*|fs_.*|common_log_.*|kernel_current_thread_name|pthread_current_thread_name|serdes_.*|u32_bits_to_f32|half_u16_to_f32|fmt_print.*|multiply_.*|divide_128_on_32)$", "Project-owned logic or project wrapper; port to Elisa unless it is reduced to a true OS/library C boundary"),
    ("build-metadata", r"^g_(version|is_release|scm_rev|scm_branch|scm_desc|scm_remote_name|scm_remote_url|scm_date)$", "Build metadata injected by build system"),
    ("windows-wide-string-shim", r"^(wide_to_utf8|utf8_to_wide)$", "Windows UTF conversion boundary"),
]


def classify(symbol: str) -> tuple[str, str] | None:
    for category, pattern, note in RULES:
        if re.match(pattern, symbol):
            return category, note
    return None


def iter_externs() -> list[tuple[Path, int, str]]:
    externs: list[tuple[Path, int, str]] = []
    for path in sorted((ROOT / "common").rglob("*.elisa")):
        for line_no, line in enumerate(path.read_text().splitlines(), 1):
            match = EXTERN_RE.match(line)
            if match:
                externs.append((path.relative_to(ROOT), line_no, match.group(1)))
    return externs


def main() -> int:
    externs = iter_externs()
    missing: list[tuple[Path, int, str]] = []
    by_category: dict[str, int] = {}
    for path, line_no, symbol in externs:
        result = classify(symbol)
        if result is None:
            missing.append((path, line_no, symbol))
            continue
        category, _ = result
        by_category[category] = by_category.get(category, 0) + 1

    if "--summary" in sys.argv:
        print(f"externs: {len(externs)}")
        for category in sorted(by_category):
            print(f"{category}: {by_category[category]}")

    if missing:
        for path, line_no, symbol in missing:
            print(f"unclassified extern {symbol} at {path}:{line_no}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
