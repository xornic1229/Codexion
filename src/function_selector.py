"""LLM-based constrained function-name selection."""

from __future__ import annotations

from pydantic import BaseModel, ConfigDict

from .constrained import CandidateDecoder
from .errors import DecodingError
from .llm import LLMClient
from .models import FunctionDefinition, PromptItem
from .prompt_builder import PromptBuilder


class FunctionSelector(BaseModel):
    """Use model logits to select one available function name."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    client: LLMClient

    def select(
        self,
        prompt: PromptItem,
        functions: list[FunctionDefinition],
    ) -> FunctionDefinition:
        """Return the definition selected by constrained decoding."""
        builder = PromptBuilder(functions=functions)
        base_prompt = builder.build_selection_prompt(prompt)
        names = [function.name for function in functions]
        chosen = CandidateDecoder(client=self.client).choose(
            base_prompt,
            names,
            forced_prefix="\n",
            terminator="\n",
        )

        for function in functions:
            if function.name == chosen:
                return function
        raise DecodingError("selected function is not in the input schema")
