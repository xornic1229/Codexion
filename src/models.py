"""Pydantic models for input schemas and generated function calls."""

from __future__ import annotations

from pydantic import BaseModel, ConfigDict, field_validator, model_validator

VALID_TYPES: set[str] = {
    "number",
    "integer",
    "string",
    "boolean",
    "null",
}

JsonValue = str | int | float | bool | None


class PromptItem(BaseModel):
    """One natural-language request from the prompt input file."""

    model_config = ConfigDict(extra="forbid", strict=True)

    prompt: str


class FunctionParameter(BaseModel):
    """The declared JSON type of one required function parameter."""

    model_config = ConfigDict(extra="forbid", strict=True)

    type: str

    @field_validator("type")
    @classmethod
    def validate_type(cls, value: str) -> str:
        """Reject parameter types that this subject version does not use."""
        if value not in VALID_TYPES:
            allowed = ", ".join(sorted(VALID_TYPES))
            raise ValueError(
                f"unsupported parameter type '{value}'; allowed: {allowed}"
            )
        return value


class FunctionReturn(BaseModel):
    """The declared JSON return type of a function definition."""

    model_config = ConfigDict(extra="forbid", strict=True)

    type: str

    @field_validator("type")
    @classmethod
    def validate_type(cls, value: str) -> str:
        """Reject return types that this subject version does not use."""
        if value not in VALID_TYPES:
            allowed = ", ".join(sorted(VALID_TYPES))
            raise ValueError(
                f"unsupported return type '{value}'; allowed: {allowed}"
            )
        return value


class FunctionDefinition(BaseModel):
    """One function that the LLM is allowed to select."""

    model_config = ConfigDict(extra="forbid", strict=True)

    name: str
    description: str
    parameters: dict[str, FunctionParameter]
    returns: FunctionReturn

    @field_validator("name", "description")
    @classmethod
    def validate_non_empty(cls, value: str) -> str:
        """Reject empty function names and descriptions."""
        if not value.strip():
            raise ValueError("field cannot be empty")
        return value

    @field_validator("parameters")
    @classmethod
    def validate_parameter_names(
        cls,
        value: dict[str, FunctionParameter],
    ) -> dict[str, FunctionParameter]:
        """Reject empty parameter names."""
        if any(not name.strip() for name in value):
            raise ValueError("parameter names cannot be empty")
        return value

    def prototype(self) -> str:
        """Return a short prototype included in the LLM prompt."""
        parameters = ", ".join(
            f"{name}: {spec.type}"
            for name, spec in self.parameters.items()
        )
        return f"{self.name}({parameters}) -> {self.returns.type}"


class InputData(BaseModel):
    """All validated prompts and function definitions."""

    model_config = ConfigDict(extra="forbid", strict=True)

    prompts: list[PromptItem]
    functions: list[FunctionDefinition]

    @model_validator(mode="after")
    def validate_functions(self) -> "InputData":
        """Require at least one function and unique function names."""
        if not self.functions:
            raise ValueError("at least one function definition is required")

        names = [function.name for function in self.functions]
        duplicates = {
            name for name in names if names.count(name) > 1
        }
        if duplicates:
            text = ", ".join(sorted(duplicates))
            raise ValueError(f"duplicated function names: {text}")
        return self


class FunctionCall(BaseModel):
    """One object written to function_calling_results.json."""

    model_config = ConfigDict(extra="forbid", strict=True)

    prompt: str
    name: str
    parameters: dict[str, JsonValue]

    @field_validator("name")
    @classmethod
    def validate_name(cls, value: str) -> str:
        """Reject an empty selected function name."""
        if not value.strip():
            raise ValueError("function name cannot be empty")
        return value


def validate_parameters(
    function: FunctionDefinition,
    parameters: dict[str, JsonValue],
) -> None:
    """Ensure generated keys and Python values match the function schema."""
    expected = set(function.parameters)
    received = set(parameters)
    if expected != received:
        missing = sorted(expected - received)
        extra = sorted(received - expected)
        raise ValueError(
            f"parameter keys do not match schema; "
            f"missing={missing}, extra={extra}"
        )

    for name, specification in function.parameters.items():
        value = parameters[name]
        declared = specification.type
        valid = False

        if declared == "number":
            valid = isinstance(value, (int, float)) and not isinstance(
                value, bool
            )
        elif declared == "integer":
            valid = isinstance(value, int) and not isinstance(value, bool)
        elif declared == "string":
            valid = isinstance(value, str)
        elif declared == "boolean":
            valid = isinstance(value, bool)
        elif declared == "null":
            valid = value is None

        if not valid:
            raise ValueError(
                f"parameter '{name}' must have type '{declared}', "
                f"got {type(value).__name__}"
            )
