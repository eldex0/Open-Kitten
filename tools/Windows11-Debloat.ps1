#requires -Version 5.1
<#
.SYNOPSIS
  Audite ou allège Windows 11 sans désactiver les composants de sécurité.

.DESCRIPTION
  Le mode Audit (par défaut) ne modifie rien. Le mode Apply sauvegarde les
  réglages utilisateur, retire une liste prudente d'applications Appx pour
  l'utilisateur courant et désactive les suggestions publicitaires.

  Le script ne touche jamais à Microsoft Store, Windows Update, Defender,
  Edge WebView2, winget, aux pilotes, aux services ni aux fichiers personnels.

.EXAMPLE
  .\Windows11-Debloat.ps1
  Affiche ce qui serait changé.

.EXAMPLE
  .\Windows11-Debloat.ps1 -Mode Apply
  Applique le profil prudent après une confirmation globale.

.EXAMPLE
  .\Windows11-Debloat.ps1 -Mode Apply -Preset Balanced -RemoveXbox
  Retire aussi les applications optionnelles et Xbox, sans toucher aux services.

.EXAMPLE
  .\Windows11-Debloat.ps1 -Mode Restore -BackupPath "C:\chemin\backup.json"
  Restaure les réglages du Registre sauvegardés. Les applications supprimées
  doivent parfois être réinstallées depuis Microsoft Store.
#>

[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
    [ValidateSet('Audit', 'Apply', 'Restore')]
    [string]$Mode = 'Audit',

    [ValidateSet('Safe', 'Balanced')]
    [string]$Preset = 'Safe',

    [switch]$RemoveXbox,

    [string]$BackupPath,

    [string]$LogPath = (Join-Path $env:TEMP 'Windows11-Debloat.log')
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Stop'
$script:CommandContext = $PSCmdlet

function Write-Log {
    param([string]$Message, [ValidateSet('INFO', 'WARN', 'ERROR')] [string]$Level = 'INFO')
    $line = '{0} [{1}] {2}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $Level, $Message
    Write-Host $line
    try { Add-Content -LiteralPath $LogPath -Value $line -Encoding UTF8 } catch { }
}

function Test-IsAdministrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal $identity
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-Windows11 {
    $caption = 'Windows'
    $build = [Environment]::OSVersion.Version.Build
    try {
        $os = Get-CimInstance Win32_OperatingSystem -ErrorAction Stop
        $caption = $os.Caption
        $build = [int]$os.BuildNumber
    } catch {
        Write-Log "CIM indisponible, détection par numéro de build Windows." 'WARN'
    }
    if ($build -lt 22000) {
        throw "Ce script est réservé à Windows 11 (système détecté : $caption, build $build)."
    }
    Write-Log "Windows 11 détecté : $caption, build $build."
}

$safeApps = @(
    'Clipchamp.Clipchamp',
    'Microsoft.549981C3F5F10',
    'Microsoft.BingNews',
    'Microsoft.BingWeather',
    'Microsoft.GetHelp',
    'Microsoft.Getstarted',
    'Microsoft.MicrosoftOfficeHub',
    'Microsoft.MicrosoftSolitaireCollection',
    'Microsoft.People',
    'Microsoft.WindowsFeedbackHub',
    'Microsoft.WindowsMaps',
    'Microsoft.ZuneMusic',
    'Microsoft.ZuneVideo'
)

$balancedApps = @(
    'Microsoft.OutlookForWindows',
    'Microsoft.Windows.DevHome',
    'Microsoft.YourPhone',
    'MicrosoftCorporationII.MicrosoftFamily'
)

$xboxApps = @(
    'Microsoft.GamingApp',
    'Microsoft.Xbox.TCUI',
    'Microsoft.XboxApp',
    'Microsoft.XboxGameOverlay',
    'Microsoft.XboxGamingOverlay',
    'Microsoft.XboxIdentityProvider',
    'Microsoft.XboxSpeechToTextOverlay'
)

$registrySettings = @(
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\AdvertisingInfo'; Name = 'Enabled'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager'; Name = 'SilentInstalledAppsEnabled'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager'; Name = 'SystemPaneSuggestionsEnabled'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager'; Name = 'SubscribedContent-338388Enabled'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager'; Name = 'SubscribedContent-338389Enabled'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager'; Name = 'SubscribedContent-353694Enabled'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\ContentDeliveryManager'; Name = 'SubscribedContent-353696Enabled'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced'; Name = 'ShowSyncProviderNotifications'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced'; Name = 'TaskbarDa'; Value = 0 },
    [pscustomobject]@{ Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\UserProfileEngagement'; Name = 'ScoobeSystemSettingEnabled'; Value = 0 }
)

function Get-TargetAppNames {
    $names = [Collections.Generic.List[string]]::new()
    foreach ($name in $safeApps) { $names.Add($name) }
    if ($Preset -eq 'Balanced') {
        foreach ($name in $balancedApps) { $names.Add($name) }
    }
    if ($RemoveXbox) {
        foreach ($name in $xboxApps) { $names.Add($name) }
    }
    return @($names | Sort-Object -Unique)
}

function Get-InstalledTargets {
    param([string[]]$Names)
    $result = [Collections.Generic.List[object]]::new()
    foreach ($name in $Names) {
        try {
            foreach ($package in @(Get-AppxPackage -Name $name -ErrorAction Stop)) {
                $result.Add([pscustomobject]@{
                    Name = $package.Name
                    PackageFullName = $package.PackageFullName
                    Version = $package.Version.ToString()
                })
            }
        } catch {
            Write-Log "Inventaire Appx inaccessible dans ce contexte : $($_.Exception.Message)" 'WARN'
            return @()
        }
    }
    return @($result)
}

function Get-RegistryBackup {
    $result = [Collections.Generic.List[object]]::new()
    foreach ($setting in $registrySettings) {
        $exists = $false
        $oldValue = $null
        if (Test-Path -LiteralPath $setting.Path) {
            $property = Get-ItemProperty -LiteralPath $setting.Path -ErrorAction SilentlyContinue
            if ($null -ne $property -and
                $null -ne $property.PSObject.Properties[$setting.Name]) {
                $exists = $true
                $oldValue = $property.PSObject.Properties[$setting.Name].Value
            }
        }
        $result.Add([pscustomobject]@{
            Path = $setting.Path
            Name = $setting.Name
            Existed = $exists
            Value = $oldValue
        })
    }
    return @($result)
}

function Save-Backup {
    param([object[]]$Packages)
    if ([string]::IsNullOrWhiteSpace($script:BackupPath)) {
        $directory = Join-Path $env:LOCALAPPDATA 'OpenKittenTools\Windows11Debloat'
        New-Item -ItemType Directory -Path $directory -Force | Out-Null
        $script:BackupPath = Join-Path $directory ('backup-{0}.json' -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
    } else {
        $parent = Split-Path -Parent $script:BackupPath
        if ($parent) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
    }
    $backup = [ordered]@{
        SchemaVersion = 1
        CreatedAt = (Get-Date).ToString('o')
        ComputerName = $env:COMPUTERNAME
        UserName = [Environment]::UserName
        Registry = @(Get-RegistryBackup)
        RemovedApps = @($Packages)
    }
    $backup | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $script:BackupPath -Encoding UTF8
    Write-Log "Sauvegarde créée : $script:BackupPath"
}

function Show-Audit {
    param([object[]]$Packages)
    Write-Host ''
    Write-Host "Profil sélectionné : $Preset"
    Write-Host "Applications trouvées et ciblées : $($Packages.Count)"
    if ($Packages.Count -gt 0) {
        $Packages | Sort-Object Name | Format-Table Name, Version -AutoSize
    }
    Write-Host "Réglages de suggestions/confidentialité ciblés : $($registrySettings.Count)"
    Write-Host ''
    Write-Host 'Aucune modification effectuée. Utilisez -Mode Apply pour appliquer.'
}

function Apply-Debloat {
    param([object[]]$Packages)
    if (-not $script:CommandContext.ShouldProcess(
            "Windows 11 pour $([Environment]::UserName)",
            "Appliquer le profil $Preset et retirer $($Packages.Count) application(s)")) {
        return
    }

    Save-Backup -Packages $Packages

    if (Test-IsAdministrator) {
        try {
            Enable-ComputerRestore -Drive "$($env:SystemDrive)\" -ErrorAction SilentlyContinue
            Checkpoint-Computer -Description 'Avant Windows11-Debloat' -RestorePointType MODIFY_SETTINGS
            Write-Log 'Point de restauration système créé.'
        } catch {
            Write-Log "Point de restauration non créé : $($_.Exception.Message)" 'WARN'
        }
    } else {
        Write-Log "Exécution sans administrateur : les modifications restent limitées à l'utilisateur courant." 'INFO'
    }

    foreach ($setting in $registrySettings) {
        try {
            if (-not (Test-Path -LiteralPath $setting.Path)) {
                New-Item -Path $setting.Path -Force | Out-Null
            }
            New-ItemProperty -LiteralPath $setting.Path -Name $setting.Name `
                -Value $setting.Value -PropertyType DWord -Force | Out-Null
            Write-Log "Réglage appliqué : $($setting.Path)\$($setting.Name)=$($setting.Value)"
        } catch {
            Write-Log "Échec du réglage $($setting.Name) : $($_.Exception.Message)" 'ERROR'
        }
    }

    foreach ($package in $Packages) {
        try {
            Remove-AppxPackage -Package $package.PackageFullName -ErrorAction Stop
            Write-Log "Application retirée pour l'utilisateur courant : $($package.Name)"
        } catch {
            Write-Log "Impossible de retirer $($package.Name) : $($_.Exception.Message)" 'WARN'
        }
    }

    Write-Host ''
    Write-Host "Terminé. Sauvegarde : $script:BackupPath"
    Write-Host 'Déconnectez-vous puis reconnectez-vous pour appliquer tous les changements visuels.'
}

function Restore-Backup {
    if ([string]::IsNullOrWhiteSpace($BackupPath)) {
        throw 'Le mode Restore nécessite -BackupPath avec le fichier JSON créé par le mode Apply.'
    }
    if (-not (Test-Path -LiteralPath $BackupPath)) {
        throw "Sauvegarde introuvable : $BackupPath"
    }
    $backup = Get-Content -LiteralPath $BackupPath -Raw | ConvertFrom-Json
    if ($backup.SchemaVersion -ne 1) { throw 'Version de sauvegarde non prise en charge.' }
    if (-not $script:CommandContext.ShouldProcess(
            "Windows 11 pour $([Environment]::UserName)",
            "Restaurer les réglages depuis $BackupPath")) {
        return
    }
    foreach ($entry in @($backup.Registry)) {
        try {
            if ($entry.Existed) {
                if (-not (Test-Path -LiteralPath $entry.Path)) {
                    New-Item -Path $entry.Path -Force | Out-Null
                }
                New-ItemProperty -LiteralPath $entry.Path -Name $entry.Name `
                    -Value $entry.Value -PropertyType DWord -Force | Out-Null
            } elseif (Test-Path -LiteralPath $entry.Path) {
                Remove-ItemProperty -LiteralPath $entry.Path -Name $entry.Name `
                    -ErrorAction SilentlyContinue
            }
            Write-Log "Réglage restauré : $($entry.Path)\$($entry.Name)"
        } catch {
            Write-Log "Échec de restauration de $($entry.Name) : $($_.Exception.Message)" 'ERROR'
        }
    }
    if (@($backup.RemovedApps).Count -gt 0) {
        Write-Host ''
        Write-Host 'Les réglages ont été restaurés. Les applications suivantes peuvent être réinstallées depuis Microsoft Store :'
        @($backup.RemovedApps) | ForEach-Object { Write-Host "  - $($_.Name)" }
    }
}

try {
    Test-Windows11
    if ($Mode -eq 'Restore') {
        Restore-Backup
        exit 0
    }
    $targets = Get-TargetAppNames
    $installed = @(Get-InstalledTargets -Names $targets)
    if ($Mode -eq 'Audit') {
        Show-Audit -Packages $installed
    } else {
        Apply-Debloat -Packages $installed
    }
} catch {
    Write-Log $_.Exception.Message 'ERROR'
    exit 1
}
