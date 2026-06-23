# PENDING project.json change: arm -Wprogress on the emulator target

The progress-safety guard (docs/102) is now clean on `emulator-cusa07399-x64-exec`
(88 → 0 progress warnings). To keep it armed so no new un-storied `while` loop can be
added silently, add a `warnings.progress` flag to the target.

Per the standing rule, project.json is NOT edited directly. Apply this stanza to the
`emulator-cusa07399-x64-exec` target's `"warnings"` object (create it if absent):

    "warnings": { "progress": true }

(Equivalent to passing `-Wprogress`; rolls up under `-Wstrict`, where the obligation
becomes a hard error.)
