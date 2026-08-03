"""Vocabulary maps and token text helpers for constrained decoding."""

from __future__ import annotations

import json
from typing import Any

from pydantic import BaseModel, ConfigDict, PrivateAttr

from .errors import ModelError
from .llm import LLMClient


class Vocabulary(BaseModel):
    """Map vocabulary strings to IDs and decode individual IDs lazily."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    token_to_id: dict[str, int]
    id_to_token: dict[int, str]

    _client: LLMClient = PrivateAttr()
    _decoded_cache: dict[int, str] = PrivateAttr(default_factory=dict)

    @classmethod
    def from_client(cls, client: LLMClient) -> "Vocabulary":
        """Read the vocabulary JSON exposed by the SDK."""
        path = client.vocab_path()
        try:
            with open(path, "r", encoding="utf-8") as file:
                data: Any = json.load(file)
        except FileNotFoundError as exc:
            raise ModelError(f"vocabulary file not found: {path}") from exc
        except json.JSONDecodeError as exc:
            raise ModelError(f"invalid vocabulary JSON: {path}") from exc
        except OSError as exc:
            raise ModelError(f"could not read vocabulary: {exc}") from exc

        if not isinstance(data, dict):
            raise ModelError("vocabulary JSON must contain an object")

        try:
            token_to_id = {
                str(token): int(token_id)
                for token, token_id in data.items()
            }
        except (TypeError, ValueError) as exc:
            raise ModelError("vocabulary contains invalid token IDs") from exc

        id_to_token = {
            token_id: token for token, token_id in token_to_id.items()
        }
        vocabulary = cls(
            token_to_id=token_to_id,
            id_to_token=id_to_token,
        )
        vocabulary._client = client
        return vocabulary

    @property
    def all_ids(self) -> set[int]:
        """Return every token ID present in the vocabulary."""
        return set(self.id_to_token)

    def raw_token(self, token_id: int) -> str:
        """Return the raw representation stored in vocab.json."""
        return self.id_to_token.get(token_id, "")

    def token_text(self, token_id: int) -> str:
        """Decode one token ID and cache its actual output text."""
        if token_id not in self._decoded_cache:
            self._decoded_cache[token_id] = self._client.decode([token_id])
        return self._decoded_cache[token_id]
