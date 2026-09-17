# MINI-8

An educational assembler and simulator for a simple eight-bit processor, written
in Python. It has four registers and 256 bytes of data memory, and supports
arithmetic, comparisons, jumps, and output. The project includes two student
notebooks and assembly examples in `examples/`. Author: Karel Šafr, PhD.

## Setup

You need Python 3.10 or later. **Each student must create their own virtual
environment** in the project folder containing `mini8.py`.

To run the notebooks, install the packages listed in `requirements-notebooks.txt`:

- `ipython` – displays the processor state in notebooks;
- `ipykernel` – provides the Python kernel for notebooks;
- `jupyterlab` – provides an interface for opening and running notebooks.

## Main features

`mini8.py` assembles source code into bytes (`asm`), runs an assembly program
(`run`) or a binary file (`exec`), and converts bytes back to assembly (`disasm`).
The assembler and simulator themselves use only the Python standard library.

With Python from your virtual environment selected, you can run:

```sh
python mini8.py run examples/01_add.asm
python mini8.py run examples/04_sum_registers.asm --trace
python mini8.py asm examples/01_add.asm -o program.bin
python mini8.py exec program.bin
python mini8.py disasm program.bin
python -m unittest discover -v
```

If the environment is not activated, replace `python` in these commands with
`.venv/bin/python` (macOS / Linux) or `.venv\Scripts\python.exe` (Windows).

In a notebook, `CPU(source)` loads a program; `show()` displays its state,
`step()` executes an instruction, `run()` continues until `HALT`, and `reset()`
resets the processor. Use `history()`, `memory()`, and `listing()` to display
the execution history, data memory, and program instructions.
