# Q-Shell

A lightweight and efficient shell implementation in C with support for command chaining, job control, and system call profiling.

## Features

- Command execution with pipes (`|`), AND (`&&`), and OR (`||`) operators
- Input/output redirection (`>`, `<`, `>>`, `2>`)
- Job control (background jobs with `&`)
- Built-in commands:
  - `cd`: Change directory
  - `help`: Show available commands
  - `exit`: Exit the shell
  - `profile`: Control system call profiling
  - `history`: View command history

## Building

```bash
make
```

## Dependencies

- GCC
- GNU Make
- readline library

On Ubuntu/Debian:
```bash
sudo apt install build-essential libreadline-dev
```

On Arch Linux:
```bash
sudo pacman -S base-devel readline
```

## License

MIT License 