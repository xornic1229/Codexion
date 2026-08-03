"""Write validated function calls to the required JSON output file."""

from __future__ import annotations

import json
from pathlib import Path

from pydantic import BaseModel, ConfigDict

from .errors import InputError
from .models import FunctionCall


class OutputWriter(BaseModel):
    """Create the output directory and serialize exact output objects."""

    model_config = ConfigDict(arbitrary_types_allowed=True)

    output_path: Path

    def write(self, calls: list[FunctionCall]) -> None:
        """Write a parseable JSON array with no extra fields."""
        try:
            self.output_path.parent.mkdir(parents=True, exist_ok=True)
            with self.output_path.open("w", encoding="utf-8") as file:
                json.dump(
                    [call.model_dump() for call in calls],
                    file,
                    ensure_ascii=False,
                    indent=2,
                )
                file.write("\n")
        except PermissionError as exc:
            raise InputError(
                f"permission denied while writing: {self.output_path}"
            ) from exc
        except OSError as exc:
            raise InputError(
                f"could not write {self.output_path}: {exc}"
            ) from exc
