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
        regex_guidance = (
            self._regex_guidance()
            if "regex" in function.parameters
            else ""
        )

        return (
            "Extract the raw arguments required by this function.\n"
            "Do not execute or calculate the function result.\n"
            "Preserve user-provided strings and spelling exactly.\n"
            "JSON strings use double quotes. Apostrophes are ordinary "
            "characters: never add a backslash before an apostrophe.\n\n"
            f"Function: {function.prototype()}\n"
            f"Description: {function.description}\n\n"
            f"{regex_guidance}"
            f"User request: {prompt.prompt!r}\n"
            "Parameters JSON:\n"
        )

    @staticmethod
    def _regex_guidance() -> str:
        """Add generic semantic guidance for regex substitutions."""
        return (
            "Regex-substitution rules:\n"
            "- source_string: copy the exact text to transform.\n"
            "- regex: return one reusable regular-expression pattern; "
            "never list only the matches found in source_string.\n"
            "- replacement: return the literal replacement text or "
            "character. Named symbols mean their character, for example "
            "asterisks -> *, hashes -> #, underscores -> _, dashes -> -.\n"
            "- Standard general patterns: numbers or digits -> \\d+; "
            "vowels -> [aeiouAEIOU]; spaces or whitespace -> \\s+.\n\n"
            "Example request: \"Replace every number in 'Room 7, "
            "floor 12' with VALUE\"\n"
            "Example parameters: "
            "{\"source_string\": \"Room 7, floor 12\", "
            "\"regex\": \"\\\\d+\", "
            "\"replacement\": \"VALUE\"}\n"
            "Example request: \"Change all vowels in 'Education' "
            "to hashes\"\n"
            "Example parameters: "
            "{\"source_string\": \"Education\", "
            "\"regex\": \"[aeiouAEIOU]\", "
            "\"replacement\": \"#\"}\n"
            "Example request: \"Replace red with blue in 'red car'\"\n"
            "Example parameters: "
            "{\"source_string\": \"red car\", "
            "\"regex\": \"red\", "
            "\"replacement\": \"blue\"}\n\n"
        )
