"""Reusable constrained-decoding primitives."""

from __future__ import annotations

import math
import re
from collections import Counter
from typing import Sequence

import numpy as np
from pydantic import BaseModel, ConfigDict

from .errors import DecodingError
from .llm import LLMClient

_NUMBER_COMPLETE = re.compile(
    r"^-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?$"
)
_NUMBER_PREFIX = re.compile(
    r"^-?(?:(?:0|[1-9][0-9]*)(?:\.[0-9]*)?"
    r"(?:[eE][+-]?[0-9]*)?)?$"
)
_INTEGER_COMPLETE = re.compile(r"^-?(0|[1-9][0-9]*)$")
_INTEGER_PREFIX = re.compile(r"^-?(?:0|[1-9][0-9]*)?$")


class LogitMasker(BaseModel):
    """Mask invalid logits and select the best remaining token."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    @staticmethod
    def select(logits: Sequence[float], valid_ids: set[int]) -> int:
        """Return argmax after assigning -inf to every invalid token."""
        if not valid_ids:
            raise DecodingError("no valid tokens are available")

        values = np.asarray(logits, dtype=float)
        in_range = {
            token_id
            for token_id in valid_ids
            if 0 <= token_id < len(values)
        }
        if not in_range:
            raise DecodingError("valid token IDs are outside logits range")

        masked = np.full(len(values), -math.inf, dtype=float)
        indices = list(in_range)
        masked[indices] = values[indices]
        return int(np.argmax(masked))

    @staticmethod
    def apply_repetition_penalty(
        logits: Sequence[float],
        generated: Sequence[int],
        penalty: float = 1.25,
    ) -> list[float]:
        """Lower the score of tokens already generated several times."""
        values = np.asarray(logits, dtype=float)
        for token_id, count in Counter(generated).items():
            if not 0 <= token_id < len(values):
                continue
            factor = penalty ** count
            if values[token_id] > 0:
                values[token_id] /= factor
            else:
                values[token_id] *= factor
        return [float(value) for value in values.tolist()]


class CandidateDecoder(BaseModel):
    """Constrain model output to exactly one finite text candidate."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    client: LLMClient

    def choose(
        self,
        base_prompt: str,
        candidates: list[str],
        forced_prefix: str = "",
        terminator: str = "",
    ) -> str:
        """Let model logits select one candidate, token by token."""
        if not candidates:
            raise DecodingError("candidate list cannot be empty")
        if len(set(candidates)) != len(candidates):
            raise DecodingError("candidate strings must be unique")

        input_ids = self.client.encode(base_prompt)
        if forced_prefix:
            input_ids.extend(self.client.encode(forced_prefix))

        paths = [
            (
                candidate,
                self.client.encode(candidate + terminator),
            )
            for candidate in candidates
        ]
        if any(not tokens for _, tokens in paths):
            raise DecodingError("a candidate produced no token IDs")

        generated: list[int] = []
        max_steps = max(len(tokens) for _, tokens in paths)

        for _ in range(max_steps):
            live = [
                (candidate, tokens)
                for candidate, tokens in paths
                if tokens[:len(generated)] == generated
            ]
            completed = [
                candidate
                for candidate, tokens in live
                if len(tokens) == len(generated)
            ]
            if completed:
                return completed[0]

            valid_ids = {
                tokens[len(generated)]
                for _, tokens in live
                if len(tokens) > len(generated)
            }
            logits = self.client.logits(input_ids)
            chosen = LogitMasker.select(logits, valid_ids)
            generated.append(chosen)
            input_ids.append(chosen)

        for candidate, tokens in paths:
            if tokens == generated:
                return candidate
        raise DecodingError(
            f"could not select a candidate from {candidates}"
        )


class JsonNumberRules(BaseModel):
    """Recognise complete and partial JSON numeric literals."""

    model_config = ConfigDict(extra="forbid", strict=True)

    @staticmethod
    def is_complete_number(text: str) -> bool:
        """Return whether text is a complete JSON number."""
        return bool(_NUMBER_COMPLETE.fullmatch(text))

    @staticmethod
    def is_number_prefix(text: str) -> bool:
        """Return whether text can still become a JSON number."""
        return bool(_NUMBER_PREFIX.fullmatch(text))

    @staticmethod
    def is_complete_integer(text: str) -> bool:
        """Return whether text is a complete JSON integer."""
        return bool(_INTEGER_COMPLETE.fullmatch(text))

    @staticmethod
    def is_integer_prefix(text: str) -> bool:
        """Return whether text can still become a JSON integer."""
        return bool(_INTEGER_PREFIX.fullmatch(text))
