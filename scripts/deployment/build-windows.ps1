<#
.SYNOPSIS
Build the Windows CPU/CUDA executables and OpenGL viewer.

.DESCRIPTION
Selects an installed MSVC toolset and Windows SDK, sets up the environment
Ninja needs, then configures and builds the project. For CUDA, the installed
toolkit declares the supported MSVC range in host_config.h. This script selects
a compiler inside that range and reports a clear error when none is installed.

.PARAMETER Cuda
Enable the CUDA backend.

.PARAMETER Viewer
Build the OpenGL viewer.

.PARAMETER Mmo
Enable the optional 3D MMO and its client/server targets.

.PARAMETER ReferenceDouble
Use the CPU-only float64 reference implementation. Incompatible with -Cuda.

.PARAMETER Architecture
CUDA architecture number to compile for, such as 89 for the packaged Ada
build. Ignored when -Cuda is not set.

.PARAMETER BuildDirectory
Override the default project-local tmp/build/windows directory.

.PARAMETER Jobs
Number of parallel build jobs. Defaults to 6.

.EXAMPLE
.\scripts\deployment\build-windows.ps1 -Cuda -Viewer -Architecture 89
.EXAMPLE
.\scripts\deployment\build-windows.ps1 -ReferenceDouble
#>

param(
    [switch]$Cuda,
    [switch]$Viewer,
    [switch]$Mmo,
    [switch]$ReferenceDouble,
    [string]$Architecture = '89',
    [string]$BuildDirectory = '',
    [int]$Jobs = 6
)

$ErrorActionPreference = 'Stop'

# Two directory levels up from scripts/deployment/ is the project root.
$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path

# Default the build directory to a project-local tmp/ so multiple build
# flavors can coexist without clobbering each other.
if (!$BuildDirectory) {
    $BuildDirectory = Join-Path $projectRoot 'tmp/build/windows'
}

# The float64 reference build has no CUDA path; rejecting the combination
# here avoids a long toolset search followed by a late error.
if ($Cuda -and $ReferenceDouble) {
    throw 'Float64 reference is CPU-only; omit -Cuda.'
}


<#
.SYNOPSIS
Discovers the MSVC toolchain, Windows SDK, and optionally the CUDA toolkit.

.DESCRIPTION
Searches for Visual Studio installations using vswhere, falls back to standard
installation paths, and verifies the existence of the x64 host compiler.
If CUDA is requested, it parses the toolkit's host_config.h to determine the
supported MSVC compiler version range and filters the discovered toolsets
accordingly. Selects the newest compatible toolset.
#>
function Get-BuildToolchain {
    param([bool]$UseCuda)

    $nvcc, $toolkitRoot = $null, $null
    $installRoots = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\18\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\18\Professional",
        "${env:ProgramFiles}\Microsoft Visual Studio\18\Enterprise",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools"
    )

    # Installer discovery also covers Build Tools and custom installation directories.
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $installRoots = @(& $vswhere -all -products '*' -property installationPath) + $installRoots
    }
    $installRoots = @($installRoots | Select-Object -Unique)

    $compilerCandidates = @()
    foreach ($install in $installRoots) {
        $candidateDir = Join-Path $install 'VC/Tools/MSVC'
        if (Test-Path -LiteralPath $candidateDir) {
            $compilerCandidates += Get-ChildItem -LiteralPath $candidateDir -Directory
        }
    }

    # A directory under VC/Tools/MSVC is a toolset only if it actually contains
    # the x64 host compiler. The directory alone is not enough evidence.
    $compilerCandidates = @(
        $compilerCandidates | Where-Object {
            Test-Path -LiteralPath (
                Join-Path $_.FullName 'bin/Hostx64/x64/cl.exe'
            )
        }
    )

    if ($UseCuda) {
        $nvcc = (Get-Command nvcc -ErrorAction Stop).Source
        $toolkitRoot = Split-Path (Split-Path $nvcc -Parent) -Parent
        $hostConfiguration = Get-Content -LiteralPath (Join-Path $toolkitRoot 'include/crt/host_config.h') -Raw
        $supportedMsvc = [regex]::Match($hostConfiguration, '_MSC_VER\s*<\s*(\d+)\s*\|\|\s*_MSC_VER\s*>=\s*(\d+)')
        
        if (!$supportedMsvc.Success) {
            throw 'Cannot determine the installed CUDA toolkit host-compiler limits.'
        }
        
        $minimumMsvc = [int]$supportedMsvc.Groups[1].Value
        $maximumMsvc = [int]$supportedMsvc.Groups[2].Value

        # Toolset 14.36 corresponds to compiler 19.36 (_MSC_VER 1936), and so on.
        $compilerCandidates = @(
            $compilerCandidates | Where-Object {
                $compilerVersion = 1900 + ([version]$_.Name).Minor
                $compilerVersion -ge $minimumMsvc -and $compilerVersion -lt $maximumMsvc
            }
        )
    }

    # Newest matching toolset wins. Sort-Object on the version directory name
    # works because MSVC toolset names are zero-padded (14.29 < 14.36 < 14.40).
    $compiler = $compilerCandidates |
        Sort-Object Name -Descending |
        Select-Object -First 1

    if (!$compiler) {
        throw 'No installed MSVC toolset satisfies the selected CUDA toolkit host-compiler limits.'
    }
    
    Write-Host "MSVC toolset: $($compiler.Name)"
    
    return [PSCustomObject]@{
        Compiler = $compiler
        InstallRoots = $installRoots
        Nvcc = $nvcc
        ToolkitRoot = $toolkitRoot
    }
}


<#
.SYNOPSIS
Configures the process-level environment variables for the build.

.DESCRIPTION
Locates the Windows SDK, shortens the compiler path to avoid spaces (which 
can break CUDA toolchain arguments), and prepends the discovered compiler, 
SDK, and Ninja paths to the PATH, INCLUDE, and LIB environment variables.
Explicitly removes any stale CUDAHOSTCXX variable to prevent it from 
overriding the selected compiler.
#>
function Set-BuildEnvironment {
    param($Toolchain)

    $compiler = $Toolchain.Compiler
    $installRoots = $Toolchain.InstallRoots
    $sdkRoot = "${env:ProgramFiles(x86)}\Windows Kits\10"
    $sdkVersion = (
        Get-ChildItem -LiteralPath (Join-Path $sdkRoot 'Lib') -Directory |
            Sort-Object Name -Descending |
            Select-Object -First 1
    ).Name
    
    Write-Host "Windows SDK: $sdkVersion"

    # Shorten the compiler path before passing it to CUDA.
    #
    # CUDA's build rules pass the toolchain path through command lines that are
    # not always quoted, so a path containing spaces ("Program Files") can be
    # split into two arguments. The Windows 8.3 short form sidesteps this by
    # returning a space-free path like C:\PROGRA~1\...
    $fileSystem = New-Object -ComObject Scripting.FileSystemObject
    $compilerRoot = $fileSystem.GetFolder($compiler.FullName).ShortPath

    # Locate Ninja.
    #
    # Ninja ships inside every Visual Studio install under Common7/IDE. Each
    # candidate path is collected first and joined into PATH as one entry, so a
    # per-install check is not needed.
    $ninjaPaths = @()
    foreach ($install in $installRoots) {
        $ninjaPaths += Join-Path $install 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja'
    }

    # Configure the environment for child processes only.
    #
    # The selected tools and child build processes use these environment variables.
    # Windows separates PATH, INCLUDE and LIB entries with semicolons.
    $env:PATH = @(
        "$compilerRoot\bin\Hostx64\x64"
        "$sdkRoot\bin\$sdkVersion\x64"
        $ninjaPaths
        $env:PATH
    ) -join ';'

    $env:INCLUDE = @(
        "$compilerRoot\include"
        "$sdkRoot\Include\$sdkVersion\ucrt"
        "$sdkRoot\Include\$sdkVersion\shared"
        "$sdkRoot\Include\$sdkVersion\um"
        "$sdkRoot\Include\$sdkVersion\winrt"
    ) -join ';'

    $env:LIB = @(
        "$compilerRoot\lib\x64"
        "$sdkRoot\Lib\$sdkVersion\ucrt\x64"
        "$sdkRoot\Lib\$sdkVersion\um\x64"
    ) -join ';'

    # CUDA 12.x sets CUDAHOSTCXX to its bundled cl.exe when it runs, and that
    # bundled compiler is often an older toolset than the one we selected above.
    # Removing the variable here stops a stale value in the parent shell from
    # overriding our choice.
    Remove-Item Env:CUDAHOSTCXX -ErrorAction SilentlyContinue
}


<#
.SYNOPSIS
Copies required Visual C++ and CUDA runtime DLLs into the export directory.

.DESCRIPTION
Enables app-local deployment by gathering the appropriate VC++ Redistributable 
DLLs and CUDA runtime libraries, allowing the built executables to run on a 
clean Windows system without requiring a developer shell or global installations.
#>
function Copy-RuntimeLibraries {
    param($Toolchain, [string]$OutputDirectory, [bool]$UseCuda)

    $compiler = $Toolchain.Compiler
    $toolkitRoot = $Toolchain.ToolkitRoot
    $vcRoot = Split-Path (Split-Path (Split-Path $compiler.FullName -Parent) -Parent) -Parent
    
    $redist = Get-ChildItem -LiteralPath (Join-Path $vcRoot 'Redist/MSVC') -Directory |
        Where-Object { $_.Name -match '^\d+\.' } |
        Sort-Object { [version]$_.Name } -Descending |
        Select-Object -First 1
        
    if ($redist) {
        Get-ChildItem -Path (Join-Path $redist.FullName 'x64/Microsoft.VC*.CRT/*.dll') -File |
            Copy-Item -Destination $OutputDirectory -Force
    }
    
    if ($UseCuda) {
        foreach ($directory in @('bin', 'bin/x64')) {
            Get-ChildItem -Path (Join-Path $toolkitRoot "$directory/cudart64*.dll") -File -ErrorAction SilentlyContinue |
                Copy-Item -Destination $OutputDirectory -Force
        }
    }
}


# 1. Select tools, then set search paths only for this build session.
$toolchain = Get-BuildToolchain -UseCuda $Cuda.IsPresent
Set-BuildEnvironment -Toolchain $toolchain


# 2. Configure all options explicitly, so old CMake cache choices cannot override this run.
$cudaOption      = if ($Cuda)            { 'ON' } else { 'OFF' }
$viewerOption    = if ($Viewer)          { 'ON' } else { 'OFF' }
$mmoOption       = if ($Mmo)             { 'ON' } else { 'OFF' }
$referenceOption = if ($ReferenceDouble) { 'ON' } else { 'OFF' }

$cudaCompilerOptions = @()
if ($Cuda) {
    $cudaCompilerOptions = @("-DCMAKE_CUDA_COMPILER=$($toolchain.Nvcc)", "-DCUDAToolkit_ROOT=$($toolchain.ToolkitRoot)")
}

Write-Host (
    "Configuring in $BuildDirectory " +
    "(CUDA=$cudaOption Viewer=$viewerOption MMO=$mmoOption ReferenceDouble=$referenceOption)"
)

& cmake -S $projectRoot -B $BuildDirectory -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_TRY_COMPILE_CONFIGURATION=Release `
    "-DENABLE_CUDA=$cudaOption" `
    "-DBUILD_VIEWER=$viewerOption" `
    "-DBUILD_MMO=$mmoOption" `
    "-DREFERENCE_DOUBLE=$referenceOption" `
    "-DCMAKE_CUDA_ARCHITECTURES=$Architecture" `
    -DCMAKE_CUDA_FLAGS=--use-local-env @cudaCompilerOptions

if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}


# 3. Build, then copy redistributable DLLs only after a successful build.
Write-Host "Building with $Jobs parallel jobs"
# GLAD 0.1 opens its bundled UTF-8 registry in Python's default text encoding.
& cmake -E env PYTHONUTF8=1 cmake --build $BuildDirectory --parallel $Jobs
$buildStatus = $LASTEXITCODE

if ($buildStatus -eq 0) {
    $outputDirectory = Join-Path $projectRoot 'exports/builds/windows-x64'
    Copy-RuntimeLibraries -Toolchain $toolchain -OutputDirectory $outputDirectory -UseCuda $Cuda.IsPresent
}

exit $buildStatus
