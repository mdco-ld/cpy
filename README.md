# cpy
Simple clipboard utility

This is a simple wrapper around `xclip`.

## Compiling
```bash
make
```

## Usage
Copy an image to clipboard:
```bash
cpy image.png
```

Copy the output of a command to clipboard
```bash
<command> | cpy
```

Write clipboard contents to a file
```
cpy > output.txt
```

Write clipboard content to stdout:
```bash
cpy
```
