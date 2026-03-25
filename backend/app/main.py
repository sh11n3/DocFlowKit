from pathlib import Path
import shutil

from fastapi import FastAPI, File, UploadFile

# The app instance is intentionally lightweight for local MVP development.
app = FastAPI(title="DocFlowKit API", version="0.1.0")

# Store uploads in a predictable local path for MVP usage.
UPLOAD_DIRECTORY = Path(__file__).resolve().parent.parent / "uploads"


@app.get("/health")
def health() -> dict[str, str]:
    """Simple liveness endpoint for local checks and future orchestration."""
    return {"status": "ok"}


@app.post("/upload")
def upload_file(file: UploadFile = File(...)) -> dict[str, str | int | bool]:
    """Accept one file, create upload directory if needed, and persist locally."""
    # Ensure the upload directory exists before attempting to write files.
    UPLOAD_DIRECTORY.mkdir(parents=True, exist_ok=True)

    # Only keep the basename to avoid client-supplied path traversal segments.
    safe_filename = Path(file.filename or "uploaded_file").name
    destination = UPLOAD_DIRECTORY / safe_filename

    # Stream file contents to disk to keep memory usage low for larger files.
    with destination.open("wb") as output_buffer:
        shutil.copyfileobj(file.file, output_buffer)

    # Read filesystem metadata after write to return canonical file size.
    saved_size = destination.stat().st_size

    return {
        "filename": safe_filename,
        "size": saved_size,
        "success": True,
        "message": "File uploaded successfully"
    }
