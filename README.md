# Perf Demo

This is an accompanying repository to the following [article](https://fiseni.com/posts/the-journey-to-630x-faster-batch-job/).
This repository contains the C implementation. The C# implementation can be found in the following [repository](https://github.com/fiseni/PerfDemo).

## Usage
The application expects two arguments: the path to the MasterParts file and the path to the Parts file. The files are included in the `data` directory. To simplify usage, there are `run` scripts that build and run the app.
```bash
# Linux. It builds the source using gcc and runs the app.
./run.sh 

# Windows. It builds the source using cl and runs the app.
./run.bat 
```

If you want to build/run a different implementation, provide the implementation suffix as follows
```bash
# It builds the source using `processor5.c` implementation.
./run.sh full 5
./run.bat full 5
```

For convenience during development, smaller datasets are included.
```bash
./run.sh short 5
./run.bat short 5
```

To run the tests.
```bash
./run.sh test 5
./run.bat test 5
```

## Challenge description

For a provided `SourceData` (shown below), we should find a match for each `Part` according to these rules:
- If `Part.partNumber` is null or less than 3 characters (trimmed) return null.
- Find and return `MasterPart.partNumber` where the current `Part.partNumber` is a suffix to `MasterPart.partNumber`.
- If not found, find and return `MasterPart.partNumber` where the current `Part.partNumber` is a suffix to `MasterPart.partNumberNoHyphens`.
- If not found, find and return `MasterPart.partNumber` where `MasterPart.partNumber` is a suffix to the current `Part.partNumber`.
- All comparisons should be case-insensitive.
- Find the best matches; an equal string, then a string with +1 length, and so on.

```C
typedef struct MasterPart {
    const char* partNumber;
    const char* partNumberNoHyphens;
    size_t partNumberLength;
    size_t partNumberNoHyphensLength;
} MasterPart;

typedef struct Part {
    const char* partNumber;
    size_t partNumberLength;
} Part;

typedef struct SourceData {
    const MasterPart* masterParts;
    size_t masterPartsCount;
    const Part* parts;
    size_t partsCount;
} SourceData;
```

### Constraints and assumptions

- Reading and preparing the source data is not in the scope of the benchmarks.
- All strings contain only ASCII characters.
- All strings are less than 50 characters.
- The `MasterPart.partNumber` in the source is already trimmed and uppercase.
- The `MasterPart.partNumberNoHyphens` is the same string where '-' is stripped out if present.
- The `Part.partNumber` is in raw form. It might contain leading/trailing spaces and an unknown case.
- The `SourceData` should be considered as read-only. If you need any processing, sorting the arrays, processing `Part.partNumber`, and so on; it should be done in a separate copy and is part of the benchmark.
- Once the matching completes, any allocated memory used during the process should be freed up.
- This challenge is focused only on finding matches. The real app does further processing for each part, so the main loop can not be removed. If there is a need to pre-process the data, it should be done before the main loop. To standardize the approach, we'll use the following main function for all implementations.
```C
static void run(const SourceData* data) {
    
    processor_initialize(data);

    size_t matchCount = 0;
    for (size_t i = 0; i < data->partsCount; i++) {
        if (processor_find_match(data->parts[i].partNumber)) {
            matchCount++;
        }
    };

    processor_clean();
}
```

The original implementation is [processor1](https://github.com/fiseni/PerfDemoC/blob/main/processor1.c), and that's the logic we're trying to optimize.
