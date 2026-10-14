# Editing parameters (internals)

The [editing parameters](../guides/editing-parameters.md) guide showed the simple API. This page is how it
works - and why it's harder than it looks for one class of function.

## The easy case: a real buffer

At `ProcessEvent` and `ProcessInternal` the parameters are a real buffer - the flat `Parms` blob, or the
frame's `Locals`. A hook gets a pointer to it, so reading and editing are just reading and writing memory. The
function reads the same buffer, so your edits are what it sees. Nothing clever is needed.

## The hard case: native calls at CallFunction

A native function invoked inline from bytecode has no buffer. It reads its arguments straight from the caller's
bytecode as it runs (see [the VM](04-the-unrealscript-vm.md)). At `CallFunction`, when a hook wants the
parameters, there's nothing to point at yet.

### Reading the arguments

Kuru reconstructs them by *borrowing the interpreter*. For each parameter it reads the argument's opcode and
calls `GNatives[opcode]` - the game's own handler - to evaluate that argument into a buffer, then rewinds the
frame's `Code` pointer. If the original inline call runs, it evaluates those expressions again.
Rewinding does not undo their side effects or restore every interpreter global. Arguments that call
functions or mutate state can have their effects repeated when the original call evaluates them again.
Using live handlers avoids a handwritten opcode table, but still depends on the right interpreter ABI.

### Editing them: write-back

When a handler evaluates an argument that's an l-value (a variable, an out-param), it leaves that argument's
real memory address in a global, `GPropAddr`, as a side effect. Kuru captures that address per argument while
reading. After a pre hook edits the reconstructed buffer, Kuru writes the changed bytes back to those
addresses; the native then re-reads the arguments and sees the new values.

The limit of write-back is literals: a constant baked into the bytecode has no address to write to, so it can't
be edited this way.

### Editing them: re-invoke

For an eligible native, Kuru can use a different path. Instead of patching argument
addresses it runs the native itself through `ProcessEvent`, passing the edited buffer. `ProcessEvent` copies
that buffer into the native's locals, so the native reads *every* argument from it, literals included, and
writes its out-parameters and return back through it. Kuru then advances the caller's `Code` past the inline
call. Eligibility requires reconstructed arguments, a known argument end, `FUNC_Native`, `iNative == 0`,
nonempty native parameter stub bytecode, and `ParmsSize <= 1024`. Other cases use inline write-back.
The code also falls back after a reinvocation fault; a fault does not prove that no side effect occurred
before the failure.

## Cancel, catch-out, edit-out

The same machinery gives the rest of the flow:

- **Cancel** - when argument evaluation found the end position, advance `Code` past it and the
  end-of-parameters token, zero the return slot where present, and skip the call. Without that position,
  the CallFunction path still calls the original despite `Block()`.
- **Catch output** - after the call, out-parameters live at the addresses captured while reading, so Kuru
  re-reads them into the buffer and a post hook sees the produced values.
- **Edit output** - a post hook's changes to out-parameters, or the return, are written back the same way.

## Current limits

`Params`, `Result`, and `Block` share one API across layers, but the support differs. CallFunction uses
a 1024-byte scratch buffer and tracks at most 32 argument slots for write-back. Evaluation failure can
leave params null; oversized blocks can be truncated. Strings, arrays, delegates, and other nontrivial
values are copied as bytes here, not deeply cloned with engine ownership semantics. Only use layouts
and argument types verified for your function. Do not retain buffer pointers past the callback.

For blocks over 1024 bytes, an early bounds break can still leave a success-looking argument-end
pointer inside the stream. Do not inspect/edit/block those CallFunction captures: `Block()` may
advance to the wrong bytecode position. A null check is not sufficient.

Source: `EvaluateCallArgs`, `ReInvokeNative`, and `DetourCallFunction` in
[HookEngine.cpp](../../../libs/KuruLoader/src/HookEngine.cpp).
