<#
.SYNOPSIS
  Builds plan/Musee-Vision-Guide.pdf from plan/guide/guide.html with headless Chrome (or Edge),
  and renders pages to PNG for checking.

.EXAMPLE
  .\build.ps1                      # print guide.html to ..\Musee-Vision-Guide.pdf
  .\build.ps1 -Png                 # ...then render every PDF page to out\p01.png … (1440 × 960)
  .\build.ps1 -Png -Pages 3,36     # ...only pages 3 and 36
  .\build.ps1 -NoPdf -Html -Pages 7   # screenshot guide.html#p07 at 1440 × 960, without printing

.NOTES
  Sheets are 15 × 10 in (1080 × 720 pt): @page { size: 15in 10in; margin: 0 } in guide.css, one
  <section class="page"> each. No headers or footers; backgrounds print (print-color-adjust: exact).
  -Png rasterises the printed PDF with Windows' own PDF renderer (Windows.Data.Pdf), so it shows
  exactly what the PDF holds. -Html screenshots the HTML instead (quicker while editing).
#>
param(
  [switch]$Png,
  [switch]$Html,
  [switch]$NoPdf,
  [int[]]$Pages,
  [int]$Width = 1440,
  [string]$Out = (Join-Path $PSScriptRoot 'out'),
  [string]$Browser
)
$ErrorActionPreference = 'Stop'

$guide = Join-Path $PSScriptRoot 'guide.html'
$pdf   = Join-Path (Split-Path $PSScriptRoot -Parent) 'Musee-Vision-Guide.pdf'
$url   = ([Uri](Resolve-Path $guide).Path).AbsoluteUri

if (-not $Browser) {
  $Browser = @(
    "$env:ProgramFiles\Google\Chrome\Application\chrome.exe",
    "${env:ProgramFiles(x86)}\Google\Chrome\Application\chrome.exe",
    "${env:ProgramFiles(x86)}\Microsoft\Edge\Application\msedge.exe",
    "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
  ) | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $Browser) { throw 'Chrome or Edge not found; pass -Browser <path to chrome.exe>.' }

# A throwaway profile, so a running browser is not disturbed.
$profileDir = Join-Path $env:TEMP 'musee-guide-browser'
$common = @('--headless=new', '--disable-gpu', '--hide-scrollbars', '--no-first-run', '--no-default-browser-check',
            '--disable-extensions', '--run-all-compositor-stages-before-draw', '--virtual-time-budget=15000',
            "--user-data-dir=$profileDir")

function Invoke-Browser([string[]]$arguments) {
  $p = Start-Process -FilePath $Browser -ArgumentList ($common + $arguments) -Wait -PassThru -WindowStyle Hidden
  if ($p.ExitCode -ne 0) { throw "Browser exited with $($p.ExitCode)" }
}

if (-not $NoPdf) {
  if (Test-Path $pdf) { Remove-Item $pdf }
  Invoke-Browser @('--no-pdf-header-footer', '--print-to-pdf-no-header', "--print-to-pdf=`"$pdf`"", $url)
  if (-not (Test-Path $pdf)) { throw 'No PDF was written.' }
  $kb = [math]::Round((Get-Item $pdf).Length / 1KB)
  Write-Host "PDF: $pdf ($kb KB)"
}

if ($Html) {
  New-Item -ItemType Directory -Force $Out | Out-Null
  $count = ([regex]::Matches((Get-Content $guide -Raw -Encoding UTF8), '<section class="page')).Count
  $list = if ($Pages) { $Pages } else { 1..$count }
  foreach ($n in $list) {
    $pngFile = Join-Path $Out ('html-p{0:D2}.png' -f $n)
    Invoke-Browser @("--screenshot=`"$pngFile`"", '--window-size=1440,960', ('{0}#p{1:D2}' -f $url, $n))
    Write-Host "  $pngFile"
  }
}

if ($Png) {
  if (-not (Test-Path $pdf)) { throw "No PDF at $pdf; build it first." }
  New-Item -ItemType Directory -Force $Out | Out-Null
  Add-Type -AssemblyName System.Runtime.WindowsRuntime
  $null = [Windows.Storage.StorageFile, Windows.Storage, ContentType = WindowsRuntime]
  $null = [Windows.Data.Pdf.PdfDocument, Windows.Data.Pdf, ContentType = WindowsRuntime]
  $null = [Windows.Storage.Streams.InMemoryRandomAccessStream, Windows.Storage.Streams, ContentType = WindowsRuntime]
  $ext = [System.WindowsRuntimeSystemExtensions].GetMethods()
  $asTask = ($ext | Where-Object { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncOperation`1' })[0]
  $asTaskAction = ($ext | Where-Object { $_.Name -eq 'AsTask' -and $_.GetParameters().Count -eq 1 -and $_.GetParameters()[0].ParameterType.Name -eq 'IAsyncAction' })[0]
  function Await($op, [Type]$type) { $t = $asTask.MakeGenericMethod($type).Invoke($null, @($op)); $t.Wait() | Out-Null; $t.Result }
  function AwaitAction($op) { $t = $asTaskAction.Invoke($null, @($op)); $t.Wait() | Out-Null }

  $file = Await ([Windows.Storage.StorageFile]::GetFileFromPathAsync((Resolve-Path $pdf).Path)) ([Windows.Storage.StorageFile])
  $doc  = Await ([Windows.Data.Pdf.PdfDocument]::LoadFromFileAsync($file)) ([Windows.Data.Pdf.PdfDocument])
  Write-Host "Pages in the PDF: $($doc.PageCount)"
  $list = if ($Pages) { $Pages } else { 1..$doc.PageCount }
  foreach ($n in $list) {
    $page = $doc.GetPage($n - 1)
    $opt = New-Object Windows.Data.Pdf.PdfPageRenderOptions
    $opt.DestinationWidth  = $Width
    $opt.DestinationHeight = [uint32][math]::Round($Width * $page.Size.Height / $page.Size.Width)
    $stream = New-Object Windows.Storage.Streams.InMemoryRandomAccessStream
    AwaitAction ($page.RenderToStreamAsync($stream, $opt))
    $in = [System.IO.WindowsRuntimeStreamExtensions]::AsStreamForRead($stream.GetInputStreamAt(0))
    $pngFile = Join-Path $Out ('p{0:D2}.png' -f $n)
    $fs = [System.IO.File]::Create($pngFile); $in.CopyTo($fs); $fs.Close(); $in.Close(); $page.Dispose()
    Write-Host "  $pngFile"
  }
}
