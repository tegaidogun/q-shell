# User Guide

## Introduction

q-shell is a modern shell implementation with advanced features and extensibility. This guide covers the key features and usage patterns.

## Installation

### Building from Source

1. Clone the repository:
```bash
git clone https://github.com/tegaidogun/q-shell.git
cd q-shell
```

2. Create a build directory and build the project:
```bash
mkdir build
cd build
cmake ..
make
```

3. Install (optional):
```bash
sudo make install
```

## Features

### Built-in Commands

#### help
Display available built-in commands and their usage.

#### cd
Change the current directory:
```bash
cd /path/to/directory
cd ~  # Home directory
cd -  # Previous directory
```

#### profile
Manage shell profiling:
```bash
profile status  # Check profiling status
profile on      # Enable profiling
profile off     # Disable profiling
```

#### history
View command history with timestamps and exit codes:
```bash
history
```

#### exit
Exit the shell:
```bash
exit
```

### Command Chaining

Chain commands using operators:
```bash
command1 && command2  # Run command2 if command1 succeeds
command1 || command2  # Run command2 if command1 fails
command1 | command2   # Pipe output of command1 to command2
command1 &           # Run command1 in background
```

### Redirection

Redirect input and output:
```bash
command > file      # Redirect stdout to file
command >> file     # Append stdout to file
command < file      # Redirect file to stdin
command 2> file     # Redirect stderr to file
command 2>> file    # Append stderr to file
```

### Job Control

Manage background jobs:
```bash
command &          # Run command in background
jobs              # List background jobs
fg %1             # Bring job 1 to foreground
bg %1             # Continue job 1 in background
```

## Troubleshooting

### Common Issues

1. **Permission denied**
   - Check file permissions
   - Use sudo if appropriate

2. **Syntax error**
   - Check command syntax
   - Verify quotes and special characters

### Getting Help

- Use the `help` command
- Check the documentation
- Report issues on GitHub

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details. 

## Additional Resources

- [Development Guidelines](../development.md) - For developers
- [API Documentation](docs/index.md) - Detailed API reference
