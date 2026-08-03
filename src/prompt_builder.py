"""Short prompt templates used by the language model."""

from __future__ import annotations

from pydantic import BaseModel, ConfigDict

from .models import FunctionDefinition, PromptItem


class PromptBuilder(BaseModel):
    """Build selection and argument-extraction prompts."""

    model_config = ConfigDict(extra="forbid", strict=True)

    functions: list[FunctionDefinition]

    def build_selection_prompt(self, prompt: PromptItem) -> str:
        """Ask the model to choose one exact available function name."""
        definitions = "\n".join(
            f"- {function.prototype()}: {function.description}"
            for function in self.functions
        )
        return (
            "Choose the best function for the user request.\n"
            "Reply on the next line with only its exact name.\n\n"
            f"Functions:\n{definitions}\n\n"
            f"User request: {prompt.prompt!r}\n"
            "Function name:"
        )

    def build_parameter_prompt(
        self,
        prompt: PromptItem,
        function: FunctionDefinition,
    ) -> str:
        """Ask the model to extract, not execute, typed arguments."""
        examples = (
            self._regex_examples()
            if "regex" in function.parameters
            else ""
        )
        return (
            "Extract the raw arguments required by this function.\n"
            "Do not execute or calculate the function result.\n"
            "Preserve user-provided strings and spelling.\n\n"
            f"Function: {function.prototype()}\n"
            f"Description: {function.description}\n\n"
            f"{examples}"
            f"User request: {prompt.prompt!r}\n"
            "Parameters JSON:\n"
        )

    @staticmethod
    def _regex_examples() -> str:
        """Provide generic, non-test-specific regex extraction examples."""
        return (
            "Example request: 'Replace all digits in A1 B22 with X'\n"
            "Example parameters: "
            "{\"source_string\": \"A1 B22\", "
            "\"regex\": \"\\\\d+\", \"replacement\": \"X\"}\n"
            "Example request: 'Replace red with blue in red car'\n"
            "Example parameters: "
            "{\"source_string\": \"red car\", "
            "\"regex\": \"red\", \"replacement\": \"blue\"}\n\n"
        )
