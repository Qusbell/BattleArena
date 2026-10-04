param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectPath
)

$ErrorActionPreference = 'Stop'

function Test-UnrealEngineRoot([string]$Path) {
    return $Path -and (Test-Path -LiteralPath (Join-Path $Path 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'))
}

try {
    $project = Get-Content -LiteralPath $ProjectPath -Raw | ConvertFrom-Json
    $association = [string]$project.EngineAssociation
    $engineRoot = $null

    if ($association) {
        # Custom/source builds registered by UnrealVersionSelector.
        $registryPaths = @(
            'Registry::HKEY_CURRENT_USER\Software\Epic Games\Unreal Engine\Builds',
            'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\Epic Games\Unreal Engine\Builds',
            'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\Epic Games\Unreal Engine\Builds'
        )

        foreach ($registryPath in $registryPaths) {
            if (Test-Path -LiteralPath $registryPath) {
                $key = Get-Item -LiteralPath $registryPath
                $registeredPath = $key.GetValue($association, $null)
                if (Test-UnrealEngineRoot $registeredPath) {
                    $engineRoot = $registeredPath
                    break
                }
            }
        }

        # Launcher builds are listed in LauncherInstalled.dat. Their AppName
        # is UE_<version>, while EngineAssociation stores only <version>.
        if (-not $engineRoot) {
            $manifest = Join-Path $env:ProgramData 'Epic\UnrealEngineLauncher\LauncherInstalled.dat'
            if (Test-Path -LiteralPath $manifest) {
                $installations = (Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json).InstallationList
                $installation = $installations |
                    Where-Object { $_.AppName -eq "UE_$association" -or $_.AppName -eq $association } |
                    Select-Object -First 1
                if ($installation -and (Test-UnrealEngineRoot $installation.InstallLocation)) {
                    $engineRoot = $installation.InstallLocation
                }
            }
        }
    }
    else {
        # Native/source projects can use an Engine folder above the project.
        $directory = Get-Item -LiteralPath (Split-Path -Parent (Resolve-Path -LiteralPath $ProjectPath))
        while ($directory) {
            if (Test-UnrealEngineRoot $directory.FullName) {
                $engineRoot = $directory.FullName
                break
            }
            $directory = $directory.Parent
        }
    }

    if (-not (Test-UnrealEngineRoot $engineRoot)) {
        [Console]::Error.WriteLine("No installed Unreal Engine matches EngineAssociation '$association'.")
        exit 1
    }

    [Console]::Out.WriteLine((Resolve-Path -LiteralPath $engineRoot).Path)
    exit 0
}
catch {
    [Console]::Error.WriteLine("Engine detection failed: $($_.Exception.Message)")
    exit 1
}
