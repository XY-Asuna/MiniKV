rm .\MiniKV.exe

.\build.ps1

"-----------------------------------------------------------------`n"
$env:bucketsize = 17
"bucketsize = $env:bucketsize"
.\MiniKV.exe --perf --turn 0 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 1 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 2 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 3 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 4 --buckets $env:bucketsize
"-----------------------------------------------------------------`n"
<#
$env:bucketsize = 541
"bucketsize = $env:bucketsize"
.\MiniKV.exe --perf --turn 1 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 2 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 3 --buckets $env:bucketsize
"-----------------------------------------------------------------`n"
$env:bucketsize = 1097
"bucketsize = $env:bucketsize"
.\MiniKV.exe --perf --turn 1 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 2 --buckets $env:bucketsize
.\MiniKV.exe --perf --turn 3 --buckets $env:bucketsize
"-----------------------------------------------------------------`n"
#>