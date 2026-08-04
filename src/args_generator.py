"""Schema-guided constrained generation of function parameters."""

from __future__ import annotations

import json
import re
from typing import Any, Callable

from pydantic import BaseModel, ConfigDict, PrivateAttr

from .constrained import CandidateDecoder, JsonNumberRules, LogitMasker
from .errors import DecodingError
from .llm import LLMClient
from .models import (
    FunctionDefinition,
    FunctionParameter,
    JsonValue,
    PromptItem,
)
from .prompt_builder import PromptBuilder
from .vocabulary import Vocabulary


class ArgumentGenerator(BaseModel):
    """Generate each required parameter under its declared type rules."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    client: LLMClient
    vocabulary: Vocabulary
    max_number_tokens: int = 20
    max_string_tokens: int = 60

    _numeric_ids: set[int] = PrivateAttr(default_factory=set)
    _separator_ids: set[int] = PrivateAttr(default_factory=set)
    _string_ids: set[int] = PrivateAttr(default_factory=set)

    def model_post_init(self, __context: Any) -> None:
        """Pre-compute candidate token sets from the vocabulary."""
        numeric_characters = set("0123456789.eE+-")

        for token_id, raw_token in self.vocabulary.id_to_token.items():
            if (
                raw_token
                and all(
                    character in numeric_characters
                    for character in raw_token
                )
            ):
                self._numeric_ids.add(token_id)

            if raw_token and raw_token[0] in {",", "}"}:
                self._separator_ids.add(token_id)

            if raw_token.startswith("Ċ") or raw_token.startswith("ĉ"):
                self._separator_ids.add(token_id)

            if raw_token and not self._is_special_raw_token(raw_token):
                self._string_ids.add(token_id)

    def generate(
        self,
        prompt: PromptItem,
        function: FunctionDefinition,
    ) -> dict[str, JsonValue]:
        """Generate all required parameter values."""
        if not function.parameters:
            return {}

        builder = PromptBuilder(functions=[function])
        context = builder.build_parameter_prompt(prompt, function) + "{\n"

        parameters: dict[str, JsonValue] = {}
        total_parameters = len(function.parameters)

        for index, (name, specification) in enumerate(
            function.parameters.items()
        ):
            key_text = f'  "{name}": '
            context += key_text

            value, raw_value = self._decode_value(
                context,
                specification,
            )

            if specification.type == "string" and isinstance(value, str):
                value = self._repair_grounded_string(
                    prompt.prompt,
                    value,
                )
                raw_value = json.dumps(
                    value,
                    ensure_ascii=False,
                )

            parameters[name] = value
            context += raw_value

            if index == total_parameters - 1:
                context += "\n}"
            else:
                context += ",\n"

        if self._has_regex_schema(function):
            parameters = self._normalise_regex_arguments(
                prompt.prompt,
                parameters,
            )

        return parameters

    def _decode_value(
        self,
        context: str,
        specification: FunctionParameter,
    ) -> tuple[JsonValue, str]:
        """Dispatch a parameter to its type-specific decoder."""
        if specification.type == "number":
            raw_value = self._decode_numeric(
                context,
                JsonNumberRules.is_number_prefix,
                JsonNumberRules.is_complete_number,
            )
            return float(raw_value), raw_value

        if specification.type == "integer":
            raw_value = self._decode_numeric(
                context,
                JsonNumberRules.is_integer_prefix,
                JsonNumberRules.is_complete_integer,
            )
            return int(raw_value), raw_value

        if specification.type == "boolean":
            raw_value = CandidateDecoder(client=self.client).choose(
                context,
                ["true", "false"],
            )
            return raw_value == "true", raw_value

        if specification.type == "string":
            value = self._decode_string(context)
            raw_value = json.dumps(
                value,
                ensure_ascii=False,
            )
            return value, raw_value

        if specification.type == "null":
            return None, "null"

        raise DecodingError(
            f"unsupported parameter type: {specification.type}"
        )

    def _decode_numeric(
        self,
        context: str,
        is_prefix: Callable[[str], bool],
        is_complete: Callable[[str], bool],
    ) -> str:
        """Generate a complete JSON number token by token."""
        input_ids = self.client.encode(context)
        generated_text = ""

        for _ in range(self.max_number_tokens):
            valid_ids = {
                token_id
                for token_id in self._numeric_ids
                if self._numeric_token_allowed(
                    token_id,
                    generated_text,
                    is_prefix,
                )
            }

            if is_complete(generated_text):
                valid_ids.update(self._separator_ids)

            logits = self.client.logits(input_ids)
            chosen_id = LogitMasker.select(logits, valid_ids)
            chosen_text = self.vocabulary.token_text(chosen_id)

            if (
                chosen_id in self._separator_ids
                and is_complete(generated_text)
            ):
                return generated_text

            candidate = generated_text + chosen_text

            if not is_prefix(candidate):
                raise DecodingError(
                    "model selected an invalid numeric token: "
                    f"{chosen_text!r}"
                )

            generated_text = candidate
            input_ids.append(chosen_id)

        if is_complete(generated_text):
            return generated_text

        raise DecodingError(
            f"numeric value did not finish: {generated_text!r}"
        )

    def _numeric_token_allowed(
        self,
        token_id: int,
        current_text: str,
        is_prefix: Callable[[str], bool],
    ) -> bool:
        """Check whether a token preserves a valid numeric prefix."""
        token_text = self.vocabulary.token_text(token_id)

        if not token_text:
            return False

        if any(character.isspace() for character in token_text):
            return False

        return is_prefix(current_text + token_text)

    @staticmethod
    def _has_regex_schema(function: FunctionDefinition) -> bool:
        """Return whether the function has standard regex arguments."""
        required = {
            "source_string",
            "regex",
            "replacement",
        }
        return required.issubset(function.parameters)

    @classmethod
    def _normalise_regex_arguments(
        cls,
        user_prompt: str,
        parameters: dict[str, JsonValue],
    ) -> dict[str, JsonValue]:
        """Canonicalise clear regex classes and named symbols."""
        regex_value = parameters.get("regex")
        replacement_value = parameters.get("replacement")

        if not isinstance(regex_value, str):
            return parameters

        if not isinstance(replacement_value, str):
            return parameters

        instruction = cls._text_outside_quotes(user_prompt).casefold()
        words = set(re.findall(r"[a-z]+", instruction))

        number_words = {
            "number",
            "numbers",
            "digit",
            "digits",
            "integer",
            "integers",
        }

        vowel_words = {
            "vowel",
            "vowels",
        }

        if words.intersection(number_words):
            parameters["regex"] = r"\d+"
        elif words.intersection(vowel_words):
            parameters["regex"] = "[aeiouAEIOU]"
        elif "white space" in instruction or "whitespace" in words:
            parameters["regex"] = r"\s+"

        replacement_aliases = {
            "asterisk": "*",
            "asterisks": "*",
            "hash": "#",
            "hashes": "#",
            "underscore": "_",
            "underscores": "_",
            "dash": "-",
            "dashes": "-",
            "hyphen": "-",
            "hyphens": "-",
        }

        replacement_key = replacement_value.strip().casefold()
        symbol = replacement_aliases.get(replacement_key)

        if symbol is not None:
            parameters["replacement"] = symbol

        return parameters

    @staticmethod
    def _text_outside_quotes(text: str) -> str:
        """Remove quoted values while preserving instruction text."""
        result: list[str] = []
        active_quote: str | None = None
        escaped = False

        for character in text:
            if active_quote is not None:
                if escaped:
                    escaped = False
                elif character == "\\":
                    escaped = True
                elif character == active_quote:
                    active_quote = None
                continue

            if character in {"'", '"'}:
                active_quote = character
                continue

            result.append(character)

        return "".join(result)

    @staticmethod
    def _repair_grounded_string(
        user_prompt: str,
        value: str,
    ) -> str:
        """Remove redundant escaping only when supported by the prompt."""
        if value in user_prompt:
            return value

        candidates = (
            value.replace("\\'", "'"),
            value.replace('\\"', '"'),
        )

        for candidate in candidates:
            if candidate in user_prompt:
                return candidate

        return value

    def _decode_string(self, context: str) -> str:
        """Generate string content until a valid closing quote."""
        input_ids = self.client.encode(context + '"')
        chunks: list[str] = []
        generated_ids: list[int] = []

        for _ in range(self.max_string_tokens):
            logits = self.client.logits(input_ids)
            valid_ids = set(self._string_ids)

            while valid_ids:
                chosen_id = LogitMasker.select(
                    logits,
                    valid_ids,
                )
                chosen_text = self.vocabulary.token_text(chosen_id)
                token_status = self._classify_string_token(
                    chosen_text
                )

                if token_status != "invalid":
                    break

                valid_ids.remove(chosen_id)
            else:
                raise DecodingError(
                    "no safe string token is available"
                )

            quote_position = chosen_text.find('"')

            if quote_position >= 0:
                prefix = chosen_text[:quote_position]

                if prefix:
                    chunks.append(prefix)

                return "".join(chunks)

            chunks.append(chosen_text)
            generated_ids.append(chosen_id)
            input_ids.append(chosen_id)

            repeated_size = self._repeated_suffix_size(
                generated_ids
            )

            if repeated_size is not None:
                del chunks[-repeated_size:]
                return "".join(chunks)

        raise DecodingError(
            "string value exceeded the token limit"
        )

    @staticmethod
    def _classify_string_token(text: str) -> str:
        """Classify a decoded token as content, closing, or invalid."""
        if not text:
            return "invalid"

        prefix = text.split('"', maxsplit=1)[0]

        if any(ord(character) < 32 for character in prefix):
            return "invalid"

        if re.fullmatch(r"<\|.*\|>|<[^>]+>", text):
            return "invalid"

        if '"' in text:
            return "closing"

        return "content"

    @staticmethod
    def _repeated_suffix_size(
        tokens: list[int],
        maximum: int = 8,
    ) -> int | None:
        """Return the size of a repeated suffix, when one exists."""
        for size in range(3, maximum + 1):
            if len(tokens) < size * 2:
                continue

            previous = tokens[-2 * size:-size]
            current = tokens[-size:]

            if current == previous:
                return size

        return None

    @staticmethod
    def _is_special_raw_token(raw_token: str) -> bool:
        """Exclude tokenizer control tokens but retain normal BPE tokens."""
        return bool(
            re.fullmatch(
                r"<\|.*\|>|<[^>]+>",
                raw_token,
            )
        )
