"""Static HTML views for the MINI-8 simulator."""
from __future__ import annotations

from html import escape
from typing import Iterable

from IPython.display import HTML, display

if __package__:
    from .mini8 import Machine, Mini8Error, Snapshot, Step, assemble, format_instruction
else:
    from mini8 import Machine, Mini8Error, Snapshot, Step, assemble, format_instruction


_STYLE = """
<style>
.mini8-panel {
  --m8-bg:#f7f9fc; --m8-card:#fff; --m8-text:#172333; --m8-muted:#526176;
  --m8-line:#cbd5e1; --m8-accent:#134e8b; --m8-change:#e1f2e8;
  --m8-change-text:#135731; --m8-jump:#fff0cf; --m8-jump-text:#754a00;
  color:var(--m8-text); background:var(--m8-bg); border:1px solid var(--m8-line);
  border-radius:12px; padding:18px; margin:12px 0; max-width:1100px;
  font:14px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;
}
.mini8-panel * { box-sizing:border-box; }
.mini8-panel h3 { color:var(--m8-text); margin:0 0 12px; font-size:18px; }
.mini8-panel h4 { color:var(--m8-text); margin:18px 0 8px; font-size:14px; }
.mini8-panel p { margin:8px 0; }
.mini8-panel code { background:none; color:inherit; padding:0; font-size:13px; }
.mini8-panel .m8-muted { color:var(--m8-muted); }
.mini8-panel .m8-registers { display:flex; flex-wrap:wrap; gap:10px; margin:14px 0; }
.mini8-panel .m8-register {
  flex:1 1 120px; padding:10px 14px; background:var(--m8-card);
  border:1px solid var(--m8-line); border-radius:8px;
}
.mini8-panel .m8-register strong { font-size:26px; display:block; line-height:1.3; }
.mini8-panel .m8-register code { display:block; letter-spacing:1px; }
.mini8-panel .m8-register small { display:block; color:var(--m8-muted); }
.mini8-panel .m8-meta { display:flex; flex-wrap:wrap; gap:8px 24px; }
.mini8-panel .m8-badge {
  display:inline-block; padding:2px 8px; border-radius:5px;
  border:1px solid var(--m8-line); background:var(--m8-card);
}
.mini8-panel .m8-scroll { overflow-x:auto; }
.mini8-panel table { border-collapse:collapse; width:100%; color:var(--m8-text); font-size:13px; }
.mini8-panel caption { text-align:left; color:var(--m8-muted); padding:0 0 8px; }
.mini8-panel th, .mini8-panel td {
  border:1px solid var(--m8-line); padding:7px 9px; text-align:left;
  vertical-align:top; background:var(--m8-card); white-space:nowrap;
}
.mini8-panel th { font-weight:600; background:var(--m8-bg); }
.mini8-panel .m8-changed { background:var(--m8-change); color:var(--m8-change-text); font-weight:600; }
.mini8-panel .m8-jump { background:var(--m8-jump); color:var(--m8-jump-text); font-weight:600; }
.mini8-panel .m8-current { border-left:4px solid var(--m8-accent); }
.mini8-panel .m8-memory { display:flex; flex-wrap:wrap; gap:7px; }
.mini8-panel .m8-cell {
  min-width:95px; background:var(--m8-card); border:1px solid var(--m8-line);
  border-radius:6px; padding:7px 10px;
}
.mini8-panel .m8-cell span { display:block; color:var(--m8-muted); font-size:12px; }
.mini8-panel .m8-cell strong { font-size:18px; }
.mini8-panel .m8-cell code { display:block; font-size:11px; }
.mini8-panel .m8-cell.m8-changed { background:var(--m8-change); }
.mini8-panel .m8-note { border-left:3px solid var(--m8-accent); padding-left:10px; }
@media (prefers-color-scheme:dark) {
  .mini8-panel {
    --m8-bg:#192330; --m8-card:#202e3c; --m8-text:#eef3fa; --m8-muted:#b6c6d8;
    --m8-line:#526276; --m8-accent:#91c8ff; --m8-change:#173f31;
    --m8-change-text:#b2f4cd; --m8-jump:#4b3916; --m8-jump-text:#ffe1a0;
  }
}
</style>
"""


def _integer(value: int, name: str, minimum: int, maximum: int | None = None) -> int:
    """Reject booleans and non-integers before applying a numeric bound."""
    if isinstance(value, bool) or not isinstance(value, int):
        raise Mini8Error(f"{name} must be an integer.")
    if value < minimum or (maximum is not None and value > maximum):
        bounds = f"between {minimum} and {maximum}" if maximum is not None else f"at least {minimum}"
        raise Mini8Error(f"{name} must be {bounds}.")
    return value


def _comparison(value: int) -> str:
    return {-1: "&lt;", 0: "=", 1: "&gt;"}[value]


def _output(values: Iterable[int]) -> str:
    return ", ".join(str(value) for value in values) or "—"


class CPU:
    """Notebook interface to a persistent MINI-8 machine."""

    def __init__(self, source: str, watch: tuple[int, ...] = (), title: str = "MINI-8"):
        if not isinstance(source, str):
            raise Mini8Error("The program must be assembly source text.")
        if not isinstance(title, str):
            raise Mini8Error("The processor title must be text.")
        self._code = assemble(source)
        try:
            addresses = tuple(watch)
        except TypeError as exc:
            raise Mini8Error("Watched addresses must be a sequence of integers.") from exc
        self._watch = tuple(dict.fromkeys(_integer(address, "Address", 0, 255) for address in addresses))
        self._title = title
        self._machine = Machine(self._code)
        self._history: list[Step] = []
        self._initial = self._machine.state()

    @property
    def registers(self) -> tuple[int, ...]:
        """The four register values, returned as an immutable tuple."""
        return self._machine.state().registers

    @property
    def output(self) -> tuple[int, ...]:
        """All values emitted by OUT since the latest reset."""
        return self._machine.state().output

    @property
    def pc(self) -> int:
        """Next instruction index; the HALT index after stopping."""
        return self._machine.state().pc

    @property
    def steps(self) -> int:
        """The number of executed instructions, including HALT."""
        return self._machine.state().steps

    @property
    def halted(self) -> bool:
        """Whether HALT has executed."""
        return self._machine.state().halted

    def _display(self, body: str, subtitle: str = "") -> None:
        heading = escape(self._title)
        if subtitle:
            heading += " · " + escape(subtitle)
        display(HTML(_STYLE + '<section class="mini8-panel" aria-label="MINI-8 processor">'
                     f"<h3>{heading}</h3>{body}</section>"))

    def _memory_html(self, state: Snapshot, addresses: Iterable[int], before: Snapshot | None = None) -> str:
        cells = []
        for address in addresses:
            value = state.memory[address]
            changed = before is not None and value != before.memory[address]
            css = "m8-cell m8-changed" if changed else "m8-cell"
            previous = f" · was {before.memory[address]}" if changed else ""
            cells.append(f'<div class="{css}"><span>Address [{address}]{previous}</span>'
                         f"<strong>{value}</strong><code>{value:08b}</code></div>")
        return '<div class="m8-memory">' + "".join(cells) + "</div>"

    def _state_html(self, before: Snapshot | None = None) -> str:
        state = self._machine.state()
        cards = []
        for index, value in enumerate(state.registers):
            changed = before is not None and before.registers[index] != value
            css = "m8-register m8-changed" if changed else "m8-register"
            previous = f"was {before.registers[index]} → {value}" if changed else "decimal / 8 bits"
            cards.append(f'<div class="{css}"><span>R{index}</span><strong>{value}</strong>'
                         f"<code>{value:08b}</code><small>{previous}</small></div>")
        comparison_css = "m8-badge m8-changed" if before and before.comparison != state.comparison else "m8-badge"
        status = "Stopped (HALT)" if state.halted else "Ready for the next step"
        body = (f'<div class="m8-meta"><span><b>PC</b> = {state.pc}</span>'
                f'<span><b>Executed instructions</b> = {state.steps}</span>'
                f'<span><b>CMP flag</b> <span class="{comparison_css}">{_comparison(state.comparison)}</span></span>'
                f'<span class="m8-badge">{status}</span></div>'
                '<div class="m8-registers">' + "".join(cards) + "</div>")
        output_css = "m8-changed" if before and before.output != state.output else ""
        body += f'<p><b>OUT output:</b> <code class="{output_css}">{_output(state.output)}</code></p>'
        if self._watch:
            body += "<h4>Watched data memory</h4>" + self._memory_html(state, self._watch, before)
        if state.halted:
            body += '<p class="m8-note">HALT has executed. The processor is waiting for <code>reset()</code>.</p>'
        elif 0 <= state.pc < len(self._machine.program):
            instruction = escape(format_instruction(self._machine.program[state.pc]))
            body += f'<p class="m8-note"><b>Next instruction {state.pc}:</b> <code>{instruction}</code></p>'
        else:
            body += f'<p class="m8-note"><b>PC = {state.pc} is outside the program.</b> HALT or a jump is missing.</p>'
        body += '<p class="m8-muted">PC is an instruction number. Only CMP changes the comparison flag; after reset it is =.</p>'
        return body

    def show(self) -> None:
        """Display the current processor state without executing an instruction."""
        self._display(self._state_html())

    def _history_html(self, events: list[Step], initial: Snapshot, limit: int = 60) -> str:
        headers = ["Step", "Executed instruction", "PC before → after", "R0", "R1", "R2", "R3", "CMP", "Memory write", "New output"]
        body = '<div class="m8-scroll"><table><caption>Each instruction row shows the state after execution. Green marks a change; yellow marks a taken jump.</caption><thead><tr>'
        body += "".join(f'<th scope="col">{header}</th>' for header in headers) + "</tr></thead><tbody>"
        initial_cells = [str(initial.steps), "Initial state", str(initial.pc)]
        initial_cells += [str(value) for value in initial.registers]
        initial_cells += [_comparison(initial.comparison), "—", _output(initial.output)]
        body += "<tr>" + "".join(f"<td>{value}</td>" for value in initial_cells) + "</tr>"
        omitted = max(0, len(events) - limit)
        if omitted:
            first, last = events[0].after.steps, events[omitted - 1].after.steps
            body += (f'<tr><td colspan="10" class="m8-muted">Hidden {omitted} instructions (steps {first}–{last}). '
                     f"Showing the last {limit} instructions; the full history remains available through history(limit=…).</td></tr>")
        for event in events[omitted:]:
            before, after = event.before, event.after
            instruction = escape(format_instruction(event.instruction))
            opcode = event.instruction[0]
            # A taken jump to the next instruction still counts as a jump.
            taken_jump = (opcode == 0x08
                          or opcode == 0x09 and before.comparison == 0
                          or opcode == 0x0A and before.comparison != 0
                          or opcode == 0x0B and before.comparison < 0
                          or opcode == 0x0C and before.comparison >= 0)
            pc_css = ' class="m8-jump"' if taken_jump else ""
            pc_note = " ↶ jump" if taken_jump else ""
            if opcode in {0x09, 0x0A, 0x0B, 0x0C} and not taken_jump:
                pc_note = " · no jump"
            if after.halted:
                pc_note = " · HALT"
            body += (f"<tr><td>{after.steps}</td><td><code>{instruction}</code></td>"
                     f"<td{pc_css}>{before.pc} → {after.pc}{pc_note}</td>")
            for old, value in zip(before.registers, after.registers):
                css = f' class="m8-changed" title="{old} → {value}"' if old != value else ""
                body += f"<td{css}>{value}</td>"
            flag_css = ' class="m8-changed"' if before.comparison != after.comparison else ""
            body += f"<td{flag_css}>{_comparison(after.comparison)}</td>"
            memory_changes = [f"[{address}]: {old} → {new}" for address, (old, new)
                              in enumerate(zip(before.memory, after.memory)) if old != new]
            # STORE may deliberately write the value that was already present.
            if opcode == 0x03 and not memory_changes:
                address = event.instruction[2]
                memory_changes = [f"[{address}]: {after.memory[address]} (unchanged)"]
            memory_css = ' class="m8-changed"' if before.memory != after.memory else ""
            emitted = after.output[len(before.output):]
            output_css = ' class="m8-changed"' if emitted else ""
            body += f'<td{memory_css}>{"<br>".join(memory_changes) or "—"}</td><td{output_css}>{_output(emitted)}</td></tr>'
        return body + "</tbody></table></div>"

    def _execute(self, count: int, require_halt: bool) -> None:
        before = self._machine.state()
        events: list[Step] = []
        try:
            for _ in range(count):
                if self.halted:
                    break
                event = self._machine.step()
                events.append(event)
                self._history.append(event)
        except Mini8Error:
            body = '<p class="m8-note">Execution stopped with an error. The state below reflects the last completed instruction.</p>'
            self._display(body + self._history_html(events, before) + self._state_html(before), "Program execution")
            raise
        body = self._history_html(events, before) if events else "<p>The processor has already stopped. No instruction was executed.</p>"
        if require_halt and not self.halted:
            body += f'<p class="m8-note"><b>Reached the limit of {count} instructions without HALT.</b> The state is preserved for further inspection.</p>'
        self._display(body + "<h4>Current processor state</h4>" + self._state_html(before), "Program execution")
        if require_halt and not self.halted:
            raise Mini8Error(f"The program did not reach HALT within {count} additional instructions. It may contain an infinite loop.")

    def step(self, count: int = 1) -> None:
        """Execute up to count instructions, stop at HALT, and show transitions."""
        self._execute(_integer(count, "Step count", 1), require_halt=False)

    def run(self, max_steps: int = 1000) -> None:
        """Continue the current machine through HALT, with a per-call step limit."""
        self._execute(_integer(max_steps, "Step limit", 1), require_halt=True)

    def reset(self) -> None:
        """Reload the same program, clear history and data, and show fresh state."""
        self._machine = Machine(self._code)
        self._history.clear()
        self._initial = self._machine.state()
        self._display(self._state_html(), "After reset")

    def history(self, limit: int = 60) -> None:
        """Show the initial state and the last limit steps."""
        limit = _integer(limit, "History limit", 1)
        self._display(self._history_html(self._history, self._initial, limit), "Step history")

    def listing(self) -> None:
        """Show instruction numbers, byte offsets, encoded bytes and assembly."""
        body = ('<div class="m8-scroll"><table><caption>Each instruction occupies 3 bytes. '
                'PC counts instructions; the byte offset is 3 × PC. Byte values are hexadecimal.</caption>'
                '<thead><tr><th scope="col">Instruction number (PC)</th><th scope="col">Byte offset</th>'
                '<th scope="col">Opcode</th><th scope="col">A</th><th scope="col">B</th>'
                '<th scope="col">Decoded instruction</th></tr></thead><tbody>')
        for pc, instruction in enumerate(self._machine.program):
            opcode, a, b = instruction
            current_css = ' class="m8-current"' if pc == self.pc else ""
            marker = " ← PC" if pc == self.pc else ""
            body += (f"<tr><td{current_css}>{pc}{marker}</td><td>{pc * 3}</td>"
                     f"<td><code>{opcode:02X}</code></td><td><code>{a:02X}</code></td><td><code>{b:02X}</code></td>"
                     f"<td><code>{escape(format_instruction(instruction))}</code></td></tr>")
        self._display(body + "</tbody></table></div>", "Program and machine code")

    def memory(self, start: int = 0, stop: int = 16) -> None:
        """Show data addresses in the half-open interval [start, stop)."""
        start = _integer(start, "First address", 0, 255)
        stop = _integer(stop, "Address upper bound", 1, 256)
        if start >= stop:
            raise Mini8Error("The first address must be less than the upper bound.")
        body = f'<p class="m8-muted">Data memory, addresses {start} through {stop - 1}. Each cell shows its value in decimal and as 8 bits.</p>'
        self._display(body + self._memory_html(self._machine.state(), range(start, stop)), "Data memory")
