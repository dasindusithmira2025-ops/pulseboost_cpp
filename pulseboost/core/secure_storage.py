from __future__ import annotations

import ctypes
from abc import ABC, abstractmethod
from ctypes import wintypes


class SecretCipher(ABC):
    @abstractmethod
    def encrypt(self, plaintext: str) -> bytes:
        raise NotImplementedError

    @abstractmethod
    def decrypt(self, ciphertext: bytes) -> str:
        raise NotImplementedError


class SecretStorageError(RuntimeError):
    pass


class WindowsDPAPICipher(SecretCipher):
    CRYPTPROTECT_UI_FORBIDDEN = 0x01

    class DATA_BLOB(ctypes.Structure):
        _fields_ = [
            ("cbData", wintypes.DWORD),
            ("pbData", ctypes.POINTER(ctypes.c_ubyte)),
        ]

    def __init__(self, *, description: str = "PulseBoost Local Session") -> None:
        if not hasattr(ctypes, "windll"):
            raise SecretStorageError("Windows DPAPI is unavailable on this runtime.")
        self.description = description
        self._crypt32 = ctypes.windll.crypt32
        self._kernel32 = ctypes.windll.kernel32

    def encrypt(self, plaintext: str) -> bytes:
        raw = plaintext.encode("utf-8")
        input_blob, input_buffer = self._blob_from_bytes(raw)
        output_blob = self.DATA_BLOB()
        if not self._crypt32.CryptProtectData(
            ctypes.byref(input_blob),
            self.description,
            None,
            None,
            None,
            self.CRYPTPROTECT_UI_FORBIDDEN,
            ctypes.byref(output_blob),
        ):
            raise SecretStorageError(f"DPAPI encryption failed with code {ctypes.GetLastError()}.")
        try:
            return ctypes.string_at(output_blob.pbData, output_blob.cbData)
        finally:
            if output_blob.pbData:
                self._kernel32.LocalFree(output_blob.pbData)
            del input_buffer

    def decrypt(self, ciphertext: bytes) -> str:
        input_blob, input_buffer = self._blob_from_bytes(ciphertext)
        output_blob = self.DATA_BLOB()
        if not self._crypt32.CryptUnprotectData(
            ctypes.byref(input_blob),
            None,
            None,
            None,
            None,
            self.CRYPTPROTECT_UI_FORBIDDEN,
            ctypes.byref(output_blob),
        ):
            raise SecretStorageError(f"DPAPI decryption failed with code {ctypes.GetLastError()}.")
        try:
            return ctypes.string_at(output_blob.pbData, output_blob.cbData).decode("utf-8")
        finally:
            if output_blob.pbData:
                self._kernel32.LocalFree(output_blob.pbData)
            del input_buffer

    def _blob_from_bytes(self, value: bytes) -> tuple[DATA_BLOB, ctypes.Array[ctypes.c_char]]:
        buffer = ctypes.create_string_buffer(value)
        blob = self.DATA_BLOB(
            cbData=len(value),
            pbData=ctypes.cast(buffer, ctypes.POINTER(ctypes.c_ubyte)),
        )
        return blob, buffer
