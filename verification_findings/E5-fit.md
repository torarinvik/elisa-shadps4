# E5 Fit Finding — Symbol Key Eq/Hash Protocols

Declined protocol inheritance/default-contract implementation for the current
symbol-resolution key surface.

The real export index key type is `static u8&`, not a custom `Nid` or
`SymbolName` wrapper:

- `core/module.elisa` defines `ModuleExportSlot.key: mutable static u8&`.
- `Module_ExportIndexInsert` hashes keys with `Module_CstrHash(key)` and tests
  equality with `streq(existing.key, key)`.
- `Module_FindDefinedSymbolAddress` probes the same raw-C-string index with
  `Module_CstrHash(name)` and `streq(entry.key, name)`.
- HLE and AeroLib resolution likewise compare NID/name strings with `streq` or
  `AeroLib_StrCmp` over `static u8&`.

Under the shared fit test, adding `Eq` / `Hashable` for `Nid` or `SymbolName`
would be decorative: those custom key types do not exist in the real symbol
dict, and the existing correctness obligation is already the local pairing of
`Module_CstrHash(static u8&)` with `streq(static u8&, static u8&)`.
