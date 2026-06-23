# lldb command: dump the tail of the guest->HLE call ring from a live (or stalled) emulator
# process. The ring stores the symbol-name C-string pointer of every guest->HLE boundary
# crossing (core/linker.elisa). Reading its tail names the last calls the guest made before it
# wedged — the service it is now busy-waiting on.
#
#   lldb --batch -p <pid> -o "command script import tools/dump_hle_ring.py" -o "hle_ring"
import lldb


def _sym_addr(target, name):
    syms = target.FindSymbols(name)
    if not syms.GetSize():
        return None
    return syms.GetContextAtIndex(0).symbol.GetStartAddress().GetLoadAddress(target)


def hle_ring(debugger, command, result, internal_dict):
    target = debugger.GetSelectedTarget()
    process = target.GetProcess()
    err = lldb.SBError()
    n = 1024
    idx_a = _sym_addr(target, "linker_hle_call_ring_index")
    ring_a = _sym_addr(target, "linker_hle_call_ring")
    tgt_a = _sym_addr(target, "linker_hle_call_target_ring")
    rdi_a = _sym_addr(target, "linker_hle_call_rdi_ring")
    if idx_a is None or ring_a is None:
        print("hle_ring: symbols not found")
        return
    idx = process.ReadUnsignedFromMemory(idx_a, 8, err)
    print("HLE ring index = %d  (last %d calls, most-recent first):" % (idx, min(idx, 30)))
    if idx == 0:
        print("   <ring empty — recorder stub not active in this build>")
        return
    seen = 0
    for k in range(min(idx, 30)):
        i = (idx - 1 - k) % n
        ptr = process.ReadUnsignedFromMemory(ring_a + i * 8, 8, err)
        if ptr == 0:
            continue
        name = process.ReadCStringFromMemory(ptr, 160, err)
        extra = ""
        if tgt_a is not None:
            t = process.ReadUnsignedFromMemory(tgt_a + i * 8, 8, err)
            extra += " target=0x%x" % t
        if rdi_a is not None:
            r = process.ReadUnsignedFromMemory(rdi_a + i * 8, 8, err)
            extra += " rdi=0x%x" % r
        print("   %2d  %-44s%s" % (k, name if name else "<unreadable>", extra))
        seen += 1
    if seen == 0:
        print("   <all tail entries null>")


def __lldb_init_module(debugger, internal_dict):
    debugger.HandleCommand("command script add -f dump_hle_ring.hle_ring hle_ring")
