# Code Signing

Windows desktop alpha builds should be code signed before distribution so Windows, SmartScreen, antivirus tools, and users can identify the publisher and verify the executable has not been modified after release.

## Certificate Types

- OV certificate: organization-validated signing certificate. Suitable for early commercial distribution, but SmartScreen reputation usually builds over time.
- EV certificate: extended-validation signing certificate. Usually requires a hardware token or cloud HSM flow and can establish stronger initial trust signals.

Do not commit certificates, private keys, passwords, tokens, PFX files, PEM files, or signing logs containing secrets.

## Release Pipeline Placement

Signing should happen after the Release build and package validation pass, and before installer publication:

1. Build `PulseBoostAI.exe`.
2. Deploy Qt runtime files with `windeployqt`.
3. Run `scripts/test.ps1`, `scripts/release-validate.ps1`, and `scripts/package-validate.ps1`.
4. Sign `PulseBoostAI.exe` and any other shipped executable binaries.
5. Build the Inno Setup installer.
6. Sign the installer.
7. Verify signatures on a clean Windows machine.

## SignTool Examples

Use placeholders only until the project owner provides the real signing method.

```powershell
signtool sign `
  /fd SHA256 `
  /td SHA256 `
  /tr "https://timestamp.example.com" `
  /f "C:\path\to\placeholder-certificate.pfx" `
  /p "<PFX_PASSWORD_FROM_SECRET_STORE>" `
  "build\Release\PulseBoostAI.exe"
```

```powershell
signtool sign `
  /fd SHA256 `
  /td SHA256 `
  /tr "https://timestamp.example.com" `
  /f "C:\path\to\placeholder-certificate.pfx" `
  /p "<PFX_PASSWORD_FROM_SECRET_STORE>" `
  "build\Installer\PulseBoostAI_Setup.exe"
```

Verify signatures before publishing:

```powershell
signtool verify /pa /v "build\Release\PulseBoostAI.exe"
signtool verify /pa /v "build\Installer\PulseBoostAI_Setup.exe"
```

## GitHub Actions Secret Placeholders

Future CI signing should use a secure certificate store or GitHub Actions secrets. Placeholder names:

- `WINDOWS_SIGNING_CERT_BASE64`
- `WINDOWS_SIGNING_CERT_PASSWORD`
- `WINDOWS_SIGNING_TIMESTAMP_URL`

The timestamp URL should come from the certificate authority or signing provider. Do not hardcode real credentials or certificate material in workflow YAML.
