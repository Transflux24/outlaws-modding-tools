param(
    [Parameter(Mandatory = $true)]
    [string] $Trace
)

$lines = Get-Content -LiteralPath $Trace
$sessionStarts = for ($index = 0; $index -lt $lines.Count; $index++) {
    if ($lines[$index].StartsWith('# session ')) { $index }
}
if ($null -eq $sessionStarts -or @($sessionStarts).Count -eq 0) {
    throw 'No observer consumer sessions found.'
}

$start = $sessionStarts[-1]
$headerIndex = $start + 1
if ($headerIndex -ge $lines.Count) { throw 'Newest session has no header.' }
$header = $lines[$headerIndex]
$dataLines = @()
if ($headerIndex + 1 -lt $lines.Count) {
    $dataLines = @(
        $lines[($headerIndex + 1)..($lines.Count - 1)] |
            Where-Object { $_ -and -not $_.StartsWith('#') -and -not $_.StartsWith("sequence`t") }
    )
}

Write-Output $lines[$start]
if (-not $dataLines) {
    Write-Output 'No filtered consumer reads were recorded in the newest session.'
    exit 0
}

$rows = @($dataLines | ConvertFrom-Csv -Delimiter "`t" -Header ($header -split "`t"))
$meaning = @{
    '0x28' = 'Has Dealt Damage'
    '0x38' = 'Fatal'
    '0x40' = 'Efficiency'
    '0x58' = 'Damage Type'
    '0x68' = 'Used Weapon'
    '0x70' = 'Projectile Range Efficiency'
    '0x80' = 'Weak-point query input'
    '0x88' = 'Damage Feedback Body Part'
    '0x90' = 'Weakpoint Destroyed'
}

Write-Output ("records={0} pulses={1} callers={2}" -f $rows.Count,
    @($rows.pulse | Sort-Object -Unique).Count,
    @($rows.caller_rva | Sort-Object -Unique).Count)

foreach ($pulseGroup in ($rows | Group-Object pulse | Sort-Object { [uint64]$_.Name })) {
    Write-Output ''
    Write-Output ("PULSE {0}" -f $pulseGroup.Name)
    foreach ($row in ($pulseGroup.Group | Sort-Object slot_offset, caller_rva)) {
        $label = $meaning[$row.slot_offset]
        if (-not $label) { $label = 'Unmapped output' }
        Write-Output ("  {0,-4} {1,-29} {2,-7} value={3,-18} caller={4} stack={5},{6},{7}" -f
            $row.slot_offset, $label, $row.storage_kind, $row.value_bits,
            $row.caller_rva, $row.stack0_rva, $row.stack1_rva, $row.stack2_rva)
    }
}

Write-Output ''
Write-Output 'CALLER SUMMARY'
foreach ($callerGroup in ($rows | Group-Object caller_rva | Sort-Object Count -Descending)) {
    $slots = @($callerGroup.Group.slot_offset | Sort-Object -Unique) -join ','
    Write-Output ("  {0,-12} reads={1,-4} slots={2}" -f $callerGroup.Name, $callerGroup.Count, $slots)
}
