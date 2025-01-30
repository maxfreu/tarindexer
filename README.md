# tarindexer

This is a little program to index tar files for fast access - inspired by https://github.com/devsnd/tarindexer and written by OpenAI o1. It has no dependencies and should compile without hassle.

This is a simple C program that reads a tar file from a specified file or standard input and outputs a list of files contained within the tar archive. For each file, it prints the file name, the byte offset where the file's data starts in the archive, and the size of the file in bytes.

The program supports standard tar formats, including long file names that use the POSIX ustar format with prefix fields and GNU tar's long name extension.

## Usage

```bash
./tarindexer [tar_file]
```

- `tar_file`: (Optional) Path to the tar file to be indexed. If not provided, the program reads from standard input.

### Help

To display usage instructions or help information:

```bash
./tarindexer --help
```

## Compilation

To compile the program, use the following command:

```bash
gcc -o tarindexer tarindexer.c
```

This will produce an executable named `tarindexer`.

## Examples

### Index a Tar File from a File

```bash
./tarindexer archive.tar
```

### Index a Tar File from Standard Input

```bash
cat archive.tar | ./tarindexer
```

## Output Format

The program outputs lines containing:

- **File Name**: The full path of the file within the tar archive.
- **Data Offset**: The byte offset in the tar file where the file's data starts.
- **Size**: The size of the file in bytes.

Example output:

```
folder/file1.txt 512 1024
long/path/to/a/file/with_long_name.txt 2048 2048
```

## Features

- **Supports Long File Names**: Handles file names longer than 100 characters using both the POSIX ustar `prefix` field and GNU tar's long name extension.
- **Error Checking**: Includes comprehensive error checking to prevent segmentation faults and other runtime errors.
- **Portability**: Uses standard C library functions for compatibility across different systems.

## Limitations

- **Tar Extensions**: Does not handle PAX extended headers or global headers.
- **Buffer Sizes**: Maximum supported file name length is limited by buffer sizes defined in the code:
  - GNU long names: up to 1024 bytes.
  - Combined `prefix` and `name`: up to 512 bytes.
- **Tar Variants**: Other tar variants or compression formats (e.g., gzip-compressed tar files) are not directly supported.

## Notes

- **Safety**: All buffers are properly managed, and string operations prevent buffer overflows.
- **Standard Compliance**: The program adheres to the standard tar format specifications and common extensions for maximum compatibility.
- **No External Dependencies**: The program does not rely on any external libraries beyond the standard C library.

## License

This program is provided under the MIT License.
