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
        """Pre-compute cheap candidate sets from raw vocabulary strings."""
        numeric_chars = set("0123456789.eE+-")
        for token_id, raw in self.vocabulary.id_to_token.items():
            if raw and all(character in numeric_chars for character in raw):
                self._numeric_ids.add(token_id)
            if raw and raw[0] in {",", "}"}:
                self._separator_ids.add(token_id)
            if raw.startswith("Ċ") or raw.startswith("ĉ"):
                self._separator_ids.add(token_id)
            if raw and not self._is_special_raw_token(raw):
                self._string_ids.add(token_id)

    def generate(
        self,
        prompt: PromptItem,
        function: FunctionDefinition,
    ) -> dict[str, JsonValue]:
        """Generate all required values and return a typed dictionary."""
        if not function.parameters:
            return {}

        builder = PromptBuilder(functions=[function])
        context = builder.build_parameter_prompt(prompt, function) + "{\n"
        parameters: dict[str, JsonValue] = {}
        total = len(function.parameters)

        for index, (name, specification) in enumerate(
            function.parameters.items()
        ):
            key_text = f'  "{name}": '
            context += key_text
            value, raw_value = self._decode_value(
                context,
                specification,
            )
            parameters[name] = value
            context += raw_value
            context += "\n}" if index == total - 1 else ",\n"

        return parameters

    def _decode_value(
        self,
        context: str,
        specification: FunctionParameter,
    ) -> tuple[JsonValue, str]:
        """Dispatch a parameter to its type-specific decoder."""
        if specification.type == "number":
            raw = self._decode_numeric(
                context,
                JsonNumberRules.is_number_prefix,
                JsonNumberRules.is_complete_number,
            )
            return float(raw), raw
        if specification.type == "integer":
            raw = self._decode_numeric(
                context,
                JsonNumberRules.is_integer_prefix,
                JsonNumberRules.is_complete_integer,
            )
            return int(raw), raw
        if specification.type == "boolean":
            raw = CandidateDecoder(client=self.client).choose(
                context,
                ["true", "false"],
            )
            return raw == "true", raw
        if specification.type == "string":
            value = self._decode_string(context)
            return value, json.dumps(value, ensure_ascii=False)
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
        """Generate a complete JSON numeric literal token by token."""
        input_ids = self.client.encode(context)
        text = ""

        for _ in range(self.max_number_tokens):
            valid_ids = {
                token_id
                for token_id in self._numeric_ids
                if self._numeric_token_allowed(
                    token_id,
                    text,
                    is_prefix,
                )
            }
            if is_complete(text):
                valid_ids.update(self._separator_ids)

            logits = self.client.logits(input_ids)
            chosen = LogitMasker.select(logits, valid_ids)
            token_text = self.vocabulary.token_text(chosen)

            if chosen in self._separator_ids and is_complete(text):
                return text

            candidate = text + token_text
            if not is_prefix(candidate):
                raise DecodingError(
                    f"model selected invalid numeric token {token_text!r}"
                )
            text = candidate
            input_ids.append(chosen)

        if is_complete(text):
            return text
        raise DecodingError(f"numeric value did not finish: {text!r}")

    def _numeric_token_allowed(
        self,
        token_id: int,
        current: str,
        is_prefix: Callable[[str], bool],
    ) -> bool:
        """Check whether a decoded token preserves a numeric prefix."""
        token_text = self.vocabulary.token_text(token_id)
        if not token_text:
            return False
        if any(character.isspace() for character in token_text):
            return False
        return is_prefix(current + token_text)

    def _decode_string(self, context: str) -> str:
        """Generate string content; json.dump later performs escaping."""
        input_ids = self.client.encode(context + '"')
        chunks: list[str] = []
        generated_ids: list[int] = []

        for _ in range(self.max_string_tokens):
            logits = self.client.logits(input_ids)
            valid_ids = set(self._string_ids)

            while valid_ids:
                chosen = LogitMasker.select(logits, valid_ids)
                token_text = self.vocabulary.token_text(chosen)
                status = self._classify_string_token(token_text)
                if status != "invalid":
                    break
                valid_ids.remove(chosen)
            else:
                raise DecodingError("no safe string token is available")

            quote_position = token_text.find('"')
            if quote_position >= 0:
                prefix = token_text[:quote_position]
                if prefix:
                    chunks.append(prefix)
                return "".join(chunks)

            chunks.append(token_text)
            generated_ids.append(chosen)
            input_ids.append(chosen)

            repeated = self._repeated_suffix_size(generated_ids)
            if repeated is not None:
                del chunks[-repeated:]
                return "".join(chunks)

        raise DecodingError("string value exceeded token limit")

    @staticmethod
    def _classify_string_token(text: str) -> str:
        """Classify one decoded token as content, closing, or invalid."""
        if not text:
            return "invalid"
        prefix = text.split('"', maxsplit=1)[0]
        if any(ord(character) < 32 for character in prefix):
            return "invalid"
        if re.fullmatch(r"<\|.*\|>|<[^>]+>", text):
            return "invalid"
        return "closing" if '"' in text else "content"

    @staticmethod
    def _repeated_suffix_size(
        tokens: list[int],
        maximum: int = 8,
    ) -> int | None:
        """Return repeated suffix length, used as an emergency loop stop."""
        for size in range(3, maximum + 1):
            if len(tokens) < size * 2:
                continue
            if tokens[-size:] == tokens[-2 * size:-size]:
                return size
        return None

    @staticmethod
    def _is_special_raw_token(raw: str) -> bool:
        """Exclude tokenizer control tokens but retain normal BPE tokens."""
        return bool(re.fullmatch(r"<\|.*\|>|<[^>]+>", raw))
