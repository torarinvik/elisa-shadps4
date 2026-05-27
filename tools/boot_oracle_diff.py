#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path


TOKEN_RE = re.compile(r"([A-Za-z0-9_]+)=([^ \t]+)")


@dataclass(frozen=True)
class Record:
    kind: str
    key: str
    fields: dict[str, str]
    line: str


def parse_kv(payload: str) -> dict[str, str]:
    return {key: value for key, value in TOKEN_RE.findall(payload)}


def normalize_fields(kind: str, fields: dict[str, str]) -> dict[str, str]:
    normalized = dict(fields)
    if kind == "module":
        if "name" not in normalized and "module" in normalized:
            normalized["name"] = normalized["module"]
        if "module" not in normalized and "name" in normalized:
            normalized["module"] = normalized["name"]
    elif kind == "entry":
        if "addr" not in normalized and "entry" in normalized:
            normalized["addr"] = normalized["entry"]
        if "entry" not in normalized and "addr" in normalized:
            normalized["entry"] = normalized["addr"]
    elif kind == "resolve":
        if "symbol" not in normalized and "name" in normalized:
            normalized["symbol"] = normalized["name"]
        if "name" not in normalized and "symbol" in normalized:
            normalized["name"] = normalized["symbol"]
        if "resolved" not in normalized and "resolved_addr" in normalized:
            normalized["resolved"] = normalized["resolved_addr"]
        if "resolved_addr" not in normalized and "resolved" in normalized:
            normalized["resolved_addr"] = normalized["resolved"]
        if "kind" not in normalized and "resolution" in normalized:
            normalized["kind"] = normalized["resolution"]
        if "resolution" not in normalized and "kind" in normalized:
            normalized["resolution"] = normalized["kind"]
    return normalized


def parse_boot_oracle(path: Path) -> list[Record]:
    records: list[Record] = []
    if not path.exists():
        raise FileNotFoundError(path)
    for raw_line in path.read_text(errors="replace").splitlines():
        # Tolerate a logger prefix (e.g. "[time] <Info> Core_Linker: BOOT_ORACLE ...")
        # so the trace can be grepped straight out of shadPS4's log file.
        pos = raw_line.find("BOOT_ORACLE ")
        if pos < 0:
            continue
        line = raw_line[pos:].strip()
        parts = line.split(maxsplit=2)
        if len(parts) < 2:
            continue
        kind = parts[1]
        fields = normalize_fields(kind, parse_kv(parts[2] if len(parts) > 2 else ""))
        key = oracle_key(kind, fields, len(records))
        records.append(Record(kind, key, fields, line))
    return records


def parse_elisa_artifacts(path: Path) -> list[Record]:
    records: list[Record] = []
    facts: dict[str, str] = {}
    if not path.exists():
        raise FileNotFoundError(path)
    for raw_line in path.read_text(errors="replace").splitlines():
        line = raw_line.strip()
        if not line.startswith("CUSA07399_ARTIFACT "):
            continue
        payload = line.split(" ", 1)[1]
        fields = parse_kv(payload)
        facts.update(fields)
        if "module" in fields and "host" in fields and "imports" in fields:
            fields = normalize_fields("module", fields)
            key = f"module:{fields.get('index', len(records))}"
            records.append(Record("module", key, fields, line))
        elif payload.startswith("resolve ") or payload.startswith("fallback_import") or payload.startswith("import_probe "):
            fields = normalize_fields("resolve", fields)
            key = resolve_key(fields, len(records))
            records.append(Record("resolve", key, fields, line))
        elif payload.startswith("trace_event "):
            key = f"trace:{fields.get('index', len(records))}"
            records.append(Record("trace", key, fields, line))

    if "entry" in facts or "main_entry" in facts:
        entry_fields = normalize_fields("entry", {
            "entry": facts.get("entry", "0"),
            "main_entry": facts.get("main_entry", "0"),
            "entry_native_prot": facts.get("guest_exec_main_entry_native_prot", ""),
        })
        records.append(Record("entry", "entry", entry_fields, "CUSA07399_ARTIFACT synthesized entry"))

    hle_module = facts.get("guest_exec_runtime_hle_module") or facts.get("last_hle_module")
    hle_symbol = facts.get("guest_exec_runtime_hle_symbol") or facts.get("last_hle_symbol")
    hle_return = facts.get("guest_exec_runtime_hle_guest_return_addr") or facts.get("guardrail_runtime_hle_saved_return")
    if hle_module or hle_symbol or hle_return:
        hle_fields = {
            "seq": facts.get("guest_exec_runtime_hle_sequence", "1"),
            "module": hle_module or "",
            "symbol": hle_symbol or "",
            "return": hle_return or "0",
            "rdi": facts.get("guest_exec_runtime_hle_arg0", "0"),
            "rsi": facts.get("guest_exec_runtime_hle_arg1", "0"),
            "rdx": facts.get("guest_exec_runtime_hle_arg2", "0"),
        }
        records.append(Record("hle", f"hle:{hle_fields['seq']}", hle_fields, "CUSA07399_ARTIFACT synthesized hle"))

    return records


def oracle_key(kind: str, fields: dict[str, str], index: int) -> str:
    if kind == "module":
        return f"module:{fields.get('index') or fields.get('name') or fields.get('module') or index}"
    if kind == "entry":
        return "entry"
    if kind == "hle":
        return f"hle:{fields.get('seq', index)}"
    if kind == "resolve":
        return resolve_key(fields, index)
    if kind == "trace":
        return f"trace:{fields.get('index', index)}"
    return f"{kind}:{fields.get('seq') or fields.get('index') or index}"


def resolve_key(fields: dict[str, str], index: int) -> str:
    nid = fields.get("nid")
    if nid:
        return f"resolve:{nid}"
    return f"resolve:{fields.get('index', index)}"


def canonical_int(text: str) -> int | None:
    try:
        if text == "":
            return None
        return int(text, 0)
    except ValueError:
        return None


def values_equal(left: str, right: str) -> bool:
    li = canonical_int(left)
    ri = canonical_int(right)
    if li is not None and ri is not None:
        return li == ri
    return left == right


def compare_records(expected: list[Record], actual: list[Record], limit: int) -> int:
    expected_by_key = {record.key: record for record in expected}
    actual_by_key = {record.key: record for record in actual}

    checked = 0
    for exp in expected:
        if limit and checked >= limit:
            break
        checked += 1
        act = actual_by_key.get(exp.key)
        if act is None:
            print(f"FIRST_DIVERGENCE missing-actual key={exp.key}")
            print(f"expected: {exp.line}")
            return 1
        for field, expected_value in exp.fields.items():
            if field in {"path", "host", "image"}:
                continue
            actual_value = act.fields.get(field)
            if actual_value is None:
                continue
            if not values_equal(expected_value, actual_value):
                print(f"FIRST_DIVERGENCE field-mismatch key={exp.key} field={field}")
                print(f"expected: {expected_value}  ({exp.line})")
                print(f"actual:   {actual_value}  ({act.line})")
                return 1

    for act in actual:
        if limit and checked >= limit:
            break
        if act.key not in expected_by_key and act.kind in {"module", "entry", "hle"}:
            print(f"FIRST_DIVERGENCE extra-actual key={act.key}")
            print(f"actual: {act.line}")
            return 1

    print(f"BOOT_ORACLE_DIFF_OK compared={checked}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Diff shadPS4 BOOT_ORACLE trace lines against Elisa CUSA artifacts.")
    parser.add_argument("--expected", required=True, help="shadPS4 BOOT_ORACLE trace file")
    parser.add_argument("--actual", default="cusa07399_artifacts.txt", help="Elisa CUSA07399 artifact file")
    parser.add_argument("--limit", type=int, default=0, help="maximum expected records to compare")
    args = parser.parse_args()

    expected = parse_boot_oracle(Path(args.expected))
    actual = parse_elisa_artifacts(Path(args.actual))
    if not expected:
        print("No BOOT_ORACLE records found in expected trace.")
        return 2
    if not actual:
        print("No Elisa artifact records found in actual trace.")
        return 2
    return compare_records(expected, actual, args.limit)


if __name__ == "__main__":
    raise SystemExit(main())
