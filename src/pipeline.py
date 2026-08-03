"""High-level orchestration of the function-calling pipeline."""

from __future__ import annotations

from pydantic import BaseModel, ConfigDict

from .args_generator import ArgumentGenerator
from .errors import DecodingError
from .function_selector import FunctionSelector
from .models import (
    FunctionCall,
    InputData,
    validate_parameters,
)


class FunctionCallingPipeline(BaseModel):
    """Select functions, generate arguments, and validate every result."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    selector: FunctionSelector
    generator: ArgumentGenerator

    def run(self, data: InputData) -> list[FunctionCall]:
        """Process every prompt in input order."""
        calls: list[FunctionCall] = []
        total = len(data.prompts)

        for index, prompt in enumerate(data.prompts, start=1):
            print(f"[{index}/{total}] {prompt.prompt}")
            function = self.selector.select(prompt, data.functions)
            parameters = self.generator.generate(prompt, function)
            try:
                validate_parameters(function, parameters)
            except ValueError as exc:
                raise DecodingError(
                    f"invalid parameters for {function.name}: {exc}"
                ) from exc

            calls.append(
                FunctionCall(
                    prompt=prompt.prompt,
                    name=function.name,
                    parameters=parameters,
                )
            )
            print(f"  -> {function.name}")

        return calls
