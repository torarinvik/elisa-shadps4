// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdint.h>
#include <zlib.h>

int elisa_zlib_uncompress(const void *src,
                          uint32_t src_len,
                          void *dst,
                          uint32_t dst_cap,
                          uint32_t *out_len,
                          int *out_zlib_ret) {
    if (!src || src_len == 0 || !dst || dst_cap == 0 || !out_len || !out_zlib_ret) {
        return -1;
    }

    uLongf produced = (uLongf)dst_cap;
    int ret = uncompress((Bytef *)dst, &produced, (const Bytef *)src, (uLong)src_len);
    *out_len = (uint32_t)produced;
    *out_zlib_ret = ret;
    return 0;
}
