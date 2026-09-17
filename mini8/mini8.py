#!/usr/bin/env python3
"""MINI-8: a two-pass assembler and simulator for a fictional 8-bit CPU.

Python 3.10+, standard library only. Does not execute native machine code.
Usage:
    python mini8.py run examples/01_add.asm
    python mini8.py run examples/04_sum_registers.asm --trace
    python mini8.py asm examples/01_add.asm -o program.bin
    python mini8.py exec program.bin
    python mini8.py disasm program.bin
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys
from typing import Callable


class Mini8Error(ValueError):
    """An error in the source program, binary code, or program execution."""


# Each instruction: three bytes [opcode, A, B].
# Operand types: r = register, i = constant, m = [address], j = jump target.
SPEC: dict[str, tuple[int, str]] = {
    "HALT": (0x00, ""),
    "LDI": (0x01, "ri"),
    "LOAD": (0x02, "rm"),
    "STORE": (0x03, "mr"),
    "MOV": (0x04, "rr"),
    "ADD": (0x05, "rr"),
    "SUB": (0x06, "rr"),
    "CMP": (0x07, "rr"),
    "JMP": (0x08, "j"),
    "JE": (0x09, "j"),
    "JNE": (0x0A, "j"),
    "JL": (0x0B, "j"),
    "JGE": (0x0C, "j"),
    "OUT": (0x0D, "r"),
}
BY_OPCODE = {opcode: (name, kind) for name, (opcode, kind) in SPEC.items()}
IDENT = r"[A-Za-z_][A-Za-z0-9_]*"
LABEL = re.compile(rf"^({IDENT})\s*:\s*(.*)$")
EQU = re.compile(rf"^\.equ\s+({IDENT})\s*,\s*(\S+)\s*$", re.IGNORECASE)


def literal(token: str) -> int:
    """A non-negative integer in decimal, hexadecimal, or binary notation."""
    if re.fullmatch(r"[0-9]+", token):
        return int(token, 10)
    if re.fullmatch(r"0[xX][0-9a-fA-F]+", token):
        return int(token, 16)
    if re.fullmatch(r"0[bB][01]+", token):
        return int(token, 2)
    raise Mini8Error(f"Invalid number or unknown symbol: {token!r}")


def byte_value(value: int) -> int:
    if not 0 <= value <= 255:
        raise Mini8Error(f"Value {value} is outside the range 0..255.")
    return value


def register(token: str) -> int:
    if not re.fullmatch(r"R[0-3]", token, re.IGNORECASE):
        raise Mini8Error(f"Expected a register R0..R3, found {token!r}.")
    return int(token[1])


def assemble(source: str) -> bytes:
    """Assemble source in two passes; symbol names are case-insensitive.

    .equ defines numeric symbols; it neither allocates nor initializes memory.
    """
    symbols: dict[str, int] = {}
    pending: list[tuple[int, str]] = []

    def define(name: str, value: int) -> None:
        name = name.upper()
        if name in symbols:
            raise Mini8Error(f"Duplicate symbol {name!r}.")
        symbols[name] = value

    for line_number, raw_line in enumerate(source.splitlines(), 1):
        text = raw_line.split(";", 1)[0].strip()
        if not text:
            continue
        try:
            match = LABEL.fullmatch(text)
            if match:
                define(match.group(1), len(pending))
                text = match.group(2).strip()
                if not text:
                    continue
            if text.startswith("."):
                match = EQU.fullmatch(text)
                if not match:
                    raise Mini8Error("The directive must have the form .equ name, number.")
                define(match.group(1), byte_value(literal(match.group(2))))
                continue
            pending.append((line_number, text))
            if len(pending) > 256:
                raise Mini8Error("A program can contain at most 256 instructions.")
        except Mini8Error as exc:
            raise Mini8Error(f"Line {line_number}: {exc}") from exc

    if not pending:
        raise Mini8Error("The program contains no instructions.")

    def value(token: str) -> int:
        key = token.upper()
        return byte_value(symbols[key] if key in symbols else literal(token))

    def address(token: str) -> int:
        match = re.fullmatch(r"\[\s*([^\[\]]+?)\s*\]", token)
        if not match:
            raise Mini8Error(f"The address must be enclosed in square brackets: {token!r}.")
        return value(match.group(1).strip())

    code = bytearray()
    for line_number, text in pending:
        try:
            parts = text.split(None, 1)
            mnemonic = parts[0].upper()
            if mnemonic not in SPEC:
                raise Mini8Error(f"Unknown instruction {mnemonic!r}.")
            opcode, kind = SPEC[mnemonic]
            operands = [part.strip() for part in parts[1].split(",")] if len(parts) == 2 else []
            if len(operands) != len(kind) or any(not item for item in operands):
                raise Mini8Error(f"{mnemonic} requires {len(kind)} operands.")
            a = b = 0
            if kind == "ri":
                a, b = register(operands[0]), value(operands[1])
            elif kind == "rm":
                a, b = register(operands[0]), address(operands[1])
            elif kind == "mr":
                # Syntax: STORE [address], register; binary order: register, address.
                a, b = register(operands[1]), address(operands[0])
            elif kind == "rr":
                a, b = register(operands[0]), register(operands[1])
            elif kind == "j":
                a = value(operands[0])
                if a >= len(pending):
                    raise Mini8Error(f"Jump target {a} is outside the program.")
            elif kind == "r":
                a = register(operands[0])
            code.extend((opcode, a, b))
        except Mini8Error as exc:
            raise Mini8Error(f"Line {line_number}: {exc}") from exc
    return bytes(code)


def decode(code: bytes) -> list[tuple[int, int, int]]:
    """Read and validate a raw binary program, including unused operands."""
    if not code or len(code) % 3 or len(code) > 256 * 3:
        raise Mini8Error("Code must contain 1..256 complete three-byte instructions.")
    instructions = [tuple(code[i:i + 3]) for i in range(0, len(code), 3)]
    for pc, (opcode, a, b) in enumerate(instructions):
        if opcode not in BY_OPCODE:
            raise Mini8Error(f"Instruction {pc}: unknown opcode 0x{opcode:02X}.")
        mnemonic, kind = BY_OPCODE[opcode]
        if kind in {"ri", "rm", "mr", "rr", "r"} and a > 3:
            raise Mini8Error(f"Instruction {pc}: invalid register R{a}.")
        if kind == "rr" and b > 3:
            raise Mini8Error(f"Instruction {pc}: invalid register R{b}.")
        if kind in {"j", "r"} and b != 0:
            raise Mini8Error(f"Instruction {pc}: unused operand B must be 0.")
        if kind == "" and (a != 0 or b != 0):
            raise Mini8Error(f"Instruction {pc}: both HALT operands must be 0.")
        if kind == "j" and a >= len(instructions):
            raise Mini8Error(f"Instruction {pc}: jump target {a} is outside the program.")
    return instructions


def format_instruction(instruction: tuple[int, int, int]) -> str:
    opcode, a, b = instruction
    name, kind = BY_OPCODE[opcode]
    if kind == "ri":
        return f"{name} R{a}, {b}"
    if kind == "rm":
        return f"{name} R{a}, [{b}]"
    if kind == "mr":
        return f"{name} [{b}], R{a}"
    if kind == "rr":
        return f"{name} R{a}, R{b}"
    if kind == "j":
        return f"{name} {a}"
    if kind == "r":
        return f"{name} R{a}"
    return name


def disassemble(code: bytes) -> str:
    """The output can be reassembled into identical binary code."""
    lines = []
    for pc, instruction in enumerate(decode(code)):
        hex_bytes = " ".join(f"{item:02X}" for item in instruction)
        lines.append(f"{format_instruction(instruction):<22} ; PC={pc:03d}  {hex_bytes}")
    return "\n".join(lines) + "\n"


@dataclass(frozen=True)
class Result:
    registers: tuple[int, ...]
    memory: tuple[int, ...]
    output: tuple[int, ...]
    steps: int
    pc: int


@dataclass(frozen=True)
class Snapshot:
    """An immutable copy of the processor state at one moment in time."""

    registers: tuple[int, ...]
    memory: tuple[int, ...]
    output: tuple[int, ...]
    pc: int
    steps: int
    comparison: int
    halted: bool


@dataclass(frozen=True)
class Step:
    """One executed instruction together with its before and after states."""

    before: Snapshot
    after: Snapshot
    instruction: tuple[int, int, int]


class Machine:
    """Stateful 8-bit CPU; PC indexes instructions.

    Only CMP updates the comparison flag (-1: less, 0: equal, 1: greater).
    Comparisons are unsigned; ADD and SUB wrap modulo 256.
    """

    def __init__(self, code: bytes) -> None:
        self.program = decode(code)
        self.registers = [0] * 4
        self.memory = [0] * 256
        self.output: list[int] = []
        self.pc = 0
        self.steps = 0
        self.comparison = 0
        self.halted = False

    def state(self) -> Snapshot:
        """Capture state without changing the processor or sharing its lists."""
        return Snapshot(
            tuple(self.registers), tuple(self.memory), tuple(self.output),
            self.pc, self.steps, self.comparison, self.halted,
        )

    def step(self) -> Step:
        """Execute exactly one instruction; HALT counts as a step and keeps PC."""
        if self.halted:
            raise Mini8Error("The processor has already stopped at HALT.")
        if not 0 <= self.pc < len(self.program):
            raise Mini8Error(f"PC={self.pc} is outside the program. Is HALT or a jump missing?")
        before = self.state()
        instruction = self.program[self.pc]
        opcode, a, b = instruction
        next_pc = self.pc + 1
        if opcode == 0x00:  # HALT
            self.halted = True
            next_pc = self.pc
        elif opcode == 0x01:  # LDI
            self.registers[a] = b
        elif opcode == 0x02:  # LOAD
            self.registers[a] = self.memory[b]
        elif opcode == 0x03:  # STORE
            self.memory[b] = self.registers[a]
        elif opcode == 0x04:  # MOV
            self.registers[a] = self.registers[b]
        elif opcode == 0x05:  # ADD
            self.registers[a] = (self.registers[a] + self.registers[b]) & 0xFF
        elif opcode == 0x06:  # SUB
            self.registers[a] = (self.registers[a] - self.registers[b]) & 0xFF
        elif opcode == 0x07:  # CMP
            self.comparison = (self.registers[a] > self.registers[b]) - (self.registers[a] < self.registers[b])
        elif opcode == 0x08:  # JMP
            next_pc = a
        elif opcode == 0x09 and self.comparison == 0:  # JE
            next_pc = a
        elif opcode == 0x0A and self.comparison != 0:  # JNE
            next_pc = a
        elif opcode == 0x0B and self.comparison < 0:  # JL
            next_pc = a
        elif opcode == 0x0C and self.comparison >= 0:  # JGE
            next_pc = a
        elif opcode == 0x0D:  # OUT
            self.output.append(self.registers[a])
        self.pc = next_pc
        self.steps += 1
        return Step(before, self.state(), instruction)


def run(
    code: bytes,
    *,
    max_steps: int = 100_000,
    trace: bool = False,
    on_output: Callable[[int], None] | None = None,
) -> Result:
    """Run a fresh machine, subject to an instruction limit."""
    if max_steps <= 0:
        raise Mini8Error("The step limit must be positive.")
    machine = Machine(code)

    for _ in range(max_steps):
        step = machine.step()
        after = step.after
        if step.instruction[0] == 0x0D and on_output is not None:
            on_output(after.output[-1])

        if trace:
            comparison_name = {-1: "LT", 0: "EQ", 1: "GT"}[after.comparison]
            values = " ".join(f"R{i}={v:3d}" for i, v in enumerate(after.registers))
            print(
                f"{after.steps:06d} PC={step.before.pc:03d} {format_instruction(step.instruction):<22}"
                f" | {values} CMP={comparison_name} NEXT={after.pc:03d}",
                file=sys.stderr,
            )
        if after.halted:
            return Result(after.registers, after.memory, after.output, after.steps, after.pc)
    raise Mini8Error(f"Exceeded instruction limit {max_steps}; the program may not terminate.")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    for command, help_text in (("run", "Assemble and run .asm"), ("exec", "Run .bin")):
        item = sub.add_parser(command, help=help_text)
        item.add_argument("input", type=Path)
        item.add_argument("--trace", action="store_true", help="Show executed instructions and register values")
        item.add_argument("--max-steps", type=int, default=100_000)
    item = sub.add_parser("asm", help="Assemble .asm into .bin")
    item.add_argument("input", type=Path)
    item.add_argument("-o", "--output", type=Path)
    item = sub.add_parser("disasm", help="Convert .bin back to numeric assembly")
    item.add_argument("input", type=Path)
    args = parser.parse_args(argv)
    try:
        if args.command == "asm":
            code = assemble(args.input.read_text(encoding="utf-8"))
            destination = args.output or args.input.with_suffix(".bin")
            if destination.resolve() == args.input.resolve():
                raise Mini8Error("The output file must not overwrite the source program.")
            destination.write_bytes(code)
            print(f"{len(code) // 3} instructions, {len(code)} bytes -> {destination}")
        elif args.command == "disasm":
            print(disassemble(args.input.read_bytes()), end="")
        else:
            code = assemble(args.input.read_text(encoding="utf-8")) if args.command == "run" else args.input.read_bytes()
            run(code, max_steps=args.max_steps, trace=args.trace, on_output=print)
    except (Mini8Error, OSError, UnicodeError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
