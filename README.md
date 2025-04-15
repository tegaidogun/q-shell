# q-shell

A modern shell implementation with advanced features and extensibility.

## Features

- Command chaining with operators (|, &&, ||)
- I/O redirection
- Background job control
- Command history
- Built-in commands
- System call profiling

## Dependencies

### Required
- GCC (GNU Compiler Collection)
- CMake (>= 3.10)
- GNU Readline library
- POSIX-compliant system

### Optional
- Python 3 (for documentation scripts)
- Doxygen (for API documentation)

## Building

1. Clone the repository:
```bash
git clone https://github.com/tegaidogun/q-shell.git
cd q-shell
```

2. Build the project:
```bash
make
```

3. Install (optional):
```bash
sudo make install
```

## Documentation and Testing

### Documentation
To generate the documentation:
```bash
make docs
```
This will:
1. Run the Python documentation script to generate markdown files
2. Generate API documentation using Doxygen
3. Output the documentation to `docs/html/`

### Testing
To run all tests:
```bash
make test
```
This will run the following test suites:
- Parser tests
- Shell core tests
- Profiler tests
- Input handling tests
- Tokenizer tests

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details. 