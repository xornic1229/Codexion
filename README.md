*This project has been created as part of the 42 curriculum by jaialons.*

# Call Me Maybe

## Description

Call Me Maybe translates natural-language requests into structured function
calls. It does **not** execute the selected function. For every input prompt, it
writes an object containing:

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

The project uses the subject-provided `Small_LLM_Model` and constrained
decoding. The LLM performs the semantic work: choosing the appropriate
function and extracting argument values. The program restricts the LLM's token
choices so that names and values remain compatible with the input schema.

## Requirements

- Python 3.10 or later
- `uv`
- Internet access on the first run, unless the Qwen model is already cached
- The included subject-provided `llm_sdk`

The application code imports only the allowed project libraries, `numpy` and
`pydantic`, plus Python's standard library. `torch`, `transformers`, and
`huggingface-hub` are implementation dependencies of the provided SDK and are
not used directly by `src/`.

## Installation

Run from the repository root:

```bash
make install
```

The `install` target runs only:

```bash
uv sync
```

The root `pyproject.toml` declares `llm_sdk` as a local path dependency. Its own
`pyproject.toml` declares the packages required internally by the SDK. This is
why the package layout intentionally contains two levels:

```text
llm_sdk/                 local Python project
├── pyproject.toml
└── llm_sdk/             importable Python package
    └── __init__.py
```

No absolute paths, user-specific folders, or manual `pip --target` commands are
required.

## Instructions

Default execution:

```bash
make run
```

Equivalent command:

```bash
uv run python -m src
```

Custom paths:

```bash
uv run python -m src \
  --functions_definition data/input/functions_definition.json \
  --input data/input/function_calling_tests.json \
  --output data/output/function_calling_results.json
```

Other Makefile rules:

```bash
make debug
make clean
make lint
make lint-strict
```

The generated directory `data/output/` is ignored by Git and must not be
submitted.

## Input files

`data/input/function_calling_tests.json` contains a JSON array of prompt
objects:

```json
[
  {"prompt": "Greet shrek"},
  {"prompt": "Reverse the string 'hello'"}
]
```

`data/input/functions_definition.json` contains the available functions, their
descriptions, parameter names and types, and return type.

Input data is validated with Pydantic before the model is loaded. The loader
handles missing files, permission errors, malformed JSON, invalid schemas, and
duplicated function names with readable messages.

## Project structure

```text
src/
├── __main__.py          command-line entry point
├── models.py            Pydantic input, schema, and output models
├── loader.py            robust input JSON loading
├── llm.py               public compatibility wrapper over llm_sdk
├── vocabulary.py        token-to-ID and ID-to-token maps
├── prompt_builder.py    concise model prompts
├── constrained.py       masks, candidate decoding, number rules
├── function_selector.py constrained function-name choice
├── args_generator.py    constrained argument generation by type
├── pipeline.py          high-level orchestration and validation
├── writer.py            exact JSON output serialization
└── errors.py            expected application errors
```

## Algorithm explanation

The pipeline contains two separate constrained-decoding stages.

### 1. Function selection

The selector first creates a prompt containing every available function
prototype and description. Each valid function name is then tokenized in the
context of that prompt.

Generation proceeds token by token:

1. Ask the model for logits for the next token.
2. Determine the next token IDs belonging to at least one surviving function
   name.
3. Set every other logit to negative infinity.
4. Select the highest-scoring valid token with `argmax`.
5. Remove candidates whose token sequence no longer matches the generated
   prefix.
6. Continue until one complete name plus a newline terminator is generated.

The newline terminator also handles the edge case where one function name is a
prefix of another. The function is chosen by the LLM's logits, not by keywords,
regular expressions, or manually coded prompt rules. The constraint only makes
hallucinated names impossible.

### 2. Parameter extraction

After function selection, the expected parameter names and types are known.
The program writes the JSON structure itself: opening brace, keys, colons,
commas, and closing brace. The LLM generates only the values.

The decoder applies different constraints for each supported type:

- `number`: only prefixes that can still become a valid JSON number, including
  decimals and scientific notation.
- `integer`: only prefixes that can still become a valid JSON integer.
- `boolean`: finite candidate decoding over `true` and `false`.
- `string`: an opening quote is forced; control and special tokens are rejected;
  generation ends at the first token containing a closing quote.
- `null`: the literal `null` is written directly because it has only one valid
  value.

The program then converts values to their Python types and validates that the
parameter keys and types exactly match the selected function definition.
Finally, `json.dump` serializes Pydantic output models, guaranteeing a parseable
JSON file with exactly `prompt`, `name`, and `parameters`.

## Why this is constrained decoding

At every model-controlled generation step, the model produces logits for its
entire vocabulary. `LogitMasker.select` creates an array filled with `-inf`,
copies the original logits only for valid token IDs, and applies `argmax`.
Therefore, the model retains semantic choice among valid continuations but
cannot generate an invalid function name or an invalid typed prefix.

## Design decisions

### One Qwen3 model

The project uses the required default model, `Qwen/Qwen3-0.6B`, for both
function selection and parameter extraction. Loading one model keeps memory use
lower and demonstrates that the required model works throughout the pipeline.

### Program-generated JSON structure

A fully generic JSON state machine would add substantial code without improving
the required schema. This implementation writes deterministic structural tokens
and constrains the model only where semantic generation is needed: names and
values. This is easier to understand, faster, and still guarantees the required
output format.

### Explicit candidate boundary

A BPE tokenizer can merge text differently at a text boundary. The selector
therefore encodes the prompt, appends an explicit newline token sequence, and
then constrains generation to candidate names encoded from that known boundary.
This removes dependence on accidental prompt/name token merges.

### Lazy token decoding

The vocabulary file supplies all token IDs, but decoding every vocabulary item
at startup would be slow. Actual token text is decoded only when needed and then
cached.

### No provided-test hardcoding

The source never checks for exact test phrases or assigns an answer based on
keywords. The short regex examples are generic demonstrations of the function's
schema and use different source strings from the supplied tests.

## Performance analysis

Function-name decoding usually requires only a few model calls because names
are short. Numeric parameters generally require one or a small number of tokens.
String generation is bounded at 60 tokens, and numbers are bounded at 20 tokens,
so a model loop cannot run forever.

The main cost is sequential inference: the SDK is called once for each generated
token and recomputes the context. This is intentionally simple and compatible
with the public SDK. The implementation uses short prompts, a single loaded
model, cached token decoding, and no terminal animation to stay within the
subject's five-minute target on standard evaluation hardware.

Structural reliability is deterministic: output JSON is produced by
`json.dump`, output objects are Pydantic models with forbidden extra fields, and
generated parameters are checked against the selected schema. Semantic accuracy
still depends on the small LLM and should be measured on the evaluator's prompt
set rather than claimed independently of hardware and model cache.

## Error handling

Expected failures are converted into concise messages instead of uncontrolled
tracebacks:

- missing, unreadable, or malformed input files
- invalid Pydantic input schemas
- duplicated function names
- SDK import or model-loading failures
- vocabulary-loading failures
- an empty valid-token set
- incomplete number or string generation
- generated values that do not match the selected schema
- output permission and filesystem errors
- keyboard interruption

## Testing strategy

Before submission, run:

```bash
make install
make lint
make run
python -c "import json; json.load(open('data/output/function_calling_results.json'))"
```

Then verify:

1. Every output object has exactly `prompt`, `name`, and `parameters`.
2. Every `name` exists in the function-definition input.
3. Parameter keys exactly match the selected function.
4. Numbers are numeric, integers are integers, strings are strings, booleans are
   booleans, and null values are `null`.
5. `data/output/` is not tracked by Git.

Recommended edge cases:

- an empty prompt
- negative, decimal, and large numbers
- accented and special characters in strings
- one function name that prefixes another function name
- a function with no parameters
- malformed JSON
- a missing file
- a duplicated function name
- an ambiguous request

## Challenges

### BPE boundary alignment

Tokenization can change when two text fragments are concatenated. The selector
avoids an ambiguous boundary by forcing a newline after the selection prompt and
encoding every candidate from that explicit boundary. A newline terminator is
also included so prefix-related function names remain distinguishable.

### Numeric validity

Allowing only digit-like tokens is insufficient because sequences such as `--`,
`01`, or `2..3` are still invalid. Prefix rules reject a token before it can make
the number impossible to complete, and termination is permitted only for a
complete JSON number.

### Free-form string loops

Small models may repeat token patterns. String generation has a token limit and
an emergency repeated-suffix detector. It does not use an aggressive repetition
penalty because legitimate extracted strings can contain repeated characters or
words.

### SDK installation

The provided SDK imports Hugging Face and PyTorch internally. It is packaged as
a local uv dependency so the evaluator's single `uv sync` command installs the
SDK and its runtime dependencies without hardcoded paths or separate pip steps.
The PyTorch CPU index is pinned in the root uv configuration to avoid downloading
unnecessary CUDA packages on the standard x86-64 Linux evaluation environment.

## Resources

- The project subject and its `llm_sdk` public interface
- Python `json` and `argparse` documentation
- Pydantic documentation
- NumPy documentation
- Astral uv project and dependency documentation
- Qwen3 model documentation
- General literature on constrained decoding and function calling

## Use of AI

AI assistance was used for architecture review, explanations of tokenization and
constrained decoding, identifying edge cases, checking packaging decisions, and
drafting documentation. The final implementation was kept deliberately small,
separated by responsibility, and documented so each part can be explained and
modified during peer review.
