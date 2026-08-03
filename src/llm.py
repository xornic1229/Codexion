"""Small compatibility layer over the subject-provided llm_sdk."""

from __future__ import annotations

from typing import Any, Sequence, cast

from pydantic import BaseModel, ConfigDict, PrivateAttr

from .errors import ModelError


class LLMClient(BaseModel):
    """Expose the SDK through a predictable list-based public API."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    model_name: str = "Qwen/Qwen3-0.6B"

    _model: Any = PrivateAttr(default=None)

    def model_post_init(self, __context: Any) -> None:
        """Create Small_LLM_Model after Pydantic initialisation."""
        try:
            from llm_sdk import Small_LLM_Model
        except ImportError as exc:
            raise ModelError(
                "llm_sdk is unavailable; run 'make install' from the "
                "project root"
            ) from exc

        try:
            self._model = Small_LLM_Model(self.model_name)
        except Exception as exc:
            raise ModelError(f"could not initialise the LLM: {exc}") from exc

    def encode(self, text: str) -> list[int]:
        """Encode text with the SDK and return a flat list of token IDs."""
        try:
            raw = self._model.encode(text)
        except Exception as exc:
            raise ModelError(f"token encoding failed: {exc}") from exc
        return self._normalise_token_ids(raw)

    def decode(self, token_ids: Sequence[int]) -> str:
        """Decode token IDs with the public SDK method."""
        try:
            return cast(str, self._model.decode(list(token_ids)))
        except Exception as exc:
            raise ModelError(f"token decoding failed: {exc}") from exc

    def logits(self, input_ids: Sequence[int]) -> list[float]:
        """Return next-token logits as a plain list of floats."""
        try:
            raw = self._model.get_logits_from_input_ids(list(input_ids))
        except Exception as exc:
            raise ModelError(f"model inference failed: {exc}") from exc

        if hasattr(raw, "detach"):
            raw = raw.detach()
        if hasattr(raw, "cpu"):
            raw = raw.cpu()
        if hasattr(raw, "numpy"):
            raw = raw.numpy()
        if hasattr(raw, "tolist"):
            raw = raw.tolist()

        if not isinstance(raw, list):
            raise ModelError(
                f"unexpected logits type returned by SDK: {type(raw)!r}"
            )
        return [float(value) for value in raw]

    def vocab_path(self) -> str:
        """Return the vocabulary path using a public SDK method."""
        try:
            if hasattr(self._model, "get_path_to_vocab_file"):
                return cast(str, self._model.get_path_to_vocab_file())
            if hasattr(self._model, "get_path_to_vocabulary_json"):
                return cast(
                    str,
                    self._model.get_path_to_vocabulary_json(),
                )
        except Exception as exc:
            raise ModelError(f"could not load vocabulary path: {exc}") from exc
        raise ModelError("SDK does not expose a public vocabulary method")

    @staticmethod
    def _normalise_token_ids(raw: Any) -> list[int]:
        """Convert Tensor-like encode output into a flat list[int]."""
        if hasattr(raw, "detach"):
            raw = raw.detach()
        if hasattr(raw, "cpu"):
            raw = raw.cpu()
        if hasattr(raw, "numpy"):
            raw = raw.numpy()
        if hasattr(raw, "tolist"):
            raw = raw.tolist()

        while (
            isinstance(raw, list)
            and len(raw) == 1
            and isinstance(raw[0], list)
        ):
            raw = raw[0]

        if not isinstance(raw, list):
            raise ModelError(
                f"unexpected encode output type: {type(raw)!r}"
            )
        return [int(token_id) for token_id in raw]
