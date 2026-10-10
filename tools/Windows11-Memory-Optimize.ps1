#requires -Version 5.1

<#+
.SYNOPSIS
    Analyse et réduit prudemment l'utilisation de la mémoire sous Windows 11.

.DESCRIPTION
    Le mode Audit ne modifie rien. Le mode Apply applique des réglages visuels
    réversibles au compte Windows courant. Le mode Restore restaure la dernière
    sauvegarde, ou celle indiquée avec -BackupPath.

    Ce script ne désactive pas le fichier d'échange, SysMain, Microsoft Defender,
    Windows Update ou l'intégrité de la mémoire. Il ne vide pas artificiellement
    les working sets des processus.

.EXAMPLE
    .\Windows11-Memory-Optimize.ps1 -Mode Audit

.EXAMPLE
    .\Windows11-Memory-Optimize.ps1 -Mode Apply

.EXAMPLE
    .\Windows11-Memory-Optimize.ps1 -Mode Apply -Profile Performance -CloseOptionalApps

.EXAMPLE
    .\Windows11-Memory-Optimize.ps1 -Mode Restore
#>

[CmdletBinding(SupportsShouldProcess = $true, ConfirmImpact = 'High')]
param(
    [ValidateSet('Audit', 'Apply', 'Restore')]
    [string]$Mode = 'Audit',

    [ValidateSet('Light', 'Performance')]
    [string]$Profile = 'Light',

    [ValidateRange(5, 50)]
    [int]$Top = 12,

    [switch]$CloseOptionalApps,

    [string]$BackupPath,

    [string]$LogPath
)

Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'
$script:CommandContext = $PSCmdlet
$script:ToolRoot = Join-Path $env:LOCALAPPDATA 'OpenKittenTools\Windows11Memory'

if (-not $LogPath) {
    $LogPath = Join-Path $script:ToolRoot 'memory-optimize.log'
}

function Write-Log {
    param(
        [Parameter(Mandatory = $true)][string]$Message,
        [ValidateSet('INFO', 'WARN', 'ERROR')][string]$Level = 'INFO'
    )

    $line = '{0} [{1}] {2}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $Level, $Message
    Write-Host $line

    try {
        $logDirectory = Split-Path -Parent $LogPath
        if ($logDirectory -and -not (Test-Path -LiteralPath $logDirectory)) {
            New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
        }
        Add-Content -LiteralPath $LogPath -Value $line -Encoding UTF8
    }
    catch {
        Write-Warning "Impossible d'écrire le journal : $($_.Exception.Message)"
    }
}

function Get-WindowsBuild {
    try {
        return [int](Get-CimInstance -ClassName Win32_OperatingSystem -ErrorAction Stop).BuildNumber
    }
    catch {
        return [Environment]::OSVersion.Version.Build
    }
}

function Assert-Windows11 {
    if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT) {
        throw 'Ce script fonctionne uniquement sous Windows.'
    }

    $build = Get-WindowsBuild
    if ($build -lt 22000) {
        throw "Windows 11 est requis (build détecté : $build)."
    }
}

function Initialize-NativeMemoryType {
    if ('OpenKitten.NativeMemory' -as [type]) {
        return
    }

    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

namespace OpenKitten {
    public static class NativeMemory {
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        private class MEMORYSTATUSEX {
            public uint dwLength = (uint)Marshal.SizeOf(typeof(MEMORYSTATUSEX));
            public uint dwMemoryLoad;
            public ulong ullTotalPhys;
            public ulong ullAvailPhys;
            public ulong ullTotalPageFile;
            public ulong ullAvailPageFile;
            public ulong ullTotalVirtual;
            public ulong ullAvailVirtual;
            public ulong ullAvailExtendedVirtual;
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
        private static extern bool GlobalMemoryStatusEx([In, Out] MEMORYSTATUSEX buffer);

        public static ulong[] GetPhysicalMemory() {
            MEMORYSTATUSEX status = new MEMORYSTATUSEX();
            if (!GlobalMemoryStatusEx(status)) {
                throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
            }
            return new ulong[] { status.ullTotalPhys, status.ullAvailPhys };
        }
    }
}
'@
}

function Get-MemorySnapshot {
    Initialize-NativeMemoryType
    $values = [OpenKitten.NativeMemory]::GetPhysicalMemory()
    $total = [double]$values[0]
    $available = [double]$values[1]
    $used = $total - $available

    [pscustomobject]@{
        TotalGB     = [math]::Round($total / 1GB, 2)
        UsedGB      = [math]::Round($used / 1GB, 2)
        AvailableGB = [math]::Round($available / 1GB, 2)
        UsedPercent = if ($total -gt 0) { [math]::Round(($used / $total) * 100, 1) } else { 0 }
    }
}

function Get-TopMemoryProcesses {
    param([int]$Count)

    Get-Process -ErrorAction SilentlyContinue |
        Group-Object -Property ProcessName |
        ForEach-Object {
            $workingSet = ($_.Group | Measure-Object -Property WorkingSet64 -Sum).Sum
            [pscustomobject]@{
                Application = $_.Name
                Processes   = $_.Count
                MemoryMB    = [math]::Round($workingSet / 1MB, 1)
            }
        } |
        Sort-Object -Property MemoryMB -Descending |
        Select-Object -First $Count
}

function Get-StartupEntries {
    $locations = @(
        'HKCU:\Software\Microsoft\Windows\CurrentVersion\Run',
        'HKLM:\Software\Microsoft\Windows\CurrentVersion\Run'
    )

    $entries = foreach ($location in $locations) {
        if (-not (Test-Path -LiteralPath $location)) {
            continue
        }

        $item = Get-ItemProperty -LiteralPath $location -ErrorAction SilentlyContinue
        if (-not $item) {
            continue
        }

        foreach ($property in $item.PSObject.Properties) {
            if ($property.Name -notmatch '^PS(Path|ParentPath|ChildName|Drive|Provider)$') {
                [pscustomobject]@{
                    Scope   = if ($location.StartsWith('HKCU:')) { 'Utilisateur' } else { 'Machine' }
                    Name    = $property.Name
                    Command = [string]$property.Value
                }
            }
        }
    }

    return @($entries)
}

function Get-OptionalAppProcesses {
    $optionalNames = @(
        'Widgets',
        'WidgetService',
        'ms-teams',
        'msedgewebview2',
        'PhoneExperienceHost',
        'GameBar',
        'GameBarFTServer',
        'XboxPcApp',
        'Copilot'
    )

    $processes = Get-Process -ErrorAction SilentlyContinue | Where-Object {
        $optionalNames -contains $_.ProcessName
    }

    # WebView2 peut être utilisé par beaucoup d'applications. Il est affiché dans
    # l'audit, mais n'est jamais fermé automatiquement par ce script.
    return @($processes)
}

function Get-RegistrySettings {
    param([string]$SelectedProfile)

    $settings = @(
        [pscustomobject]@{
            Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize'
            Name = 'EnableTransparency'
            Value = 0
            Type = 'DWord'
            Profile = 'Light'
            Description = 'Désactiver les effets de transparence'
        },
        [pscustomobject]@{
            Path = 'HKCU:\Control Panel\Desktop\WindowMetrics'
            Name = 'MinAnimate'
            Value = '0'
            Type = 'String'
            Profile = 'Light'
            Description = "Désactiver l'animation de réduction des fenêtres"
        },
        [pscustomobject]@{
            Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced'
            Name = 'TaskbarAnimations'
            Value = 0
            Type = 'DWord'
            Profile = 'Light'
            Description = 'Désactiver les animations de la barre des tâches'
        },
        [pscustomobject]@{
            Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced'
            Name = 'ListviewAlphaSelect'
            Value = 0
            Type = 'DWord'
            Profile = 'Performance'
            Description = 'Désactiver le rectangle de sélection translucide'
        },
        [pscustomobject]@{
            Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced'
            Name = 'ListviewShadow'
            Value = 0
            Type = 'DWord'
            Profile = 'Performance'
            Description = "Désactiver l'ombre des étiquettes du Bureau"
        },
        [pscustomobject]@{
            Path = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\VisualEffects'
            Name = 'VisualFXSetting'
            Value = 2
            Type = 'DWord'
            Profile = 'Performance'
            Description = 'Privilégier les performances pour les effets visuels'
        }
    )

    if ($SelectedProfile -eq 'Performance') {
        return $settings
    }

    return @($settings | Where-Object { $_.Profile -eq 'Light' })
}

function Get-RegistryValueState {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Type
    )

    $exists = $false
    $value = $null
    if (Test-Path -LiteralPath $Path) {
        $item = Get-ItemProperty -LiteralPath $Path -Name $Name -ErrorAction SilentlyContinue
        if ($null -ne $item) {
            $exists = $true
            $value = $item.$Name
        }
    }

    [pscustomobject]@{
        Path = $Path
        Name = $Name
        Type = $Type
        Existed = $exists
        Value = $value
    }
}

function Save-RegistryBackup {
    param([object[]]$Settings)

    if (-not (Test-Path -LiteralPath $script:ToolRoot)) {
        New-Item -ItemType Directory -Path $script:ToolRoot -Force | Out-Null
    }

    $target = if ($BackupPath) {
        $BackupPath
    }
    else {
        Join-Path $script:ToolRoot ('backup-{0}.json' -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
    }

    $targetDirectory = Split-Path -Parent $target
    if ($targetDirectory -and -not (Test-Path -LiteralPath $targetDirectory)) {
        New-Item -ItemType Directory -Path $targetDirectory -Force | Out-Null
    }

    $backup = [pscustomobject]@{
        Version = 1
        CreatedAt = (Get-Date).ToString('o')
        Computer = $env:COMPUTERNAME
        User = $env:USERNAME
        Settings = @($Settings | ForEach-Object {
            Get-RegistryValueState -Path $_.Path -Name $_.Name -Type $_.Type
        })
    }

    $backup | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $target -Encoding UTF8
    Set-Content -LiteralPath (Join-Path $script:ToolRoot 'latest-backup.txt') -Value $target -Encoding UTF8
    Write-Log "Sauvegarde créée : $target"
    return $target
}

function Set-RegistrySetting {
    param([Parameter(Mandatory = $true)]$Setting)

    if (-not (Test-Path -LiteralPath $Setting.Path)) {
        New-Item -Path $Setting.Path -Force | Out-Null
    }

    New-ItemProperty -LiteralPath $Setting.Path -Name $Setting.Name -Value $Setting.Value -PropertyType $Setting.Type -Force | Out-Null
    Write-Log $Setting.Description
}

function Resolve-RestoreBackup {
    if ($BackupPath) {
        return $BackupPath
    }

    $pointer = Join-Path $script:ToolRoot 'latest-backup.txt'
    if (-not (Test-Path -LiteralPath $pointer)) {
        throw 'Aucune sauvegarde trouvée. Utilisez -BackupPath pour en indiquer une.'
    }

    return (Get-Content -LiteralPath $pointer -Raw).Trim()
}

function Restore-RegistryBackup {
    $source = Resolve-RestoreBackup
    if (-not (Test-Path -LiteralPath $source)) {
        throw "Sauvegarde introuvable : $source"
    }

    $backup = Get-Content -LiteralPath $source -Raw | ConvertFrom-Json
    if (-not $backup.Settings) {
        throw 'La sauvegarde ne contient aucun réglage.'
    }

    if (-not $script:CommandContext.ShouldProcess('les réglages visuels du compte courant', "restaurer la sauvegarde $source")) {
        return
    }

    foreach ($setting in $backup.Settings) {
        if ($setting.Existed) {
            if (-not (Test-Path -LiteralPath $setting.Path)) {
                New-Item -Path $setting.Path -Force | Out-Null
            }
            New-ItemProperty -LiteralPath $setting.Path -Name $setting.Name -Value $setting.Value -PropertyType $setting.Type -Force | Out-Null
        }
        elseif (Test-Path -LiteralPath $setting.Path) {
            Remove-ItemProperty -LiteralPath $setting.Path -Name $setting.Name -ErrorAction SilentlyContinue
        }
    }

    Write-Log "Réglages restaurés depuis : $source"
    Write-Host 'Déconnectez-vous puis reconnectez-vous pour appliquer tous les changements.' -ForegroundColor Yellow
}

function Stop-OptionalApps {
    $processes = @(Get-OptionalAppProcesses | Where-Object { $_.ProcessName -ne 'msedgewebview2' })
    if ($processes.Count -eq 0) {
        Write-Log "Aucune application facultative ciblée n'est ouverte."
        return
    }

    foreach ($process in $processes) {
        if ($script:CommandContext.ShouldProcess("$($process.ProcessName) (PID $($process.Id))", 'fermer le processus facultatif')) {
            try {
                Stop-Process -Id $process.Id -ErrorAction Stop
                Write-Log "Processus fermé : $($process.ProcessName) (PID $($process.Id))"
            }
            catch {
                Write-Log "Impossible de fermer $($process.ProcessName) : $($_.Exception.Message)" 'WARN'
            }
        }
    }
}

function Show-Audit {
    $memory = Get-MemorySnapshot
    Write-Host ''
    Write-Host 'Mémoire physique' -ForegroundColor Cyan
    $memory | Format-Table -AutoSize

    Write-Host 'Processus les plus gourmands (regroupés par application)' -ForegroundColor Cyan
    Get-TopMemoryProcesses -Count $Top | Format-Table -AutoSize

    $startup = @(Get-StartupEntries)
    Write-Host "Applications de démarrage détectées : $($startup.Count)" -ForegroundColor Cyan
    if ($startup.Count -gt 0) {
        $startup | Select-Object Scope, Name | Format-Table -AutoSize
    }

    $optional = @(Get-OptionalAppProcesses)
    Write-Host "Processus facultatifs actuellement ouverts : $($optional.Count)" -ForegroundColor Cyan
    if ($optional.Count -gt 0) {
        $optional |
            Group-Object ProcessName |
            ForEach-Object {
                [pscustomobject]@{
                    Application = $_.Name
                    Processes = $_.Count
                    MemoryMB = [math]::Round((($_.Group | Measure-Object WorkingSet64 -Sum).Sum) / 1MB, 1)
                }
            } |
            Sort-Object MemoryMB -Descending |
            Format-Table -AutoSize
    }
}

try {
    Assert-Windows11
    Write-Log "Démarrage : mode=$Mode, profil=$Profile"

    switch ($Mode) {
        'Audit' {
            Show-Audit
            Write-Host 'Aucun changement effectué.' -ForegroundColor Green
        }

        'Apply' {
            $before = Get-MemorySnapshot
            $settings = @(Get-RegistrySettings -SelectedProfile $Profile)

            if ($script:CommandContext.ShouldProcess('les réglages visuels du compte courant', "appliquer le profil $Profile")) {
                $null = Save-RegistryBackup -Settings $settings
                foreach ($setting in $settings) {
                    Set-RegistrySetting -Setting $setting
                }
            }

            if ($CloseOptionalApps) {
                Stop-OptionalApps
                Start-Sleep -Seconds 2
            }

            $after = Get-MemorySnapshot
            Write-Host ''
            Write-Host ("Mémoire utilisée avant : {0} Go ({1} %)" -f $before.UsedGB, $before.UsedPercent)
            Write-Host ("Mémoire utilisée après : {0} Go ({1} %)" -f $after.UsedGB, $after.UsedPercent)
            Write-Host 'Déconnectez-vous puis reconnectez-vous pour appliquer tous les réglages visuels.' -ForegroundColor Yellow
            Write-Host 'Conseil : désactivez manuellement les applications inutiles dans Paramètres > Applications > Démarrage.'
        }

        'Restore' {
            Restore-RegistryBackup
        }
    }

    Write-Log 'Opération terminée.'
}
catch {
    Write-Log $_.Exception.Message 'ERROR'
    throw
}
