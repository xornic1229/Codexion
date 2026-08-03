"""Command-line entry point for Call Me Maybe."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pydantic import ValidationError

from .args_generator import ArgumentGenerator
from .errors import CallMeMaybeError
from .function_selector import FunctionSelector
from .llm import LLMClient
from .loader import Loader
from .pipeline import FunctionCallingPipeline
from .vocabulary import Vocabulary
from .writer import OutputWriter

DEFAULT_FUNCTIONS = "data/input/functions_definition.json"
DEFAULT_INPUT = "data/input/function_calling_tests.json"
DEFAULT_OUTPUT = "data/output/function_calling_results.json"


def parse_arguments() -> argparse.Namespace:
    """Parse the three paths required by the subject."""
    parser = argparse.ArgumentParser(
        description="Function calling with constrained LLM decoding"
    )
    parser.add_argument(
        "--functions_definition",
        default=DEFAULT_FUNCTIONS,
        help="path to functions_definition.json",
    )
    parser.add_argument(
        "--input",
        default=DEFAULT_INPUT,
        help="path to function_calling_tests.json",
    )
    parser.add_argument(
        "--output",
        default=DEFAULT_OUTPUT,
        help="path to function_calling_results.json",
    )
    return parser.parse_args()


def main() -> int:
    """Run loading, model inference, constrained decoding, and writing."""
    arguments = parse_arguments()

    try:
        data = Loader(
            functions_path=Path(arguments.functions_definition),
            prompts_path=Path(arguments.input),
        ).load()

        client = LLMClient()
        vocabulary = Vocabulary.from_client(client)
        pipeline = FunctionCallingPipeline(
            selector=FunctionSelector(client=client),
            generator=ArgumentGenerator(
                client=client,
                vocabulary=vocabulary,
            ),
        )
        calls = pipeline.run(data)
        OutputWriter(output_path=Path(arguments.output)).write(calls)
    except CallMeMaybeError as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1
    except ValidationError as exc:
        print(f"Error: internal validation failed: {exc}", file=sys.stderr)
        return 1
    except KeyboardInterrupt:
        print("Interrupted by user.", file=sys.stderr)
        return 130
    except Exception as exc:
        print(f"Unexpected error: {exc}", file=sys.stderr)
        return 1

    print(f"Done: wrote {len(calls)} result(s) to {arguments.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
