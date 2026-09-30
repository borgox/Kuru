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
frame's `Code` pointer so the real call re-evaluates the arguments untouched. Because it uses the game's own
handlers, it never needs a table of what opcodes mean, which keeps it working across the fork's opcode changes.

### Editing them: write-back

When a handler evaluates an argument that's an l-value (a variable, an out-param), it leaves that argument's
real memory address in a global, `GPropAddr`, as a side effect. Kuru captures that address per argument while
reading. After a pre hook edits the reconstructed buffer, Kuru writes the changed bytes back to those
addresses; the native then re-reads the arguments and sees the new values.

The limit of write-back is literals: a constant baked into the bytecode has no address to write to, so it can't
be edited this way.

### Editing them: re-invoke

For a true native - one whose `Func` is real native code - Kuru can do better. Instead of patching argument
addresses it runs the native itself through `ProcessEvent`, passing the edited buffer. `ProcessEvent` copies
that buffer into the native's locals, so the native reads *every* argument from it, literals included, and
writes its out-parameters and return back through it. Kuru then advances the caller's `Code` past the inline
call so the original doesn't run a second time. Body-less stubs - which have no real native to run - stay on
write-back.

## Cancel, catch-out, edit-out

The same machinery gives the rest of the flow:

- **Cancel** - advance `Code` past the arguments and the end-of-parameters token, zero the return, and skip the
  call.
- **Catch output** - after the call, out-parameters live at the addresses captured while reading, so Kuru
  re-reads them into the buffer and a post hook sees the produced values.
- **Edit output** - a post hook's changes to out-parameters, or the return, are written back the same way.

## The result

Because of all this, the flow a mod actually uses - edit inputs, cancel, read outputs, edit outputs - is the
same three calls (`Params`, `Result`, `Block`) whether the function runs through the event system, the
interpreter, or a native call. The layer differences are handled here so they never reach the mod.
