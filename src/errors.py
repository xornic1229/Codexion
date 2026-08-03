"""Expected errors raised by the Call Me Maybe application."""


class CallMeMaybeError(Exception):
    """Base class for errors that can be shown cleanly to the user."""


class InputError(CallMeMaybeError):
    """Raised when an input or output file cannot be handled."""


class ModelError(CallMeMaybeError):
    """Raised when the subject-provided LLM SDK fails."""


class DecodingError(CallMeMaybeError):
    """Raised when constrained decoding cannot finish a valid value."""
