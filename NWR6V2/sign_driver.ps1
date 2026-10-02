# Run this script as Administrator
$ErrorActionPreference = "Stop"
$sys = "C:\Users\Merlyn\Desktop\NWR6V2\Build\Release\SebwettKM.sys"
$sysTmp = $sys + ".tmp"
$sysBak = $sys + ".bak"

# Stop driver service if running
Write-Host "[0] Stopping driver service..."
Stop-Service -Name WinDrvMgr -Force -ErrorAction SilentlyContinue
sc delete WinDrvMgr 2>$null | Out-Null
Start-Sleep -Seconds 3

# Force kill any process using the file
Get-Process | Where-Object { $_.Path -like '*SebwettKM*' } | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1


# Step 1: Create cert with legacy CSP provider
Write-Host "[1] Creating cert with legacy CSP..."
$cert = New-SelfSignedCertificate `
    -Type CodeSigningCert `
    -Subject "CN=SebwettKM_TestCert" `
    -KeyUsage DigitalSignature `
    -KeyAlgorithm RSA `
    -KeyLength 2048 `
    -HashAlgorithm SHA256 `
    -CertStoreLocation Cert:\LocalMachine\My `
    -NotAfter (Get-Date).AddYears(5)
Write-Host "  Thumbprint: $($cert.Thumbprint)"

# Step 2: Check the private key provider
$privKey = $cert.PrivateKey
if ($privKey) {
    Write-Host "  Provider: $($privKey.CspKeyContainerInfo.ProviderName)"
} else {
    Write-Host "  WARNING: No private key found"
}

# Step 3: Export to PFX
Write-Host "`n[2] Exporting to PFX..."
$pfxPath = "C:\Users\Merlyn\Desktop\NWR6V2\Build\Release\export.pfx"
$pfxPass = ConvertTo-SecureString -String "test123" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath $pfxPath -Password $pfxPass
Write-Host "  Exported to: $pfxPath"

# Step 4: Patch PE header to Native subsystem
Write-Host "`n[3] Patching PE header..."
Copy-Item -Path $sys -Destination $sysBak -Force
$bytes = [System.IO.File]::ReadAllBytes($sysBak)
$e = [BitConverter]::ToUInt32($bytes, 0x3C)
$off = [int]$e + 4 + 20 + 68
$bytes[$off] = 1
$bytes[$off + 1] = 0
[System.IO.File]::WriteAllBytes($sysTmp, $bytes)
Copy-Item -Path $sysTmp -Destination $sys -Force
Remove-Item -Path $sysTmp -Force -ErrorAction SilentlyContinue
Write-Host "  Patched to Native"

# Step 5: Sign with signtool + PFX
Write-Host "`n[4] Signing with signtool..."
$signtool = "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe"
& $signtool sign /v /fd SHA256 /f $pfxPath /p "test123" $sys

# Step 6: Verify
Write-Host "`n[5] Verifying..."
$verify = Get-AuthenticodeSignature $sys
Write-Host "  Status: $($verify.Status)"
if ($verify.SignerCertificate) {
    Write-Host "  Signer: $($verify.SignerCertificate.Subject)"
}
