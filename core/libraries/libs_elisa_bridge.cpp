// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// FFI bridge for the Elisa port: thin C-ABI wrapper that delegates HLE library
// registration to shadPS4's Libraries::InitHLELibs. Bound from Elisa via
// core/libraries/libs_ffi.elisa (@link_name(LibrariesElisa_InitHLELibs) ->
// c_Libraries_InitHLELibs), which core/libraries/libs.elisa::InitHLELibs calls.
//
// Reconstructed faithfully from the FFI contract + shadPS4 oracle signature
// (Libraries::InitHLELibs(Core::Loader::SymbolsResolver*), src/core/libraries/libs.h)
// after the original local-only (untracked) copy was lost; now tracked so it
// cannot be lost again.

#include "core/libraries/libs.h"
#include "core/loader/symbols_resolver.h"

extern "C" {

void LibrariesElisa_InitHLELibs(void* sym) {
    Libraries::InitHLELibs(static_cast<Core::Loader::SymbolsResolver*>(sym));
}

} // extern "C"
