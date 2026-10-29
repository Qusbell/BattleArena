param([Parameter(Mandatory = $true)][string]$Project)
$ErrorActionPreference = 'Stop'
try {
    $projectDirectory = Split-Path -Parent $Project
    $association = (Get-Content -LiteralPath $Project -Raw | ConvertFrom-Json).EngineAssociation
    $candidates = @()
    if ($env:SHOOTINGARENA_ENGINE_ROOT) {
        $override = $env:SHOOTINGARENA_ENGINE_ROOT
        if (-not [IO.Path]::IsPathRooted($override)) {
            $override = Join-Path $projectDirectory $override
        }
        $candidates = @($override)
    } else {
        # Source builds registered by UnrealVersionSelector use per-user GUIDs.
        $userBuilds = Get-ItemProperty 'HKCU:\Software\Epic Games\Unreal Engine\Builds' -ErrorAction SilentlyContinue
        if ($userBuilds -and $association) {
            $property = $userBuilds.PSObject.Properties[$association]
            if ($property) { $candidates += $property.Value }
        }
        # Launcher installs can live on any drive.
        if ($association) {
            foreach ($registryRoot in @('HKLM:\SOFTWARE\EpicGames\Unreal Engine', 'HKLM:\SOFTWARE\WOW6432Node\EpicGames\Unreal Engine')) {
                $installation = Get-ItemProperty "$registryRoot\$association" -ErrorAction SilentlyContinue
                if ($installation -and $installation.InstalledDirectory) {
                    $candidates += $installation.InstalledDirectory
                }
            }
        }
        if ($env:ProgramData -and $association) {
            $manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
            if (Test-Path -LiteralPath $manifest) {
                $installs = (Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json).InstallationList
                $candidates += @($installs | Where-Object { $_.AppName -eq "UE_$association" } | ForEach-Object { $_.InstallLocation })
            }
        }
        if ($association -and $env:ProgramFiles) {
            $candidates += Join-Path $env:ProgramFiles "Epic Games\UE_$association"
        }
        # Projects inside a source engine tree may have no EngineAssociation.
        if (-not $association) {
            $directory = Get-Item -LiteralPath $projectDirectory
            while ($directory) {
                $candidates += $directory.FullName
                $directory = $directory.Parent
            }
        }
    }
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'Engine\Build\BatchFiles\Build.bat'))) {
            [Console]::WriteLine([IO.Path]::GetFullPath($candidate))
            exit 0
        }
    }
    throw "Cannot find Unreal Engine '$association'. Install/register that engine, or set SHOOTINGARENA_ENGINE_ROOT to its root (relative paths are resolved beside the project)."
} catch {
    [Console]::Error.WriteLine('[ERROR] ' + $_.Exception.Message)
    exit 1
}
