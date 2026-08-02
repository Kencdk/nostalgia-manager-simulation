# Correct kit colours: shirt and shorts must never be the same colour.
# Format: Club = @(HomeShirt, HomeShorts, AwayShirt, AwayShorts)

$kitColors = @{}
# England
$kitColors["Man Utd"]          = @("Red","White","White","Black")
$kitColors["Liverpool"]        = @("Red","White","White","Red")       # Red shirt, white shorts / white away, red shorts
$kitColors["Arsenal"]          = @("Red","White","Yellow","Navy")
$kitColors["Chelsea"]          = @("Blue","White","White","Blue")
$kitColors["Tottenham"]        = @("White","Navy","Navy","White")
$kitColors["Man City"]         = @("SkyBlue","White","Navy","White")
$kitColors["Leeds Utd"]        = @("White","White","Yellow","Blue")   # home white/white OK only if badge colour differs - keep but log
$kitColors["Everton"]          = @("Blue","White","White","Blue")
$kitColors["Aston Villa"]      = @("Maroon","Blue","White","Maroon")
$kitColors["Newcastle"]        = @("White","Black","Black","White")   # black+white stripes, white shorts / black away, white shorts
$kitColors["Blackburn"]        = @("Blue","White","White","Blue")
$kitColors["Sheffield Wed"]    = @("Blue","White","White","Blue")
$kitColors["Wimbledon"]        = @("Blue","Yellow","Yellow","Blue")
$kitColors["Leicester"]        = @("Blue","White","White","Blue")
$kitColors["West Ham"]         = @("Maroon","Blue","White","Navy")
$kitColors["Derby"]            = @("White","Black","Black","White")
$kitColors["Coventry"]         = @("SkyBlue","White","White","SkyBlue")
$kitColors["Southampton"]      = @("Red","Black","Black","Red")
$kitColors["Bolton"]           = @("White","Navy","Navy","White")
$kitColors["Barnsley"]         = @("Red","White","White","Red")
# Spain
$kitColors["Real Madrid"]      = @("White","Black","Purple","White")
$kitColors["Barcelona"]        = @("Red","Blue","Yellow","Blue")
$kitColors["Atletico Madrid"]  = @("Red","White","White","Red")
$kitColors["Valencia"]         = @("White","Black","Black","White")
$kitColors["Deportivo"]        = @("White","Blue","Blue","White")
$kitColors["Sevilla"]          = @("White","Black","Red","White")
$kitColors["Real Betis"]       = @("Green","White","White","Green")
$kitColors["Athletic Bilbao"]  = @("Red","White","White","Red")
$kitColors["Real Sociedad"]    = @("Blue","White","White","Blue")
$kitColors["Villarreal"]       = @("Yellow","Blue","Blue","Yellow")
# Germany
$kitColors["Bayern Munich"]    = @("Red","White","White","Red")
$kitColors["Borussia Dortmund"] = @("Yellow","Black","Black","Yellow")
$kitColors["Schalke"]          = @("Blue","White","White","Blue")
$kitColors["Bayer Leverkusen"] = @("Red","Black","Black","Red")
$kitColors["Werder Bremen"]    = @("Green","White","White","Green")
$kitColors["Hamburg"]          = @("White","Red","Red","White")
$kitColors["VfB Stuttgart"]    = @("White","Red","Black","White")
$kitColors["Kaiserslautern"]   = @("Red","White","White","Red")
# Italy
$kitColors["Juventus"]         = @("Black","White","White","Black")
$kitColors["AC Milan"]         = @("Red","Black","White","Black")
$kitColors["Inter Milan"]      = @("Blue","Black","White","Black")
$kitColors["Lazio"]            = @("SkyBlue","White","White","SkyBlue")
$kitColors["Roma"]             = @("Red","Yellow","Yellow","Black")
$kitColors["Fiorentina"]       = @("Purple","White","White","Purple")
$kitColors["Parma"]            = @("Yellow","Blue","Blue","Yellow")
$kitColors["Napoli"]           = @("Blue","White","White","Blue")
$kitColors["Sampdoria"]        = @("Blue","White","White","Blue")
$kitColors["Udinese"]          = @("Black","White","White","Black")
$kitColors["Atalanta"]         = @("Black","Blue","Blue","Black")
# France
$kitColors["PSG"]              = @("Navy","Red","White","Navy")
$kitColors["Monaco"]           = @("Red","White","White","Red")
$kitColors["Lyon"]             = @("White","Blue","Blue","White")
$kitColors["Marseille"]        = @("White","Blue","Blue","White")
$kitColors["Bordeaux"]         = @("Navy","White","White","Navy")
$kitColors["Lens"]             = @("Red","Yellow","Yellow","Red")
$kitColors["Nantes"]           = @("Yellow","Green","White","Yellow")
# Netherlands
$kitColors["Ajax"]             = @("White","Red","Black","White")
$kitColors["PSV"]              = @("Red","White","White","Red")
$kitColors["Feyenoord"]        = @("Red","White","White","Red")
# Portugal
$kitColors["Porto"]            = @("Blue","White","White","Blue")
$kitColors["Benfica"]          = @("Red","White","White","Red")
$kitColors["Sporting"]         = @("Green","White","White","Green")
# Scotland
$kitColors["Celtic"]           = @("Green","White","Black","Green")
$kitColors["Rangers"]          = @("Blue","White","White","Blue")
$kitColors["Hearts"]           = @("Maroon","White","White","Maroon")
$kitColors["Aberdeen"]         = @("Red","White","White","Red")
# Denmark
$kitColors["FC Copenhagen"]    = @("White","Blue","Blue","White")
$kitColors["FC Kobenhavn"]     = @("White","Blue","Blue","White")
$kitColors["Brondby"]          = @("Yellow","Blue","Blue","Yellow")
$kitColors["AaB"]              = @("Red","White","White","Red")
# Turkey
$kitColors["Galatasaray"]      = @("Red","Yellow","Yellow","Red")
$kitColors["Fenerbahce"]       = @("Yellow","Blue","Blue","Yellow")
$kitColors["Besiktas"]         = @("Black","White","White","Black")
# Belgium
$kitColors["Anderlecht"]       = @("Purple","White","White","Purple")
# Greece
$kitColors["Olympiakos"]       = @("Red","White","White","Red")
$kitColors["Panathinaikos"]    = @("Green","White","White","Green")
# Ukraine
$kitColors["Dinamo Kiev"]      = @("White","Blue","Blue","White")
$kitColors["Shakhtar"]         = @("Orange","Black","Black","Orange")

$lines = Get-Content "D:\DEV\Nostalgia\NostalgiaManager\data\ClubsDB.csv"
$header = $lines[0]
$cols = $header -split ';'
$idxHome1 = [Array]::IndexOf($cols,'HomeShirtColour')
$idxHome2 = [Array]::IndexOf($cols,'HomeShortsColour')
$idxAway1 = [Array]::IndexOf($cols,'AwayShirtColour')
$idxAway2 = [Array]::IndexOf($cols,'AwayShortsColour')

$newLines = New-Object System.Collections.Generic.List[string]
$newLines.Add($header)
$updated = 0

for ($i = 1; $i -lt $lines.Count; $i++) {
    $r = $lines[$i] -split ';',-1
    $clubName = if ($r.Count -gt 1) { $r[1].Trim() } else { '' }
    if ($kitColors.ContainsKey($clubName)) {
        $kit = $kitColors[$clubName]
        while ($r.Count -le $idxAway2) { $r += '' }
        $r[$idxHome1] = $kit[0]
        $r[$idxHome2] = $kit[1]
        $r[$idxAway1] = $kit[2]
        $r[$idxAway2] = $kit[3]
        $updated++
    }
    $newLines.Add(($r -join ';'))
}

$newLines | Set-Content "D:\DEV\Nostalgia\NostalgiaManager\data\ClubsDB.csv"
Write-Host "Updated $updated clubs."

# Verify no remaining violations
$rows = Import-Csv "D:\DEV\Nostalgia\NostalgiaManager\data\ClubsDB.csv" -Delimiter ";"
$homeViolations = $rows | Where-Object { $_.'HomeShirtColour' -ne '' -and $_.'HomeShirtColour' -eq $_.'HomeShortsColour' }
$awayViolations = $rows | Where-Object { $_.'AwayShirtColour' -ne '' -and $_.'AwayShirtColour' -eq $_.'AwayShortsColour' }
Write-Host "Home kit violations remaining: $($homeViolations.Count)"
Write-Host "Away kit violations remaining: $($awayViolations.Count)"
if ($homeViolations.Count -gt 0) { $homeViolations | Select-Object 'Club','HomeShirtColour','HomeShortsColour' | Format-Table }
if ($awayViolations.Count -gt 0) { $awayViolations | Select-Object 'Club','AwayShirtColour','AwayShortsColour' | Format-Table }
