// PEVirus Detection Rules
// Author: Malware Detector Lab
// Purpose: Detect PEVirus infection patterns

rule PEVirus_Hacked_Section
{
  meta:
    author = "lab"
    description = "Detects PEVirus infection by .hacked section name"
    severity = "high"
    malware_family = "PEVirus"
  strings:
    $hacked_section = ".hacked" ascii
  condition:
    uint16(0) == 0x5A4D and // MZ header
    $hacked_section
}

rule PEVirus_Mutex
{
  meta:
    author = "lab"
    description = "Detects PEVirus mutex used for single instance"
    severity = "high"
    malware_family = "PEVirus"
  strings:
    $mutex = "Global\\PEVirus_Mutex_npc0vo" ascii wide
  condition:
    uint16(0) == 0x5A4D and
    $mutex
}

rule PEVirus_Registry_Persistence
{
  meta:
    author = "lab"
    description = "Detects PEVirus persistence registry key"
    severity = "high"
    malware_family = "PEVirus"
  strings:
    $reg1 = "SystemUpdate" ascii wide
    $reg2 = "Software\\Microsoft\\Windows\\CurrentVersion\\Run" ascii wide
  condition:
    uint16(0) == 0x5A4D and
    all of them
}

rule PEVirus_Shellcode_Pattern
{
  meta:
    author = "lab"
    description = "Detects PEVirus shellcode patterns"
    severity = "high"
    malware_family = "PEVirus"
  strings:
    // Shellcode header from PEVirus.h
    $shellcode_start = { 41 51 41 50 52 51 56 48 33 D2 65 48 8B 52 60 }
    // Common shellcode patterns
    $api1 = "kernel32.dll" ascii wide nocase
    $api2 = "user32.dll" ascii wide nocase
    $api3 = "MessageBox" ascii wide nocase
  condition:
    uint16(0) == 0x5A4D and
    ($shellcode_start or (2 of ($api*)))
}

rule PEVirus_Startup_Copy
{
  meta:
    author = "lab"
    description = "Detects PEVirus startup folder persistence"
    severity = "medium"
    malware_family = "PEVirus"
  strings:
    $startup1 = "\\Start Menu\\Programs\\Startup\\" ascii wide
    $startup2 = "CSIDL_STARTUP" ascii
    $copy = "SystemUpdate.exe" ascii wide
  condition:
    uint16(0) == 0x5A4D and
    any of them
}

rule PEVirus_Daemon_Behavior
{
  meta:
    author = "lab"
    description = "Detects PEVirus daemon/persistence behavior"
    severity = "high"
    malware_family = "PEVirus"
  strings:
    $str1 = "Daemon thread" ascii wide
    $str2 = "Background daemon" ascii wide
    $str3 = "Persistence lost" ascii wide
    $str4 = "Repairing persistence" ascii wide
    $backup = "svchost_backup.exe" ascii wide
  condition:
    uint16(0) == 0x5A4D and
    2 of them
}

rule PEVirus_Full_Detection
{
  meta:
    author = "lab"
    description = "Comprehensive PEVirus detection combining multiple indicators"
    severity = "critical"
    malware_family = "PEVirus"
  strings:
    $sec = ".hacked" ascii
    $mutex = "PEVirus_Mutex" ascii
    $persist1 = "SystemUpdate" ascii
    $persist2 = "Background daemon" ascii
    $api1 = "WriteProcessMemory" ascii
    $api2 = "VirtualAllocEx" ascii
    $api3 = "CreateRemoteThread" ascii
  condition:
    uint16(0) == 0x5A4D and
    ($sec or ($mutex and $persist1) or (3 of ($api*)))
}

rule PEVirus_Infection_Marker
{
  meta:
    author = "lab"
    description = "Detects infection success messages and patterns"
    severity = "high"
    malware_family = "PEVirus"
  strings:
    $msg1 = "Infection Successful" ascii wide
    $msg2 = "Total files infected" ascii wide
    $msg3 = "File already infected" ascii wide
    $msg4 = "PEVirus Infected by npc0vo" ascii wide
  condition:
    uint16(0) == 0x5A4D and
    any of them
}

rule PEVirus_Encryption_Feature
{
  meta:
    author = "lab"
    description = "Detects PEVirus encryption functionality"
    severity = "high"
    malware_family = "PEVirus"
  strings:
    $enc1 = "EncryptFile" ascii wide
    $enc2 = "EncryptDirectory" ascii wide
    $enc3 = ".encrypted" ascii wide
    $enc4 = "PEVirus2024" ascii wide
  condition:
    uint16(0) == 0x5A4D and
    2 of them
}

rule PEVirus_C2_Communication
{
  meta:
    author = "lab"
    description = "Detects PEVirus C2 reverse shell capability"
    severity = "critical"
    malware_family = "PEVirus"
  strings:
    $c2_1 = "ReverseShellThread" ascii wide
    $c2_2 = "client/client.h" ascii
    $c2_3 = "CreateRemoteThread" ascii
    $c2_4 = "cmd.exe" ascii wide
  condition:
    uint16(0) == 0x5A4D and
    any of them
}

rule Generic_PE_Suspicious_Section
{
  meta:
    author = "lab"
    description = "Detects suspicious section names commonly used by malware"
    severity = "medium"
  strings:
    $s1 = ".hacked" ascii
    $s2 = ".evil" ascii
    $s3 = ".virus" ascii
    $s4 = ".inject" ascii
    $s5 = ".packed" ascii
    $s6 = "UPX0" ascii
    $s7 = "UPX1" ascii
  condition:
    uint16(0) == 0x5A4D and
    any of them
}
