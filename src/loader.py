"""Read and validate the two JSON input files."""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any

from pydantic import BaseModel, ConfigDict, ValidationError

from .errors import InputError
from .models import FunctionDefinition, InputData, PromptItem


class Loader(BaseModel):
    """Load prompts and function definitions from configurable paths."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    functions_path: Path
    prompts_path: Path

    def load(self) -> InputData:
        """Return the complete validated input used by the pipeline."""
        prompts = self.load_prompts()
        functions = self.load_functions()
        try:
            return InputData(prompts=prompts, functions=functions)
        except ValidationError as exc:
            raise InputError(f"invalid input data: {exc}") from exc

    def load_prompts(self) -> list[PromptItem]:
        """Load prompts from function_calling_tests.json."""
        data = self._read_json(self.prompts_path)
        if not isinstance(data, list):
            raise InputError("prompts file must contain a JSON array")

        prompts: list[PromptItem] = []
        for index, item in enumerate(data):
            try:
                if isinstance(item, str):
                    prompts.append(PromptItem(prompt=item))
                else:
                    prompts.append(PromptItem.model_validate(item))
            except ValidationError as exc:
                raise InputError(
                    f"invalid prompt at index {index}: {exc}"
                ) from exc
        return prompts

    def load_functions(self) -> list[FunctionDefinition]:
        """Load definitions from functions_definition.json."""
        data = self._read_json(self.functions_path)
        if not isinstance(data, list):
            raise InputError("functions file must contain a JSON array")

        functions: list[FunctionDefinition] = []
        for index, item in enumerate(data):
            try:
                functions.append(FunctionDefinition.model_validate(item))
            except ValidationError as exc:
                raise InputError(
                    f"invalid function at index {index}: {exc}"
                ) from exc
        return functions

    @staticmethod
    def _read_json(path: Path) -> Any:
        """Read one JSON file while converting OS errors to clear messages."""
        try:
            with path.open("r", encoding="utf-8") as file:
                return json.load(file)
        except FileNotFoundError as exc:
            raise InputError(f"file not found: {path}") from exc
        except PermissionError as exc:
            raise InputError(
                f"permission denied while reading: {path}"
            ) from exc
        except json.JSONDecodeError as exc:
            raise InputError(
                f"invalid JSON in {path}: line {exc.lineno}, "
                f"column {exc.colno}"
            ) from exc
        except OSError as exc:
            raise InputError(f"could not read {path}: {exc}") from exc
