$kitColors = @{}
$kitColors["Man Utd"]         = @("Red","White","White","Black")
$kitColors["Liverpool"]       = @("Red","Red","White","White")
$kitColors["Arsenal"]         = @("Red","White","Yellow","Navy")
$kitColors["Chelsea"]         = @("Blue","Blue","White","White")
$kitColors["Tottenham"]       = @("White","Navy","Navy","Navy")
$kitColors["Man City"]        = @("SkyBlue","White","Navy","Navy")
$kitColors["Leeds Utd"]       = @("White","White","Yellow","Yellow")
$kitColors["Everton"]         = @("Blue","White","White","White")
$kitColors["Aston Villa"]     = @("Maroon","Blue","White","White")
$kitColors["Newcastle"]       = @("Black","Black","White","White")
$kitColors["Blackburn"]       = @("Blue","White","White","White")
$kitColors["Sheffield Wed"]   = @("Blue","White","White","White")
$kitColors["Wimbledon"]       = @("Blue","Blue","Yellow","Yellow")
$kitColors["Leicester"]       = @("Blue","White","White","Blue")
$kitColors["West Ham"]        = @("Maroon","Blue","White","White")
$kitColors["Derby"]           = @("White","Black","Black","Black")
$kitColors["Coventry"]        = @("SkyBlue","SkyBlue","White","White")
$kitColors["Southampton"]     = @("Red","Black","Black","Black")
$kitColors["Bolton"]          = @("White","Navy","Navy","Navy")
$kitColors["Barnsley"]        = @("Red","Red","White","White")
$kitColors["Real Madrid"]     = @("White","White","Purple","Purple")
$kitColors["Barcelona"]       = @("Red","Blue","Yellow","Yellow")
$kitColors["Atletico Madrid"] = @("Red","White","White","White")
$kitColors["Valencia"]        = @("White","White","Black","Black")
$kitColors["Deportivo"]       = @("White","White","Blue","Blue")
$kitColors["Sevilla"]         = @("White","White","Red","Red")
$kitColors["Real Betis"]      = @("Green","White","White","White")
$kitColors["Athletic Bilbao"] = @("Red","White","White","White")
$kitColors["Real Sociedad"]   = @("Blue","White","White","White")
$kitColors["Villarreal"]      = @("Yellow","Blue","Blue","Blue")
$kitColors["Bayern Munich"]   = @("Red","Red","White","White")
$kitColors["Borussia Dortmund"] = @("Yellow","Black","Black","Black")
$kitColors["Schalke"]         = @("Blue","White","White","White")
$kitColors["Bayer Leverkusen"] = @("Red","Black","Black","Black")
$kitColors["Werder Bremen"]   = @("Green","White","White","White")
$kitColors["Hamburg"]         = @("White","White","Blue","Blue")
$kitColors["VfB Stuttgart"]   = @("White","Red","Black","Black")
$kitColors["Kaiserslautern"]  = @("Red","White","White","White")
$kitColors["Juventus"]        = @("Black","White","White","White")
$kitColors["AC Milan"]        = @("Red","Black","White","White")
$kitColors["Inter Milan"]     = @("Blue","Black","White","White")
$kitColors["Lazio"]           = @("SkyBlue","White","White","White")
$kitColors["Roma"]            = @("Red","Yellow","Yellow","Yellow")
$kitColors["Fiorentina"]      = @("Purple","White","White","White")
$kitColors["Parma"]           = @("Yellow","Blue","Blue","Blue")
$kitColors["Napoli"]          = @("Blue","Blue","White","White")
$kitColors["Sampdoria"]       = @("Blue","Blue","White","White")
$kitColors["Udinese"]         = @("Black","White","White","White")
$kitColors["Atalanta"]        = @("Black","Blue","White","White")
$kitColors["PSG"]             = @("Navy","Red","White","White")
$kitColors["Monaco"]          = @("Red","White","White","White")
$kitColors["Lyon"]            = @("White","White","Blue","Blue")
$kitColors["Marseille"]       = @("White","White","Blue","Blue")
$kitColors["Bordeaux"]        = @("Navy","Navy","White","White")
$kitColors["Lens"]            = @("Red","Yellow","Yellow","Yellow")
$kitColors["Nantes"]          = @("Yellow","Yellow","White","White")
$kitColors["Ajax"]            = @("White","Red","Black","Black")
$kitColors["PSV"]             = @("Red","White","White","White")
$kitColors["Feyenoord"]       = @("Red","White","White","White")
$kitColors["Porto"]           = @("Blue","Blue","White","White")
$kitColors["Benfica"]         = @("Red","Red","White","White")
$kitColors["Sporting"]        = @("Green","White","White","White")
$kitColors["Celtic"]          = @("Green","White","Black","Black")
$kitColors["Rangers"]         = @("Blue","Blue","White","White")
$kitColors["Hearts"]          = @("Maroon","White","White","White")
$kitColors["Aberdeen"]        = @("Red","Red","White","White")
$kitColors["FC Copenhagen"]   = @("White","Blue","Blue","Blue")
$kitColors["FC Kobenhavn"]    = @("White","Blue","Blue","Blue")
$kitColors["Brondby"]         = @("Yellow","Blue","Blue","Blue")
$kitColors["AaB"]             = @("Red","Red","White","White")
$kitColors["Galatasaray"]     = @("Red","Yellow","Yellow","Yellow")
$kitColors["Fenerbahce"]      = @("Yellow","Blue","Blue","Blue")
$kitColors["Besiktas"]        = @("Black","White","White","Black")
$kitColors["Anderlecht"]      = @("Purple","White","White","White")
$kitColors["Olympiakos"]      = @("Red","White","White","White")
$kitColors["Panathinaikos"]   = @("Green","Green","White","White")
$kitColors["Dinamo Kiev"]     = @("White","White","Blue","Blue")
$kitColors["Shakhtar"]        = @("Orange","Black","White","White")

$lines = Get-Content "D:\DEV\Nostalgia\NostalgiaManager\data\ClubsDB.csv"
$header = $lines[0]
$cols = $header -split ';'
$idxHome1 = [Array]::IndexOf($cols,'HomeShirtColour')
$idxHome2 = [Array]::IndexOf($cols,'HomeShortsColour')
$idxAway1 = [Array]::IndexOf($cols,'AwayShirtColour')
$idxAway2 = [Array]::IndexOf($cols,'AwayShortsColour')

$newLines = New-Object System.Collections.Generic.List[string]
$newLines.Add($header)

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
    }
    $newLines.Add(($r -join ';'))
}

$newLines | Set-Content "D:\DEV\Nostalgia\NostalgiaManager\data\ClubsDB.csv"
Write-Host "Done."
