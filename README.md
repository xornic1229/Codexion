*This project has been created as part of the 42 curriculum by jaialons.*

# Call Me Maybe

## Description

Call Me Maybe translates natural-language requests into structured function
calls.

Given a request such as:

```text
What is the sum of 2 and 3?
```

the program does not calculate the result. It produces:

```json
{
  "prompt": "What is the sum of 2 and 3?",
  "name": "fn_add_numbers",
  "parameters": {
    "a": 2.0,
    "b": 3.0
  }
}
```

The project uses the subject-provided `Small_LLM_Model` with the required
`Qwen/Qwen3-0.6B` model.

Function selection and parameter extraction use constrained decoding. At each
generation step, invalid token logits are replaced with negative infinity, so
the model can only choose continuations compatible with the available function
names or the expected parameter type.

## Requirements

- Python 3.10 or later
- `uv`
- The included `llm_sdk`
- Internet access on the first execution, unless the Qwen model is cached

The application code uses Python's standard library, NumPy and Pydantic.
PyTorch, Transformers and Hugging Face Hub are internal dependencies of the
subject-provided SDK and are not imported directly by `src/`.

## Instructions

Install all dependencies from the repository root:

```bash
make install
```

This runs:

```bash
uv sync
```

Run the program with the default paths:

```bash
make run
```

Equivalent command:

```bash
uv run python -m src
```

Run with custom files:

```bash
uv run python -m src \
  --functions_definition data/input/functions_definition.json \
  --input data/input/function_calling_tests.json \
  --output data/output/function_calling_results.json
```

Other available commands:

```bash
make debug
make clean
make lint
make lint-strict
```

The `data/output/` directory is generated during execution and must not be
submitted.

## Input and output

The prompts file contains a JSON array:

```json
[
  {
    "prompt": "Greet shrek"
  },
  {
    "prompt": "Reverse the string 'hello'"
  }
]
```

The function-definition file describes every available function, its
description, parameter names and types, and return type.

Both files are validated with Pydantic before loading the model. Missing files,
malformed JSON, duplicated functions and invalid schemas produce readable error
messages.

The generated output contains exactly:

- `prompt`: the original user request
- `name`: the selected function name
- `parameters`: the extracted typed arguments

## Project structure

```text
src/
├── __main__.py
├── args_generator.py
├── constrained.py
├── errors.py
├── function_selector.py
├── llm.py
├── loader.py
├── models.py
├── pipeline.py
├── prompt_builder.py
├── vocabulary.py
└── writer.py

llm_sdk/
├── pyproject.toml
└── llm_sdk/
    └── __init__.py
```

## Algorithm explanation

### Function selection

The program builds a prompt containing all available function prototypes and
descriptions.

Each valid function name is tokenized. During generation:

1. The LLM returns logits for the next token.
2. The selector finds which tokens can continue at least one valid function
   name.
3. Every other logit is replaced with `-inf`.
4. The highest-scoring valid token is selected.
5. Candidates that no longer match are removed.
6. The process continues until one complete function name remains.

The LLM therefore makes the semantic decision, while constrained decoding
prevents invented function names.

Function selection does not use keyword-based rules or exact prompt matching.

### Parameter extraction

Once the function is selected, the expected parameter names and types are
known.

The program writes the JSON structure itself, including braces, keys, colons
and commas. The LLM only generates parameter values.

The constraints depend on the declared type:

- `number`: only valid JSON-number prefixes are allowed.
- `integer`: decimal points and exponents are rejected.
- `boolean`: the model chooses between `true` and `false`.
- `string`: unsafe control and special tokens are rejected, and generation
  finishes at the closing quote.
- `null`: the program writes the only valid value, `null`.

Numbers and strings have token limits to prevent infinite generation loops.

For functions with `source_string`, `regex` and `replacement`, a small
post-processing step canonicalises unambiguous regex concepts:

- numbers or digits become `\d+`
- vowels become `[aeiouAEIOU]`
- whitespace becomes `\s+`
- named symbols such as `asterisk`, `hash` or `underscore` become their literal
  characters

This normalisation does not select the function and does not match complete
provided test prompts. It only converts general linguistic concepts into their
canonical technical representation.

Finally, generated parameters are validated against the selected function
schema and serialized with `json.dump`.

## Design decisions

### One model

The project loads `Qwen/Qwen3-0.6B` once and uses it for both function selection
and parameter extraction. This reduces memory usage and guarantees compatibility
with the required model.

### Deterministic JSON structure

The model does not generate braces, keys or separators freely. The program
writes these structural elements and restricts the LLM to the values requiring
semantic interpretation.

This approach is smaller and easier to understand than a completely generic
JSON parser while still guaranteeing the required output structure.

### Public SDK interface

The application interacts with the model only through public SDK methods:

- `encode`
- `decode`
- `get_logits_from_input_ids`
- `get_path_to_vocab_file`

No private SDK methods or attributes are used.

### No supplied-prompt hardcoding

The implementation does not compare complete prompts with the supplied test
sentences and does not return predefined function calls for them.

## Performance analysis

The model is loaded only once. Function names require few decoding steps, and
numeric values are normally produced in a small number of tokens.

String generation is limited to 60 tokens and numeric generation to 20 tokens,
preventing unbounded loops.

The main cost is sequential inference because the SDK recomputes logits after
each generated token. Short prompts, cached token decoding and the absence of
terminal animations reduce unnecessary work.

In local testing, the program successfully processed the 11 supplied prompts
with correct function names, parameters and valid JSON. The first execution may
take longer because the model must be downloaded. Runtime after caching depends
on the evaluator's hardware.

## Error handling

The program handles:

- missing or unreadable files
- malformed JSON
- invalid input schemas
- duplicated function names
- SDK import or model-loading failures
- vocabulary-loading failures
- empty valid-token sets
- incomplete number or string generation
- incompatible generated parameter types
- output filesystem errors
- keyboard interruption

## Testing strategy

Before submission:

```bash
make install
make lint
rm -rf data/output
make run
```

Validate the generated JSON:

```bash
python3 -m json.tool \
  data/output/function_calling_results.json
```

Testing includes:

- positive, negative and decimal numbers
- strings with spaces, punctuation and accented characters
- regex categories and literal replacements
- malformed or missing input files
- functions with no parameters
- boolean, integer and null parameters
- function names where one name is a prefix of another
- ambiguous requests
- custom input and output paths

## Example usage

Input:

```json
[
  {
    "prompt": "Replace all vowels in 'Programming is fun' with asterisks"
  }
]
```

Command:

```bash
uv run python -m src
```

Output:

```json
[
  {
    "prompt": "Replace all vowels in 'Programming is fun' with asterisks",
    "name": "fn_substitute_string_with_regex",
    "parameters": {
      "source_string": "Programming is fun",
      "regex": "[aeiouAEIOU]",
      "replacement": "*"
    }
  }
]
```

The substitution function is not executed. The program only identifies the
function and extracts its parameters.

## Challenges faced

### Token boundaries

BPE tokenization can change depending on the text surrounding a token. Function
names are encoded from a controlled boundary so their token sequences remain
consistent during constrained selection.

### JSON numbers

Restricting generation to digits is not sufficient because strings such as
`--2`, `01` or `2..3` are invalid JSON numbers. Prefix validation rejects a
token before it can make the value impossible to complete.

### String loops

Small models can repeat token sequences. String generation uses a maximum token
limit and repeated-suffix detection without applying an aggressive repetition
penalty that could damage legitimate repeated text.

### SDK packaging

The provided SDK depends internally on PyTorch and Hugging Face libraries. It is
included as a local uv dependency so the evaluator only needs to execute
`uv sync`.

## Resources

- Project subject and the public `Small_LLM_Model` interface
- Python documentation for `json`, `argparse` and exception handling
- Pydantic documentation
- NumPy documentation
- Astral uv documentation
- Qwen3 model documentation
- Literature about constrained decoding, tokenization and function calling

## Use of AI

AI assistance was used for:

- reviewing the project architecture
- explaining tokenization, logits and constrained decoding
- identifying edge cases in numeric and string generation
- reviewing local SDK packaging and dependency management
- debugging test failures
- reviewing documentation and testing scenarios

All suggestions were manually reviewed and tested. The final implementation was
validated with `flake8`, `mypy` and execution against the provided inputs.